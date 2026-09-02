#pragma once

#include <SDL2/SDL.h>
#include <imgui.h>

#include "core/AppResult.hpp"
#include "core/LauncherState.hpp"
#include "services/ServiceFactory.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace launcher {
class Store;
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
};

} // namespace App
