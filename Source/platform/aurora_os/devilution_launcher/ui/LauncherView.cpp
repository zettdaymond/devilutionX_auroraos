#include "LauncherView.hpp"

#include "Icons.hpp"
#include "Scale.hpp"
#include "Theme.hpp"
#include "dialogs/Dialogs.hpp"
#include "screens/Screens.hpp"
#include "thirdparty/FileBrowser.h"

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <string>

namespace launcher::ui {

LauncherView::LauncherView(float dpiScale)
    : m_dpiScale(dpiScale)
{
	// Start browsing from the user's home directory — the most likely
	// place for transferred MPQ files (~/Documents, ~/Downloads).
	std::filesystem::path startDir;
#ifdef _WIN32
	if (const char *profile = std::getenv("USERPROFILE")) {
		startDir = profile;
	}
#else
	if (const char *home = std::getenv("HOME")) {
		startDir = home;
	}
#endif
	if (startDir.empty() || !std::filesystem::is_directory(startDir)) {
		startDir = std::filesystem::current_path();
	}

	m_fileBrowser = std::make_unique<ImGui::FileBrowser>(
	    ImGuiFileBrowserFlags_Fullscreen | ImGuiFileBrowserFlags_NoResize | ImGuiFileBrowserFlags_NoMove
	    | ImGuiFileBrowserFlags_NoTitleBar,
	    startDir);
	m_fileBrowser->SetTypeFilters({ "*.mpq", "*.MPQ" });
	m_fileBrowser->SetTitle("Выберите DIABDAT.MPQ");
	m_fileBrowser->SetMobileBehavior(true);
}

LauncherView::~LauncherView() = default;

void LauncherView::SetBackgroundTexture(void *texture, ImVec2 size)
{
	m_backgroundTexture = texture;
	m_backgroundTextureSize = size;
}

void LauncherView::Render(const LauncherState &state, const Dispatcher &dispatch)
{
	Scale::beginFrame(m_dpiScale);

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float navHeight = Scale::px(3.2F);
	const float pad = Scale::px(1.2F);

	// Root window: covers the viewport, no chrome.
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	// The root is a transparent layout container: NoInputs keeps it from
	// covering the navbar and stealing clicks when ImGui reorders windows
	// (children of a NoInputs window remain interactive on their own).
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
	    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
	    | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;

	ImGui::Begin("##launcher-root", nullptr, flags);
	RenderBackground();

	// Content area: the nav bar reserves space at the bottom (portrait)
	// or at the top (landscape).
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const ImVec2 contentPos(pad, Scale::portrait() ? pad : navHeight + pad * 0.5F);
	const ImVec2 contentSize(windowSize.x - pad * 2.0F, windowSize.y - navHeight - pad * 1.5F);

	ImGui::SetCursorPos(contentPos);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::BeginChild("##content", contentSize, ImGuiChildFlags_None)) {
		RenderScreen(state, dispatch);
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();

	ImGui::End();
	ImGui::PopStyleVar();

	RenderNavBar(state, dispatch);
	RenderDialogs(state, dispatch);
	RenderFileBrowser(state, dispatch);
	dialogs::Toast(state, dispatch);
}

void LauncherView::RenderBackground() const
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImDrawList *draw = ImGui::GetBackgroundDrawList();

	if (m_backgroundTexture != nullptr && m_backgroundTextureSize.x > 0 && m_backgroundTextureSize.y > 0) {
		// Aspect-fill: crop the image so it covers the viewport.
		const ImVec2 &v = viewport->WorkSize;
		const ImVec2 &t = m_backgroundTextureSize;
		const float scale = std::max(v.x / t.x, v.y / t.y);
		const ImVec2 shown(t.x * scale, t.y * scale);
		const ImVec2 crop(0.5F - (v.x / shown.x) * 0.5F, 0.5F - (v.y / shown.y) * 0.5F);
		draw->AddImage(m_backgroundTexture, viewport->WorkPos, viewport->WorkPos + v,
		    ImVec2(crop.x, crop.y), ImVec2(1.0F - crop.x, 1.0F - crop.y));
	}

