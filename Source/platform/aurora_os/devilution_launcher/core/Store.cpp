#include "Store.hpp"

#include <spdlog/spdlog.h>

#include <initializer_list>
#include <utility>

namespace launcher {

namespace {

std::vector<KnownFile> MissingOf(const std::array<int64_t, kKnownFileCount> &sizes,
    std::initializer_list<KnownFile> required)
{
	std::vector<KnownFile> missing;
	for (KnownFile file : required) {
		if (sizes[static_cast<size_t>(file)] < 0) {
			missing.push_back(file);
		}
	}
	return missing;
}

bool AllPresent(const std::array<int64_t, kKnownFileCount> &sizes,
    std::initializer_list<KnownFile> required)
{
	return MissingOf(sizes, required).empty();
}

constexpr std::initializer_list<KnownFile> kHellfireRequired {
	KnownFile::Diabdat, KnownFile::Hellfire, KnownFile::HfMonk, KnownFile::HfMusic, KnownFile::HfVoice
};

} // namespace

Store::Store(IConfigService &configService,
    IGameFilesService &filesService,
    IDownloadService &downloadService,
    IPathProvider &pathProvider)
    : m_configService(configService)
    , m_filesService(filesService)
    , m_downloadService(downloadService)
    , m_pathProvider(pathProvider)
{
}

void Store::init()
{
	m_config = m_configService.load();
	rescanAndDerive();
}

void Store::dispatch(Intent intent)
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	m_queue.push_back(std::move(intent));
}

// ---- Intent handlers ----
// (must be defined before poll()/reduce() so the specializations are
// seen before the template is implicitly instantiated there)

template <>
void Store::reduceIntent<intent::UiNavigate>(const intent::UiNavigate &i)
{
	m_state.screen = i.screen;
	m_state.dialog = Dialog::None;
}

void Store::rescanAndDerive()
{
	const FileScanResult scan = m_filesService.scan(m_pathProvider.candidateDataDirs(m_config));
	m_state.fileSizes = scan.sizes;
	m_state.fileFolders = scan.folders;
	m_state.dataFolder = m_config.dataFolder.value_or(std::filesystem::path {});
	m_state.freeDiskBytes = m_filesService.freeSpace(m_pathProvider.downloadsDir());
	deriveGameCards();
}

void Store::deriveGameCards()
{
	const auto &sizes = m_state.fileSizes;

	m_state.diablo.available = AllPresent(sizes, { KnownFile::Diabdat });
	m_state.diablo.missingFiles = MissingOf(sizes, { KnownFile::Diabdat });

	m_state.hellfire.available = AllPresent(sizes, kHellfireRequired);
	m_state.hellfire.missingFiles = MissingOf(sizes, kHellfireRequired);

	m_state.demo.available = AllPresent(sizes, { KnownFile::Spawn });
	m_state.demo.missingFiles = MissingOf(sizes, { KnownFile::Spawn });

	m_state.russianVoiceInstalled = sizes[static_cast<size_t>(KnownFile::RuVoice)] >= 0;
}

void Store::beginDownload(KnownFile file)
{
	const FileSpec &spec = FileSpecOf(file);
	const int64_t freeBytes = m_filesService.freeSpace(m_pathProvider.downloadsDir());
	if (spec.expectedSizeBytes > 0 && freeBytes >= 0 && freeBytes < spec.expectedSizeBytes) {
		m_state.errorText = "Недостаточно свободного места для загрузки";
		m_state.dialog = Dialog::Error;
		return;
	}

	DownloadState download;
	download.file = file;
	download.active = true;
	download.totalBytes = std::max<int64_t>(spec.expectedSizeBytes, 0);
	m_state.download = download;
	m_state.dialog = Dialog::DownloadProgress;

	IDownloadService::Listener listener;
	listener.onProgress = [this](int64_t total, int64_t downloaded, int64_t bytesPerSec) {
		dispatch(intent::EvDownloadProgress { total, downloaded, bytesPerSec });
	};
	listener.onFinished = [this](bool success, std::string error) {
		dispatch(intent::EvDownloadFinished { success, std::move(error) });
	};

	m_downloadService.start(std::string(DownloadUrl(file)),
	    m_pathProvider.downloadsDir() / spec.canonical.data(),
	    std::move(listener));
}

// ---- Intent handlers ----

template <>
void Store::reduceIntent<intent::UiOpenDialog>(const intent::UiOpenDialog &i)
{
	if (i.dialog == Dialog::HellfireMissingFiles) {
		m_state.hellfireMissing = m_state.hellfire.missingFiles;
	}
	m_state.dialog = i.dialog;
}

