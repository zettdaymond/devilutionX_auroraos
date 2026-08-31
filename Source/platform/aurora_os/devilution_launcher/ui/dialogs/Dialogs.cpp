#include "Dialogs.hpp"

#include "../Format.hpp"
#include "../Icons.hpp"
#include "../Scale.hpp"
#include "../Theme.hpp"

#include "core/GameFiles.hpp"

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <string>

namespace launcher::ui::dialogs {

namespace {

/// Centered modal window helper. The desired width is given in rem and
/// always capped by the viewport so the dialog can never overflow a
/// phone screen. Returns true while the dialog is open.
bool BeginModal(const char *name, float widthRem)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float maxWidth = viewport->WorkSize.x * 0.94F;
	const ImVec2 size(std::min(Scale::px(widthRem), maxWidth), 0.0F);
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5F,
	                    viewport->WorkPos.y + viewport->WorkSize.y * 0.5F),
	    ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
	ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, Theme::color(ColorRole::Panel));
	const bool open = ImGui::BeginPopupModal(name, nullptr,
	    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar
	        | ImGuiWindowFlags_AlwaysAutoResize);
	if (!open) {
		ImGui::PopStyleColor();
	}
	return open;
}

/// Width for one of a pair of dialog buttons: the preferred rem-based
/// size, never wider than half of the available row.
ImVec2 PairedButtonSize(float widthRem, float heightRem)
{
	const float avail = ImGui::GetContentRegionAvail().x;
	const float half = (avail - ImGui::GetStyle().ItemSpacing.x) * 0.5F;
	return ImVec2(std::min(Scale::px(widthRem), half), Scale::px(heightRem));
}

void EndModal()
{
	ImGui::EndPopup();
	ImGui::PopStyleColor();
}

void ModalTitle(const char *text)
{
	Theme::pushFont(FontRole::BodyBold);
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextHeading));
	ImGui::TextUnformatted(text);
	ImGui::PopStyleColor();
	Theme::popFont();
	ImGui::Dummy(ImVec2(0, Scale::px(0.3F)));
}

} // namespace

namespace confirm {

void Download(const LauncherState &state, const Dispatcher &dispatch, KnownFile file)
{
	const FileSpec &spec = FileSpecOf(file);
	const bool isDemo = (file == KnownFile::Spawn);
	const char *title = isDemo ? "Скачать демо-версию?" : "Скачать русскую озвучку?";

	if (!BeginModal(title, 24.0F)) {
		return;
	}

	ModalTitle(title);
	ImGui::TextWrapped("%s",
	    isDemo
	        ? "Бесплатная shareware-версия Diablo (~2 уровня подземелий).\n"
	          "Полная игра таким образом не заменяется."
	        : "Русская озвучка и тексты для полной версии игры.");

	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	ImGui::Text("Размер: ~%s", FormatBytes(spec.expectedSizeBytes).c_str());
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	ImGui::Text("Свободно: %s", FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));

	const ImVec2 buttonSize = PairedButtonSize(9.0F, 2.2F);
	if (ImGui::Button("Скачать", buttonSize)) {
		ImGui::CloseCurrentPopup();
		dispatch(intent::StartDownload { file });
	}
	ImGui::SameLine();
	widgets::IconButton(icons::Times, "Отмена", false, buttonSize, [&dispatch] {
		ImGui::CloseCurrentPopup();
		dispatch(intent::UiCloseDialog {});
	});

	EndModal();
}

} // namespace confirm

