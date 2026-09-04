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
    IPathProvider &pathProvider,
    IEngineOptionsService &engineOptionsService,
    std::vector<int> displayHeights)
    : m_configService(configService)
    , m_filesService(filesService)
    , m_downloadService(downloadService)
    , m_pathProvider(pathProvider)
    , m_engineOptionsService(engineOptionsService)
    , m_displayHeights(std::move(displayHeights))
    , m_mainThreadId(std::this_thread::get_id())
{
}

void Store::Init()
{
	m_config = m_configService.Load();
	m_state.settingValues = m_engineOptionsService.Load();
	m_state.settingsLoaded = true;
	// Список разрешений — зеркало игрового: режимы дисплеев + сырое
	// значение из ini (движок всегда держит текущий выбор в списке).
	m_state.resolutionOptions = BuildResolutionOptions(m_displayHeights,
	    m_state.settingValues[static_cast<size_t>(SettingId::Resolution)]);
	RescanAndDerive();
}

void Store::SetWakeCallback(std::function<void()> callback)
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	m_wakeCallback = std::move(callback);
}

void Store::Dispatch(Intent intent)
{
	std::function<void()> wake;
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_queue.push_back(std::move(intent));
		// Будим главный поток только для «чужих» вызовов: свои интенты
		// (пользовательский ввод) он и так применит в ближайшем кадре.
		if (std::this_thread::get_id() != m_mainThreadId) {
			wake = m_wakeCallback;
		}
	}
	if (wake != nullptr) {
		wake();
	}
}

// ---- Intent handlers ----
// (определения должны идти до Poll()/Reduce(): специализации
// должны быть видны до неявного создания шаблона)

template <>
void Store::ReduceIntent<intent::UiNavigate>(const intent::UiNavigate &i)
{
	m_state.screen = i.screen;
	m_state.dialog = Dialog::None;
}

void Store::RescanAndDerive()
{
	const FileScanResult scan = m_filesService.Scan(m_pathProvider.CandidateDataDirs(m_config));
	m_state.fileSizes = scan.sizes;
	m_state.fileFolders = scan.folders;
	m_state.dataFolder = m_config.dataFolder.value_or(std::filesystem::path {});
	m_state.freeDiskBytes = m_filesService.FreeSpace(m_pathProvider.DownloadsDir());
	DeriveGameCards();
}

void Store::DeriveGameCards()
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

void Store::BeginDownload(KnownFile file)
{
	const FileSpec &spec = FileSpecOf(file);
	const int64_t freeBytes = m_filesService.FreeSpace(m_pathProvider.DownloadsDir());
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
		Dispatch(intent::EvDownloadProgress { total, downloaded, bytesPerSec });
	};
	listener.onFinished = [this](bool success, std::string error) {
		Dispatch(intent::EvDownloadFinished { success, std::move(error) });
	};

	m_downloadService.Start(std::string(DownloadUrl(file)),
	    m_pathProvider.DownloadsDir() / spec.canonical.data(),
	    std::move(listener));
}

// ---- Intent handlers ----

template <>
void Store::ReduceIntent<intent::UiOpenDialog>(const intent::UiOpenDialog &i)
{
	if (i.dialog == Dialog::HellfireMissingFiles) {
		m_state.hellfireMissing = m_state.hellfire.missingFiles;
	}
	m_state.dialog = i.dialog;
}

template <>
void Store::ReduceIntent<intent::UiCloseDialog>(const intent::UiCloseDialog &)
{
	if (m_state.dialog == Dialog::DownloadProgress && m_state.download && !m_state.download->active) {
// Закрытие оверлея завершённой или упавшей загрузки очищает её целиком.
		m_state.download.reset();
	}
	m_state.dialog = Dialog::None;
}

template <>
void Store::ReduceIntent<intent::UiDismissToast>(const intent::UiDismissToast &)
{
	m_state.toast.reset();
}

template <>
void Store::ReduceIntent<intent::SelectDataFolder>(const intent::SelectDataFolder &)
{
	m_state.fileBrowserOpen = true;
}

template <>
void Store::ReduceIntent<intent::DataFolderSelected>(const intent::DataFolderSelected &i)
{
	if (std::filesystem::is_directory(i.dir)) {
		m_config.dataFolder = i.dir;
		m_configService.Save(m_config);
		m_state.toast = "Папка с файлами игры обновлена";
	}
	RescanAndDerive();
	m_state.fileBrowserOpen = false;
}

template <>
void Store::ReduceIntent<intent::RescanFiles>(const intent::RescanFiles &)
{
	RescanAndDerive();
}

template <>
void Store::ReduceIntent<intent::CancelFolderSelection>(const intent::CancelFolderSelection &)
{
	m_state.fileBrowserOpen = false;
}

template <>
void Store::ReduceIntent<intent::DeleteDownloadedFile>(const intent::DeleteDownloadedFile &i)
{
	if (m_state.DownloadInProgress()) {
		return;
	}
	if (m_filesService.RemoveFile(m_pathProvider.DownloadsDir(), i.file)) {
		m_state.toast = "Файл удалён";
	} else {
		m_state.toast = "Не удалось удалить файл";
	}
	RescanAndDerive();
}

template <>
void Store::ReduceIntent<intent::LaunchGame>(const intent::LaunchGame &i)
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
		} else if (m_state.HasAnyFiles() || m_config.dataFolder) {
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
void Store::ReduceIntent<intent::StartDownload>(const intent::StartDownload &i)
{
	if (m_state.DownloadInProgress() || !FileSpecOf(i.file).downloadable) {
		return;
	}
	BeginDownload(i.file);
}

template <>
void Store::ReduceIntent<intent::CancelDownload>(const intent::CancelDownload &)
{
	if (m_state.DownloadInProgress()) {
		m_downloadService.Cancel();
	}
}

template <>
void Store::ReduceIntent<intent::SettingChanged>(const intent::SettingChanged &i)
{
	const size_t index = static_cast<size_t>(i.setting);
	if (index >= kSettingCount) {
		return;
	}
	m_state.settingValues[index] = i.value;
	// ini крошечный, а мобильный процесс могут убить в любой момент —
	// пишем сразу, без накопления.
	m_engineOptionsService.SaveAll(m_state.settingValues);
}

template <>
void Store::ReduceIntent<intent::SettingsReset>(const intent::SettingsReset &)
{
	m_state.settingValues = DefaultSettingValues();
	m_engineOptionsService.SaveAll(m_state.settingValues);
	m_state.dialog = Dialog::None;
	m_state.toast = "Настройки сброшены";
}

template <>
void Store::ReduceIntent<intent::EvDownloadProgress>(const intent::EvDownloadProgress &i)
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
void Store::ReduceIntent<intent::EvDownloadFinished>(const intent::EvDownloadFinished &i)
{
	if (!m_state.download) {
		return;
	}

	if (i.success) {
		m_state.download.reset();
		m_state.dialog = Dialog::None;
		m_state.toast = "Загрузка завершена";
		RescanAndDerive();
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

void Store::Poll()
{
	std::deque<Intent> pending;
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		pending.swap(m_queue);
	}
	for (const Intent &intent : pending) {
		Reduce(intent);
	}
}

void Store::Reduce(const Intent &intent)
{
	std::visit([this](const auto &i) { ReduceIntent(i); }, intent);
}

} // namespace launcher
