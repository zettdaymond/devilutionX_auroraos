#pragma once

#include <SDL2/SDL.h>
#include <imgui.h>

#include "core/AppResult.hpp"
#include "core/LauncherState.hpp"
#include "services/ServiceFactory.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace launcher {
class Store;
}

struct wl_registry;
struct wl_array;
struct qt_surface_extension;
struct qt_extended_surface;

namespace launcher::ui {
class LauncherView;
}

namespace App {

using launcher::AppResult;
using launcher::ExitAction;

/// Владеет рендерером SDL, контекстом ImGui и тройкой MVI
/// (Store + LauncherView поверх набора сервисов) и крутит цикл
/// интерфейса, пока пользователь не запустит игру или не закроет окно.
///
/// Два конструктора:
/// - (окно, компания, приложение) собирает настоящие сервисы платформы
///   (так лаунчер вызывается из входной точки игры в Source/main.cpp);
/// - (окно, сервисы) принимает уже готовый набор снаружи — например,
///   сценарии-имитации на десктопе.
class Application {
public:
	Application(SDL_Window *window, const std::string &companyNamespace, const std::string &appName);
	Application(SDL_Window *window, launcher::ServiceBundle services);
	~Application();

	Application(const Application &) = delete;
	Application(Application &&) = delete;
	Application &operator=(const Application &) = delete;
	Application &operator=(Application &&) = delete;

/// Запускает цикл интерфейса; результат — какую игру запустить.
	[[nodiscard]] AppResult Run();

/// Задать первый экран (утилита для отладки на десктопе).
	void SetInitialScreen(launcher::Screen screen) { m_initialScreen = screen; }

/// Открыть выбор папки с MPQ сразу при старте (отладка на десктопе).
	void SetInitialBrowser(bool open) { m_initialBrowser = open; }

/// Открыть диалог сразу при старте (отладка на десктопе):
/// confirm-demo | confirm-ru | hellfire-missing | error.
	void SetInitialDialog(launcher::Dialog dialog) { m_initialDialog = dialog; }

/// Рисовать вместо интерфейса кадр «обложки» свёрнутого приложения
/// (отладка обложки на десктопе, без сворачивания окна).
	void SetCoverPreview(bool enabled) { m_coverPreview = enabled; }

/// Начать загрузку демо сразу при старте (отладка обложки с прогрессом).
	void SetInitialDownload(bool start) { m_initialDownload = start; }

	void Stop();
	void OnEvent(const SDL_WindowEvent &event);

	/// Правда, если окно сейчас «в плитке»: скрыто/свёрнуто (десктоп) или
	/// надолго потеряло фокус (Аврора — см. m_focusLostAt).
	[[nodiscard]] bool IsTiled() const;

private:
	/// Общий бутстрап SDL/ImGui; false — фатальная ошибка инициализации.
	bool Setup();

	/// Грузит арт из вшитых ассетов (фон, hero-панели, иконки плиток).
	void LoadArtTextures();

	/// Дублирует лог в файл рядом с настройками: на устройстве системный
	/// журнал читается только рутом, а файл доступен пользователю напрямую.
	void AttachFileLog();

	/// Один кадр «обложки» для плитки Авроры: после сворачивания композитор
	/// показывает буфер окна в плитке домашнего экрана, поэтому вместо
	/// интерфейса рисуем фирменный кадр (LauncherView::RenderCover) и
	/// обновляем его только по факту изменений (прогресс загрузки).
	void RenderCoverFrame();

	SDL_Window *m_window { nullptr };
	SDL_Renderer *m_renderer { nullptr };

	std::string m_companyNamespace;
	std::string m_appName;

	launcher::ServiceBundle m_services;
	std::unique_ptr<launcher::Store> m_store;
	std::unique_ptr<launcher::ui::LauncherView> m_view;

	SDL_Texture *m_backgroundTexture { nullptr };
	ImVec2 m_backgroundSize { 0.0F, 0.0F };

	/// Арты hero-панелей режимов; индекс — значение ExitAction (пусто = обрезка общего фона).
	std::array<SDL_Texture *, 3> m_heroTextures {};
	std::array<ImVec2, 3> m_heroSizes {};

