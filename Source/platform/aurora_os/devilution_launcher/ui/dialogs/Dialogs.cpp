#include "Dialogs.hpp"

#include "../Format.hpp"
#include "../Icons.hpp"
#include "../Scale.hpp"
#include "../Theme.hpp"

#include "core/GameFiles.hpp"

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

namespace launcher::ui::dialogs {

namespace {

/// Помощник для модального окна по центру. Ширина задаётся в rem и
/// всегда ограничена экраном, чтобы диалог не вылез за телефон.
/// Возвращает true, пока диалог открыт.
bool BeginModal(const char *name, float widthRem)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float maxWidth = viewport->WorkSize.x * 0.94F;
	const ImVec2 size(std::min(Scale::Px(widthRem), maxWidth), 0.0F);
	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5F,
	                    viewport->WorkPos.y + viewport->WorkSize.y * 0.5F),
	    ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
	ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, Theme::Color(ColorRole::Panel));
	const bool open = ImGui::BeginPopupModal(name, nullptr,
	    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar
	        | ImGuiWindowFlags_AlwaysAutoResize);
	if (!open) {
		ImGui::PopStyleColor();
	}
	return open;
}

/// Ширина одной из пары кнопок диалога: желаемый размер в rem,
/// но не больше половины доступного ряда.
ImVec2 PairedButtonSize(float widthRem, float heightRem)
{
	const float avail = ImGui::GetContentRegionAvail().x;
	const float half = (avail - ImGui::GetStyle().ItemSpacing.x) * 0.5F;
	return ImVec2(std::min(Scale::Px(widthRem), half), Scale::Px(heightRem));
}

void EndModal()
{
	ImGui::EndPopup();
	ImGui::PopStyleColor();
}

void ModalTitle(const char *text)
{
	Theme::PushFont(FontRole::BodyBold);
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextHeading));
	ImGui::TextUnformatted(text);
	ImGui::PopStyleColor();
	Theme::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.3F)));
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

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::Text("Размер: ~%s", FormatBytes(spec.expectedSizeBytes).c_str());
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::Text("Свободно: %s", FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));

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

/// Полоса прогресса: золотой градиент заливки и «горящий» край у кромки —
/// заливка раскаляется к переднему краю, над ней свечение и искры.
/// Мерцание и движение искр — чистые функции времени (как угольки фона).
void DrawProgressBar(ImDrawList *draw, const ImVec2 &pos, const ImVec2 &size, float fraction, bool active)
{
	const float fillX = pos.x + size.x * fraction;
	draw->AddRectFilled(pos, pos + size, Theme::ColorU32(ColorRole::PanelHover), Scale::Px(0.2F));
	if (fraction > 0.0F) {
		draw->AddRectFilledMultiColor(pos, pos + ImVec2(size.x * fraction, size.y),
		    Theme::ColorU32(ColorRole::Red), Theme::ColorU32(ColorRole::BorderGold),
		    Theme::ColorU32(ColorRole::BorderGold), Theme::ColorU32(ColorRole::Red));
	}
	if (!active || fraction <= 0.0F || fraction >= 1.0F) {
		return;
	}

	const float time = static_cast<float>(ImGui::GetTime());
	const float flicker = 0.75F + 0.25F * std::sin(time * 13.0F + 2.0F * std::sin(time * 5.1F));
	const float heat = std::min(fraction * 4.0F, 1.0F); // разгорается по мере старта

	const float hotWidth = std::min(Scale::Px(3.0F), size.x * fraction);
	const ImU32 hot = ImGui::GetColorU32(ImVec4(1.0F, 0.62F, 0.18F, 0.85F * flicker * heat));
	draw->AddRectFilledMultiColor(ImVec2(fillX - hotWidth, pos.y), ImVec2(fillX, pos.y + size.y),
	    IM_COL32(0, 0, 0, 0), hot, hot, IM_COL32(0, 0, 0, 0));

	// Мягкое свечение над кромкой: пара концентрических кругов с падающей
	// прозрачностью. Прямоугольник с жёсткими гранями здесь выглядел
	// посторонним блоком между искрами и шкалой.
	for (int ring = 2; ring >= 1; --ring) {
		const float radius = Scale::Px(0.22F * static_cast<float>(ring));
		const float ringAlpha = 0.22F * flicker * heat / static_cast<float>(ring);
		draw->AddCircleFilled(ImVec2(fillX, pos.y), radius,
		    ImGui::GetColorU32(ImVec4(1.0F, 0.55F, 0.15F, ringAlpha)), 10);
	}

	constexpr int kSparks = 3;
	for (int i = 0; i < kSparks; ++i) {
		const float seed = static_cast<float>(i) * 0.618034F;
		const float cycle = 0.7F + 0.5F * std::sin(seed * 9.4F);
		const float phase = std::fmod(time / cycle + seed, 1.0F);
		const float alpha = 0.8F * std::sin(phase * 3.14159265F) * flicker * heat;
		if (alpha <= 0.02F) {
			continue;
		}
		const float rise = Scale::Px(0.9F) * phase;
		// Рой чуть левее кромки, над раскалённой заливкой: точки справа
		// от кромки видны на тёмном треке, слева тонут в свечении, и рой
		// казался смещённым вправо.
		const ImVec2 spark(fillX - Scale::Px(0.2F) + std::sin(time * (2.2F + seed) + seed * 7.0F) * Scale::Px(0.25F),
		    pos.y - Scale::Px(0.12F) - rise);
		draw->AddCircleFilled(spark, Scale::Px(0.07F + 0.03F * std::sin(seed * 5.3F)),
		    ImGui::GetColorU32(ImVec4(1.0F, 0.66F, 0.22F, alpha)), 5);
	}
}