template <>
void Store::reduceIntent<intent::UiCloseDialog>(const intent::UiCloseDialog &)
{
	if (m_state.dialog == Dialog::DownloadProgress && m_state.download && !m_state.download->active) {
		// Closing a finished/failed download overlay clears it entirely.
		m_state.download.reset();
	}
	m_state.dialog = Dialog::None;
}

template <>
void Store::reduceIntent<intent::UiDismissToast>(const intent::UiDismissToast &)
{
	m_state.toast.reset();
}

template <>
void Store::reduceIntent<intent::SelectDataFolder>(const intent::SelectDataFolder &)
{
	m_state.fileBrowserOpen = true;
}

template <>
void Store::reduceIntent<intent::DataFolderSelected>(const intent::DataFolderSelected &i)
{
	if (std::filesystem::is_directory(i.dir)) {
		m_config.dataFolder = i.dir;
		m_configService.save(m_config);
		m_state.toast = "Папка с файлами игры обновлена";
	}
	rescanAndDerive();
	m_state.fileBrowserOpen = false;
}

template <>
void Store::reduceIntent<intent::RescanFiles>(const intent::RescanFiles &)
{
	rescanAndDerive();
}

template <>
void Store::reduceIntent<intent::CancelFolderSelection>(const intent::CancelFolderSelection &)
{
	m_state.fileBrowserOpen = false;
}

template <>
void Store::reduceIntent<intent::DeleteDownloadedFile>(const intent::DeleteDownloadedFile &i)
{
	if (m_state.downloadInProgress()) {
		return;
	}
	if (m_filesService.removeFile(m_pathProvider.downloadsDir(), i.file)) {
		m_state.toast = "Файл удалён";
	} else {
		m_state.toast = "Не удалось удалить файл";
	}
	rescanAndDerive();
}

template <>
void Store::reduceIntent<intent::LaunchGame>(const intent::LaunchGame &i)
{
	switch (i.game) {
	case ExitAction::LaunchDiablo:
		if (m_state.diablo.available) {
			m_state.pendingLaunch = ExitAction::LaunchDiablo;
		} else {
			m_state.fileBrowserOpen = true;
		}
		break;
	case ExitAction::LaunchHellfire:
		if (m_state.hellfire.available) {
			m_state.pendingLaunch = ExitAction::LaunchHellfire;
		} else if (m_state.hasAnyFiles() || m_config.dataFolder) {
			m_state.hellfireMissing = m_state.hellfire.missingFiles;
			m_state.dialog = Dialog::HellfireMissingFiles;
		} else {
			m_state.fileBrowserOpen = true;
		}
		break;
	case ExitAction::LaunchDemo:
		if (m_state.demo.available) {
			m_state.pendingLaunch = ExitAction::LaunchDemo;
		} else {
			m_state.dialog = Dialog::ConfirmDownloadDemo;
		}
		break;
	}
}

template <>
void Store::reduceIntent<intent::StartDownload>(const intent::StartDownload &i)
{
	if (m_state.downloadInProgress() || !FileSpecOf(i.file).downloadable) {
		return;
	}
	beginDownload(i.file);
}

template <>
void Store::reduceIntent<intent::CancelDownload>(const intent::CancelDownload &)
{
	if (m_state.downloadInProgress()) {
		m_downloadService.cancel();
	}
}

template <>
void Store::reduceIntent<intent::EvDownloadProgress>(const intent::EvDownloadProgress &i)
{
	if (!m_state.download || !m_state.download->active) {
		return;
	}
	m_state.download->totalBytes = i.totalBytes;
	m_state.download->downloadedBytes = i.downloadedBytes;
	m_state.download->bytesPerSec = i.bytesPerSec;
	m_state.download->fraction = i.totalBytes > 0
	    ? static_cast<float>(static_cast<double>(i.downloadedBytes) / static_cast<double>(i.totalBytes))
	    : 0.0F;
}

template <>
void Store::reduceIntent<intent::EvDownloadFinished>(const intent::EvDownloadFinished &i)
{
	if (!m_state.download) {
		return;
	}

	if (i.success) {
		m_state.download.reset();
		m_state.dialog = Dialog::None;
		m_state.toast = "Загрузка завершена";
		rescanAndDerive();
		return;
	}

	if (i.error == "cancelled") {
		m_state.download.reset();
		m_state.dialog = Dialog::None;
		return;
	}

	m_state.download->active = false;
	m_state.download->error = i.error;
}

void Store::poll()
{
	std::deque<Intent> pending;
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		pending.swap(m_queue);
	}
	for (const Intent &intent : pending) {
		reduce(intent);
	}
}

void Store::reduce(const Intent &intent)
{
	std::visit([this](const auto &i) { reduceIntent(i); }, intent);
}

} // namespace launcher
