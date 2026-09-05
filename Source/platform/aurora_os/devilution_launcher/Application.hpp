#pragma once

#include <SDL2/SDL.h>
#include <imgui.h>

#include "core/AppResult.hpp"
#include "core/LauncherState.hpp"
#include "services/ServiceFactory.hpp"

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace launcher {
class Store;
}

namespace launcher::aurora {
class StateWatch;
}


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

/// Рисовать карточку «Пауза» в режиме превью обложки (отладка плитки
/// на фазе движка: состояние pendingLaunch, которого в превью нет).
	void SetCoverPause(bool enabled) { m_coverPause = enabled; }

	void Stop();
	void OnEvent(const SDL_WindowEvent &event);

private:
	/// Общий бутстрап SDL/ImGui; false — фатальная ошибка инициализации.
	bool Setup();

	/// Грузит арт из вшитых ассетов (фон, hero-панели, иконки плиток).
	void LoadArtTextures();

	/// Дублирует лог в файл рядом с настройками: на устройстве системный
	/// журнал читается только рутом, а файл доступен пользователю напрямую.
	void AttachFileLog();

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

	/// Лица режимов для карточки плитки (белый лайн-арт на прозрачном);
	/// индекс — ExitAction, демо наследует череп Diablo.
	std::array<SDL_Texture *, 3> m_coverFaceTextures {};
	std::array<ImVec2, 3> m_coverFaceSizes {};

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
	bool m_coverPause = false;
	bool m_initialDownload = false;

	/// Общий обработчик SDL-событий: кадры ImGui, кардиограмма обложки,
	/// QUIT/CLOSE. Работает и в главном цикле, и в цикле обложки.
	void ProcessEvent(const SDL_Event &event);

	/// Правда, если окно сейчас никому не видно (плитка Авроры, гашение
	/// экрана, минимизация на десктопе) — интерфейс рендерить не для кого.
	/// До первого FOCUS_GAINED всегда false: на Авроре FOCUS_LOST приходит
	/// ДО первого кадра, а фокус композитор даёт только показанному окну.
	[[nodiscard]] bool WindowHidden() const;

	/// Цикл свёрнутого состояния: плитку ведёт нативная обложка Lipstick,
	/// поэтому интерфейс не рендерится вовсе — спим в блокирующем
	/// SDL_WaitEvent (будит любое событие: возврат фокуса, закрытие,
	/// пинки стора и наблюдателя состояния) и выходим, когда окно снова
	/// видимо, приложение закрыто или запускается игра.
	void RunCoverLoop();

	/// Окно получало фокус хотя бы раз — защёлка от дедлока старта
	/// (см. WindowHidden).
	bool m_focusKnown = false;

#ifdef AURORA_OS
	/// Наблюдатель состояния Авроры на фазу лаунчера: дисплей для
	/// WindowHidden(); события из его потока дрена будят цикл обложки.
	std::unique_ptr<launcher::aurora::StateWatch> m_stateWatch;

	/// Свежий кадр карточки: ImGui-рендер обложки (LauncherView::
	/// RenderCover) в офскрин-таргет размера окна обложки (NativeCover::
	/// Size — аспект плитки) и снимок пикселей RGB24. false — кадр не
	/// собрался (нет рендерера/таргет не создался).
	[[nodiscard]] bool RenderCoverPixels(
	    std::vector<unsigned char> &outPixels, int &outWidth, int &outHeight);

	/// Перерисовать нативную обложку живым кадром (если связана).
	void UpdateNativeCover();

	/// POC нативной обложки Lipstick: окно категории cover, связанное с
	/// главным через SAILFISH_COVER_WINDOW (см. NativeCover). Отключается
	/// на устройстве через DEVILUTIONX_NATIVE_COVER=0.
	void TryNativeCover();

	/// Нативная обложка связана (живой рендер и кардиограмма отладки).
	bool m_nativeCoverActive = false;

	/// Пользовательское событие «перезалить кадр обложки» (таймер 500 мс
	/// в режиме DEVILUTIONX_NATIVE_COVER_DEBUG=1).
	Uint32 m_nativeCoverHeartbeat = 0;

	/// Кадр обложки устарел: wake-пинок стора (прогресс загрузки),
	/// configure свитчера или кардиограмма — цикл обложки перерисует.
	bool m_coverDirty = false;

	/// Тип пользовательского SDL-события «фоновый поток положил интент
	/// в Store» — будит цикл обложки на перерисовку прогресса в плитке.
	Uint32 m_wakeEventType { 0 };

#endif
};

} // namespace App