void Download(const LauncherState &state, const Dispatcher &dispatch)
{
	if (!state.download.has_value()) {
		return;
	}
	const DownloadState &download = *state.download;
	const FileSpec &spec = FileSpecOf(download.file);

	// Затемнение на весь экран под модалкой.
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::GetBackgroundDrawList()->AddRectFilled(viewport->WorkPos, viewport->WorkPos + viewport->WorkSize,
	    ImGui::GetColorU32(ImVec4(0, 0, 0, 0.80F)));

	if (!BeginModal("##download-overlay", 26.0F)) {
		return;
	}

	ModalTitle(download.active ? "Загрузка" : (download.error.empty() ? "Готово" : "Ошибка загрузки"));
	ImGui::TextUnformatted(spec.displayName.data());
	ImGui::Dummy(ImVec2(0, Scale::Px(0.5F)));

	const ImVec2 barSize(ImGui::GetContentRegionAvail().x, Scale::Px(1.2F));
	DrawProgressBar(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), barSize,
	    std::clamp(download.fraction, 0.0F, 1.0F), download.active);
	ImGui::Dummy(barSize);
	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));

	// Байты / проценты / скорость.
	const float fraction = std::clamp(download.fraction, 0.0F, 1.0F);
	const int percent = static_cast<int>(fraction * 100.0F);
	ImGui::Text("%d%%   %s / %s", percent,
	    FormatBytes(download.downloadedBytes).c_str(),
	    download.totalBytes > 0 ? FormatBytes(download.totalBytes).c_str() : "?");

// Скорость и оставшееся время
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	if (download.active && download.bytesPerSec > 0) {
		const int64_t remaining = download.bytesPerSec > 0
		    ? (std::max<int64_t>(download.totalBytes - download.downloadedBytes, 0)) / download.bytesPerSec
		    : -1;
		ImGui::Text("%s/с   %s", FormatBytes(download.bytesPerSec).c_str(), FormatEta(remaining).c_str());
	} else if (!download.error.empty()) {
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::Error));
		ImGui::TextWrapped("%s", download.error.c_str());
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	}
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
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

	ImGui::Dummy(ImVec2(0, Scale::Px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextWrapped("%s", "Скопируйте их в папку с DIABDAT.MPQ и повторите поиск, либо выберите другую папку.");
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::Px(0.5F)));
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
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::Error));
	ImGui::TextWrapped("%s", state.errorText.c_str());
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0, Scale::Px(0.5F)));
	if (ImGui::Button("Закрыть", ImVec2(Scale::Px(10.0F), Scale::Px(2.2F)))) {
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
	// Мерим тем же шрифтом и кеглем, которыми рисуем: CalcTextText по
	// умолчанию берёт текущий шрифт, и плашка получалась не по тексту —
	// он висел у правого края вместо центра.
	ImFont *toastFont = Theme::Font(FontRole::Body);
	const float toastSize = Scale::Px(1.0F);
	const ImVec2 textSize = toastFont != nullptr
	    ? toastFont->CalcTextSizeA(toastSize, FLT_MAX, 0.0F, state.toast->c_str())
	    : ImGui::CalcTextSize(state.toast->c_str());
	const ImVec2 size(textSize.x + Scale::Px(2.0F), textSize.y + Scale::Px(1.0F));
	const ImVec2 pos(viewport->WorkPos.x + (viewport->WorkSize.x - size.x) * 0.5F,
	    viewport->WorkPos.y + viewport->WorkSize.y - size.y - Scale::Px(1.4F)
	        + (1.0F - alpha) * Scale::Px(0.3F));

	ImDrawList *draw = ImGui::GetForegroundDrawList();
	auto withAlpha = [alpha](ImU32 col) {
		ImVec4 c = ImGui::ColorConvertU32ToFloat4(col);
		c.w *= alpha;
		return ImGui::ColorConvertFloat4ToU32(c);
	};
	draw->AddRectFilled(pos, pos + size, withAlpha(Theme::ColorU32(ColorRole::Panel)), Scale::Px(0.3F));
	draw->AddRect(pos, pos + size, withAlpha(Theme::ColorU32(ColorRole::GoldDim)), Scale::Px(0.3F), 0,
	    Scale::Px(0.06F));
	draw->AddText(toastFont, toastSize, pos + ImVec2(Scale::Px(1.0F), Scale::Px(0.5F)),
	    withAlpha(Theme::ColorU32(ColorRole::TextBody)), state.toast->c_str());
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