	/// Золотые иконки плиток в виде заранее уменьшенных копий (256/128/64);
	/// индексы: сначала режим, затем уровень; 0 уровней = глиф FontAwesome.
	std::array<std::array<SDL_Texture *, 3>, 3> m_iconTextures {};
	std::array<std::array<ImVec2, 3>, 3> m_iconSizes {};
	std::array<int, 3> m_iconLevelCounts {};

	bool m_running { true };
	std::optional<launcher::Screen> m_initialScreen;
	bool m_initialBrowser = false;
	std::optional<launcher::Dialog> m_initialDialog;
	bool m_coverPreview = false;
	bool m_initialDownload = false;

	/// Тип пользовательского SDL-события: «фоновый поток положил интент
	/// в очередь Store» — будит блокирующее ожидание свёрнутого цикла.
	Uint32 m_wakeEventType { 0 };

	/// Состояние обложки свёрнутого окна: был ли кадр уже нарисован и
	/// устарел ли он (пришёл wake-пинок с новым прогрессом загрузки).
	bool m_wasHidden = false;
	bool m_coverDirty = false;

	/// Дисплей включён и не заблокирован. На Авроре — из сигналов демона
	/// mce, на десктопе — всегда true. Погашенный/заблокированный экран =
	/// не рендерить вовсе: композитор окно не показывает, а кадры в тёмную
	/// матрицу тратят батарею.
	bool m_displayOn = true;
	bool m_tkLocked = false;

#ifdef AURORA_OS
	/// Аврора не шлёт MINIMIZED/HIDDEN при сворачивании в плитку — только
	/// FOCUS_LOST. «В плитке» = потеря переднего плана: сигнал композитора
	/// privateTopmostWindowProcessIdChanged действует мгновенно (он
	/// авторитетен), а SDL-фокус — фолбэк с дебаунсом 100 мс.
	std::optional<std::chrono::steady_clock::time_point> m_focusLostAt;
	bool m_topmostLost = false;

	/// Грейс после пробуждения (разблокировка): пару секунд считаем себя
	/// передним планом — между «экран разблокирован» и «композитор поднял
	/// окно» проходит анимация локскрина, и обложка в этом зазоре
	/// мелькает поверх неё. Если мы и правда плитка, topmost за грейс
	/// не вернётся — тогда обложка. Грейс не нужен, если перед сном мы
	/// уже были плиткой.
	std::optional<std::chrono::steady_clock::time_point> m_wakeGraceUntil;
	bool m_skipWakeGrace = false;

	/// Наблюдатель состояния: поток слушает D-Bus (демон mce — дисплей и
	/// блокировка; композитор Lipstick — верхнее окно) и переправляет
	/// изменения в очередь событий SDL своим пользовательским событием.
	std::thread m_displayWatch;
	std::atomic<bool> m_displayWatchStop { false };
	Uint32 m_displayEventType { 0 };

	/// ВРЕМЕННАЯ телеметрия «aurora-probe» (переходы режима плитки).
	bool m_wasTiledProbe = false;

	/// Wayland-хук плитки: реестр, расширение Qt и обёртка нашей
	/// поверхности; активна ли «плитка» по слову композитора.
	struct wl_registry *m_coverRegistry = nullptr;
	struct qt_surface_extension *m_coverExtension = nullptr;
	struct qt_extended_surface *m_coverSurface = nullptr;
	bool m_coverActive = false;

	void StartDisplayWatch();
	void StopDisplayWatch();
	void DisplayWatchLoop();
	void PushStateEvent(int what, bool value);

	/// Хук на Wayland-расширение Qt (qt_surface_extension): Lipstick
	/// сообщает окну состояние «плитки» свойством cover_status ещё ДО
	/// отпускания пальца в жесте сворачивания. Колбэки приходят в потоке
	/// SDL (внутри Poll/WaitEvent) — состояние меняем прямо из них.
	void InitCoverWatch();
	void StopCoverWatch();
	void OnCoverProperty(const char *name, const struct wl_array *value);
#endif
};

} // namespace App