namespace overlay {

void Download(const LauncherState &state, const Dispatcher &dispatch)
{
	if (!state.download.has_value()) {
		return;
	}
	const DownloadState &download = *state.download;
	const FileSpec &spec = FileSpecOf(download.file);

	// Darkened fullscreen overlay.
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImDrawList *draw = ImGui::GetBackgroundDrawList();
	draw->AddRectFilled(viewport->WorkPos, viewport->WorkPos + viewport->WorkSize,
	    ImGui::GetColorU32(ImVec4(0, 0, 0, 0.80F)));

	if (!BeginModal("##download-overlay", 26.0F)) {
		return;
	}

	ModalTitle(download.active ? "Загрузка" : (download.error.empty() ? "Готово" : "Ошибка загрузки"));
	ImGui::TextUnformatted(spec.displayName.data());
	ImGui::Dummy(ImVec2(0, Scale::px(0.5F)));

	// Progress bar with a gold gradient fill.
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 barPos = ImGui::GetCursorScreenPos();
	const ImVec2 barSize(width, Scale::px(1.2F));
	const float fraction = std::clamp(download.fraction, 0.0F, 1.0F);
	draw->AddRectFilled(barPos, barPos + barSize, Theme::colorU32(ColorRole::PanelHover), Scale::px(0.2F));
	if (fraction > 0.0F) {
		draw->AddRectFilledMultiColor(barPos, barPos + ImVec2(barSize.x * fraction, barSize.y),
		    Theme::colorU32(ColorRole::Red), Theme::colorU32(ColorRole::BorderGold),
		    Theme::colorU32(ColorRole::BorderGold), Theme::colorU32(ColorRole::Red));
	}
	ImGui::Dummy(barSize);
	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));

	// Bytes / percent
	const int percent = static_cast<int>(fraction * 100.0F);
	ImGui::Text("%d%%   %s / %s", percent,
	    FormatBytes(download.downloadedBytes).c_str(),
	    download.totalBytes > 0 ? FormatBytes(download.totalBytes).c_str() : "?");

	// Speed + ETA
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	if (download.active && download.bytesPerSec > 0) {
		const int64_t remaining = download.bytesPerSec > 0
		    ? (std::max<int64_t>(download.totalBytes - download.downloadedBytes, 0)) / download.bytesPerSec
		    : -1;
		ImGui::Text("%s/с   %s", FormatBytes(download.bytesPerSec).c_str(), FormatEta(remaining).c_str());
	} else if (!download.error.empty()) {
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::Error));
		ImGui::TextWrapped("%s", download.error.c_str());
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	}
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::px(0.6F)));
	const ImVec2 buttonSize = PairedButtonSize(10.0F, 2.2F);
	if (download.active) {
		widgets::IconButton(icons::Times, "Отмена", false, buttonSize, [&dispatch] {
			dispatch(intent::CancelDownload {});
		});
	} else if (!download.error.empty()) {
		if (ImGui::Button("Повторить", buttonSize)) {
			ImGui::CloseCurrentPopup();
			dispatch(intent::StartDownload { download.file });
		}
		ImGui::SameLine();
		widgets::IconButton(icons::Times, "Закрыть", false, buttonSize, [&dispatch] {
			ImGui::CloseCurrentPopup();
			dispatch(intent::UiCloseDialog {});
		});
	}

	EndModal();
}

} // namespace overlay

void MissingFiles(const LauncherState &state, const Dispatcher &dispatch)
{
	if (!BeginModal("Не хватает файлов", 26.0F)) {
		return;
	}

	ModalTitle("Не хватает файлов Hellfire");
	ImGui::TextWrapped("%s", "Для запуска Hellfire нужны все файлы дополнения:");

	for (KnownFile file : state.hellfireMissing) {
		ImGui::BulletText("%s", FileSpecOf(file).canonical.data());
	}

	ImGui::Dummy(ImVec2(0, Scale::px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::TextDim));
	ImGui::TextWrapped("%s", "Скопируйте их в папку с DIABDAT.MPQ и повторите поиск, либо выберите другую папку.");
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::px(0.5F)));
	const ImVec2 buttonSize = PairedButtonSize(11.0F, 2.2F);
	widgets::IconButton(icons::Folder, "Выбрать папку", true, buttonSize, [&dispatch] {
		ImGui::CloseCurrentPopup();
		dispatch(intent::SelectDataFolder {});
	});
	ImGui::SameLine();
	widgets::IconButton(icons::Times, "Закрыть", false, buttonSize, [&dispatch] {
		ImGui::CloseCurrentPopup();
		dispatch(intent::UiCloseDialog {});
	});

	EndModal();
}

