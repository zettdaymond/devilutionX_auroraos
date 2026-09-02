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

/// Верхний уровень интерфейса: фон, панель навигации, роутер экранов,
/// модальные диалоги и файловый браузер MPQ.
///
/// Роль в MVI: чистая функция от LauncherState. Единственный способ
/// что-то изменить — отправить Intent; бизнес-состояния не хранит
/// (кроме экземпляра FileBrowser и учёта открытых попапов).
class LauncherView {
public:
	using Dispatcher = widgets::Dispatcher;

	explicit LauncherView(float dpiScale = 1.0F);
	~LauncherView();

	/// Рисует один кадр. `dispatch` потокобезопасен (Store::Dispatch).
	void Render(const LauncherState &state, const Dispatcher &dispatch);

	/// Закончилась ли iris-анимация запуска игры. Главный цикл держит
	/// кадр и дорисовывает диафрагму, пока это не станет истинно.
	[[nodiscard]] bool LaunchIrisDone() const;

	void SetBackgroundTexture(void *texture, ImVec2 size);

	/// Подключает собственный арт hero-панели режима; пустая текстура —
	/// остаётся обрезка общего фона.
	void SetHeroTexture(ExitAction mode, void *texture, ImVec2 size);

	/// Подключает уровни золотого силуэта иконки режима (от крупного);
	/// нулевое число — глиф FontAwesome в плитках.
	void SetModeIconLevels(ExitAction mode, const widgets::BackgroundArt *levels, int count);

private:
	void RenderBackground() const;
	void RenderQuickActions(const Dispatcher &dispatch);

	/// Закреплённая шапка вторичного экрана (кнопка «назад» + заголовок
	/// + разделитель) в собственном окне над прокручиваемым списком.
	/// Возвращает высоту занятого блока.
	float RenderScreenHeader(const char *title, const ImVec2 &pos, float width, const Dispatcher &dispatch);

	void RenderScreen(const LauncherState &state, const Dispatcher &dispatch);
	void RenderDialogs(const LauncherState &state, const Dispatcher &dispatch);
	void RenderFileBrowser(const LauncherState &state, const Dispatcher &dispatch);
	void RenderLaunchIris(const LauncherState &state);

	/// Тонкий золотой индикатор прокрутки у правого края: появляется при
	/// скролле и растворяется (замена скрытого родного скроллбара).
	void RenderScrollIndicator(float scrollY, float scrollMaxY, const ImVec2 &topLeft, float height);

	void *m_backgroundTexture = nullptr;
	ImVec2 m_backgroundTextureSize { 0.0F, 0.0F };
	widgets::BackgroundArt m_heroArts[3] = {};   // indexed by ExitAction
	widgets::IconSet m_iconSets[3] = {};         // indexed by ExitAction
	float m_dpiScale = 1.0F;

	Dialog m_lastDialog = Dialog::None;
	std::unique_ptr<ImGui::FileBrowser> m_fileBrowser;

	/// Истинно во время прокрутки перетаскиванием: клики на время жеста
	/// подавляются, чтобы провести пальцем по карточке — не значит
	/// «нажать» на неё.
	bool m_gestureDrag = false;

	/// Кинетическая прокрутка: скорость (пикселей в секунду) на момент
	/// отпускания — после жеста список движется по инерции и затухает.
	/// Скорость считается по следу позиций за ~120 мс: палец (в отличие
	/// от мышиного броска) замедляется перед подъёмом, и последние кадры
	/// занижают скорость.
	struct FlickSample {
		double time;
		float y;
	};
	FlickSample m_flickTrail[16] = {};
	int m_flickTrailLen = 0;
	float m_flickSpeed = 0.0F;
	bool m_flickActive = false;

	/// Телеметрия текущего скольжения (временная диагностика).
	float m_glideStartY = 0.0F;
	double m_glideStartedAt = 0.0;

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
