#pragma once

#include "core/Intent.hpp"
#include "core/LauncherState.hpp"
#include "widgets/Widgets.hpp"

#include <imgui.h>

#include <memory>

namespace ImGui {
class FileBrowser;
}

namespace launcher::ui {

/// Top-level view: background, navigation bar, screen router, modal
/// dialogs and the MPQ file browser.
///
/// MVI role: a pure function of LauncherState. The only way it changes
/// anything is by dispatching Intents; it owns no business state
/// (besides the FileBrowser widget instance and popup bookkeeping).
class LauncherView {
public:
	using Dispatcher = widgets::Dispatcher;

	explicit LauncherView(float dpiScale = 1.0F);
	~LauncherView();

	/// Render one frame. `dispatch` is thread-safe (Store::dispatch).
	void Render(const LauncherState &state, const Dispatcher &dispatch);

	void SetBackgroundTexture(void *texture, ImVec2 size);

private:
	void RenderBackground() const;
	void RenderNavBar(const LauncherState &state, const Dispatcher &dispatch);
	void RenderScreen(const LauncherState &state, const Dispatcher &dispatch);
	void RenderDialogs(const LauncherState &state, const Dispatcher &dispatch);
	void RenderFileBrowser(const LauncherState &state, const Dispatcher &dispatch);

	void *m_backgroundTexture = nullptr;
	ImVec2 m_backgroundTextureSize { 0.0F, 0.0F };
	float m_dpiScale = 1.0F;

	Dialog m_lastDialog = Dialog::None;
	std::unique_ptr<ImGui::FileBrowser> m_fileBrowser;

	/// True while the user is drag-scrolling the content (touch): clicks
	/// dispatched during the gesture are suppressed so dragging over a
	/// card does not "press" it.
	bool m_gestureDrag = false;

	/// Времена появления текущего экрана/диалога — для fade-анимаций
	/// (чисто презентационное состояние view-слоя).
	Screen m_lastScreen = Screen::Home;
	double m_screenShownAt = 0.0;
	double m_dialogShownAt = 0.0;
};

} // namespace launcher::ui