	// Dark vignette so text stays readable over the artwork.
	const ImU32 shade = ImGui::GetColorU32(ImVec4(0.02F, 0.01F, 0.01F, 0.28F));
	draw->AddRectFilled(viewport->WorkPos, viewport->WorkPos + viewport->WorkSize, shade);
}

void LauncherView::RenderNavBar(const LauncherState &state, const Dispatcher &dispatch)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float height = Scale::px(3.2F);
	const ImVec2 size(viewport->WorkSize.x, height);
	const ImVec2 pos = Scale::portrait()
	    ? ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - height)
	    : viewport->WorkPos;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scale::px(0.4F), Scale::px(0.4F)));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::color(ColorRole::Panel));
	ImGui::Begin("##navbar", nullptr,
	    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
	        | ImGuiWindowFlags_NoBringToFrontOnFocus);

	struct NavItem {
		Screen screen;
		const char *icon;
		const char *label;
	};
	const NavItem items[] {
		{ Screen::Home, icons::Home, "Главная" },
		{ Screen::Data, icons::Folder, "Данные" },
		{ Screen::About, icons::Info, "О порте" },
	};

	const float buttonWidth = ImGui::GetContentRegionAvail().x / std::size(items);
	for (const NavItem &item : items) {
		const bool selected = (state.screen == item.screen);
		ImGui::PushStyleColor(ImGuiCol_Button, Theme::color(selected ? ColorRole::Red : ColorRole::Bg));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::color(selected ? ColorRole::RedHover : ColorRole::Panel));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::color(ColorRole::RedPressed));
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::color(selected ? ColorRole::GoldBright : ColorRole::TextBody));

		const std::string label = std::string(item.icon) + "  " + item.label;
		if (ImGui::Button(label.c_str(), ImVec2(buttonWidth - Scale::px(0.4F), height - Scale::px(0.8F)))) {
			dispatch(intent::UiNavigate { item.screen });
		}
		ImGui::SameLine(0, Scale::px(0.4F));
		ImGui::PopStyleColor(4);
	}

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}

void LauncherView::RenderScreen(const LauncherState &state, const Dispatcher &dispatch)
{
	const widgets::BackgroundArt art { m_backgroundTexture, m_backgroundTextureSize };
	switch (state.screen) {
	case Screen::Home:
		screens::Home(state, dispatch, art);
		break;
	case Screen::Data:
		screens::Data(state, dispatch);
		break;
	case Screen::About:
		screens::About(state);
		break;
	}
}

void LauncherView::RenderDialogs(const LauncherState &state, const Dispatcher &dispatch)
{
	if (state.dialog != m_lastDialog) {
		if (state.dialog != Dialog::None) {
			dialogs::OpenFor(state.dialog);
		}
		m_lastDialog = state.dialog;
	}

	switch (state.dialog) {
	case Dialog::ConfirmDownloadDemo:
		dialogs::confirm::Download(state, dispatch, KnownFile::Spawn);
		break;
	case Dialog::ConfirmDownloadRu:
		dialogs::confirm::Download(state, dispatch, KnownFile::RuVoice);
		break;
	case Dialog::DownloadProgress:
		dialogs::overlay::Download(state, dispatch);
		break;
	case Dialog::HellfireMissingFiles:
		dialogs::MissingFiles(state, dispatch);
		break;
	case Dialog::Error:
		dialogs::Error(state, dispatch);
		break;
	case Dialog::None:
		break;
	}
}

void LauncherView::RenderFileBrowser(const LauncherState &state, const Dispatcher &dispatch)
{
	if (state.fileBrowserOpen && !m_fileBrowser->IsOpened()) {
		m_fileBrowser->Open();
	} else if (!state.fileBrowserOpen && m_fileBrowser->IsOpened()) {
		m_fileBrowser->Close();
	}

	m_fileBrowser->Display();

	// The browser closed itself (Отмена / × / Esc) — sync the state,
	// otherwise the block above would immediately reopen it.
	if (state.fileBrowserOpen && !m_fileBrowser->IsOpened()) {
		dispatch(intent::CancelFolderSelection {});
	}

	if (m_fileBrowser->HasSelected()) {
		const std::filesystem::path selected = m_fileBrowser->GetSelected();
		m_fileBrowser->ClearSelected();
		dispatch(intent::DataFolderSelected { selected.parent_path() });
	}
}

} // namespace launcher::ui
