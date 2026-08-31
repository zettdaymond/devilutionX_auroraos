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

/// Owns the SDL renderer, the ImGui context and the MVI trio
/// (Store + LauncherView over a ServiceBundle), and runs the launcher
/// UI loop until the user starts a game or closes the window.
///
/// Two constructors:
/// - (window, company, app) builds real platform services (used by the
///   game entry point in Source/main.cpp);
/// - (window, services) accepts an externally composed bundle, e.g.
///   mock scenarios on the desktop.
class Application {
public:
	Application(SDL_Window *window, const std::string &companyNamespace, const std::string &appName);
	Application(SDL_Window *window, launcher::ServiceBundle services);
	~Application();

	Application(const Application &) = delete;
	Application(Application &&) = delete;
	Application &operator=(const Application &) = delete;
	Application &operator=(Application &&) = delete;

	/// Runs the UI loop; returns what to launch.
	[[nodiscard]] AppResult Run();

	/// Override the first screen (desktop development aid).
	void SetInitialScreen(launcher::Screen screen) { m_initialScreen = screen; }

	/// Open the MPQ folder browser on startup (desktop development aid).
	void SetInitialBrowser(bool open) { m_initialBrowser = open; }

	/// Open a dialog on startup (desktop development aid):
	/// confirm-demo | confirm-ru | hellfire-missing | error.
	void SetInitialDialog(launcher::Dialog dialog) { m_initialDialog = dialog; }

	void Stop();
	void OnEvent(const SDL_WindowEvent &event);

private:
	/// Common SDL/ImGui bootstrap. Returns false on failure.
	bool Setup();

	/// Adds a rotating file sink next to the console logger so on-device
	/// issues can be read without root access to the system journal.
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

	/// Dedicated hero artworks, indexed by ExitAction (null = bg.png crop).
	std::array<SDL_Texture *, 3> m_heroTextures {};
	std::array<ImVec2, 3> m_heroSizes {};

	/// Golden tile icons as pre-scaled levels (256/128/64), indexed by
	/// ExitAction then level; count 3 = FA glyph fallback.
	std::array<std::array<SDL_Texture *, 3>, 3> m_iconTextures {};
	std::array<std::array<ImVec2, 3>, 3> m_iconSizes {};
	std::array<int, 3> m_iconLevelCounts {};

	bool m_running { true };
	std::optional<launcher::Screen> m_initialScreen;
	bool m_initialBrowser = false;
	std::optional<launcher::Dialog> m_initialDialog;
};

} // namespace App
