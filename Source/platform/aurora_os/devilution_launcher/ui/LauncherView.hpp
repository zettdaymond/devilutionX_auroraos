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

	/// Закончилась ли iris-анимация запуска игры. Главный цикл держит
	/// кадр и дорисовывает диафрагму, пока это не станет истинно.
	[[nodiscard]] bool LaunchIrisDone() const;

	void SetBackgroundTexture(void *texture, ImVec2 size);

	/// Attach a dedicated hero artwork for a game mode; a null texture
	/// keeps the bg.png-crop fallback for that mode.
	void SetHeroTexture(ExitAction mode, void *texture, ImVec2 size);

	/// Attach the golden silhouette icon for a game mode; null keeps the
	/// FontAwesome glyph fallback in tiles.
	void SetModeIconTexture(ExitAction mode, void *texture, ImVec2 size);

private:
	void RenderBackground() const;
	void RenderNavBar(const LauncherState &state, const Dispatcher &dispatch);
	void RenderScreen(const LauncherState &state, const Dispatcher &dispatch);
	void RenderDialogs(const LauncherState &state, const Dispatcher &dispatch);
	void RenderFileBrowser(const LauncherState &state, const Dispatcher &dispatch);
	void RenderLaunchIris(const LauncherState &state);

	/// Тонкий золотой индикатор прокрутки у правого края: появляется при
	/// скролле и растворяется (замена скрытого родного скроллбара).
	void RenderScrollIndicator(float scrollY, float scrollMaxY, const ImVec2 &topLeft, float height);

	void *m_backgroundTexture = nullptr;
	ImVec2 m_backgroundTextureSize { 0.0F, 0.0F };
	widgets::BackgroundArt m_heroArts[3] = {}; // indexed by ExitAction
	widgets::BackgroundArt m_iconArts[3] = {}; // indexed by ExitAction
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

	/// Момент старта iris-анимации запуска (< 0 — не запускалась).
	double m_irisStartedAt = -1.0;

	/// Оверлей-индикатор прокрутки: последняя позиция и момент активности.
	float m_lastContentScrollY = 0.0F;
	double m_scrollActiveAt = -1.0;
};

} // namespace launcher::ui