void Error(const LauncherState &state, const Dispatcher &dispatch)
{
	if (!BeginModal("Ошибка", 24.0F)) {
		return;
	}

	ModalTitle("Ошибка");
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(ColorRole::Error));
	ImGui::TextWrapped("%s", state.errorText.c_str());
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::px(0.5F)));
	if (ImGui::Button("Закрыть", ImVec2(Scale::px(10.0F), Scale::px(2.2F)))) {
		ImGui::CloseCurrentPopup();
		dispatch(intent::UiCloseDialog {});
	}

	EndModal();
}

void Toast(const LauncherState &state, const Dispatcher &dispatch)
{
	static std::chrono::steady_clock::time_point shownAt {};
	static std::string lastToast {};

	if (!state.toast.has_value()) {
		lastToast.clear();
		return;
	}

	if (*state.toast != lastToast) {
		lastToast = *state.toast;
		shownAt = std::chrono::steady_clock::now();
	}

	// Плавное появление и растворение перед авто-закрытием.
	constexpr float kShowTime = 3.0F;
	const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - shownAt).count();
	if (elapsed >= kShowTime) {
		dispatch(intent::UiDismissToast {});
		return;
	}
	const float alpha = std::clamp(std::min(elapsed / 0.15F, (kShowTime - elapsed) / 0.35F), 0.0F, 1.0F);

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 textSize = ImGui::CalcTextSize(state.toast->c_str());
	const ImVec2 size(textSize.x + Scale::px(2.0F), textSize.y + Scale::px(1.0F));
	const ImVec2 pos(viewport->WorkPos.x + (viewport->WorkSize.x - size.x) * 0.5F,
	    viewport->WorkPos.y + viewport->WorkSize.y - size.y - Scale::px(1.4F)
	        + (1.0F - alpha) * Scale::px(0.3F));

	ImDrawList *draw = ImGui::GetForegroundDrawList();
	auto withAlpha = [alpha](ImU32 col) {
		ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
		c.w *= alpha;
		return ImGui::ColorConvertFloat4ToU32(c);
	};
	draw->AddRectFilled(pos, pos + size, withAlpha(Theme::colorU32(ColorRole::Panel)), Scale::px(0.3F));
	draw->AddRect(pos, pos + size, withAlpha(Theme::colorU32(ColorRole::GoldDim)), Scale::px(0.3F), 0,
	    Scale::px(0.06F));
	draw->AddText(Theme::font(FontRole::Body), Scale::px(1.0F), pos + ImVec2(Scale::px(1.0F), Scale::px(0.5F)),
	    withAlpha(Theme::colorU32(ColorRole::TextBody)), state.toast->c_str());
}

const char *ConfirmDemoId()
{
	return "Скачать демо-версию?";
}

const char *ConfirmRuId()
{
	return "Скачать русскую озвучку?";
}

const char *DownloadOverlayId()
{
	return "##download-overlay";
}

void OpenFor(Dialog dialog)
{
	switch (dialog) {
	case Dialog::ConfirmDownloadDemo:
		ImGui::OpenPopup(ConfirmDemoId());
		break;
	case Dialog::ConfirmDownloadRu:
		ImGui::OpenPopup(ConfirmRuId());
		break;
	case Dialog::DownloadProgress:
		ImGui::OpenPopup(DownloadOverlayId());
		break;
	case Dialog::HellfireMissingFiles:
		ImGui::OpenPopup("Не хватает файлов");
		break;
	case Dialog::Error:
		ImGui::OpenPopup("Ошибка");
		break;
	case Dialog::None:
		break;
	}
}

} // namespace launcher::ui::dialogs
