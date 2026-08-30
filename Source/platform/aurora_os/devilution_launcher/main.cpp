/// Desktop entry point for the DevilutionX launcher (development build).
///
/// Lets the launcher run standalone for fast UI iteration and user-flow
/// walkthroughs with scripted mock scenarios — no Aurora OS device and
/// no network required:
///
///   devilution_launcher [--mock-scenario=<name>] [--window=<WxH>]
///
/// Scenarios: empty, slow-download, fail-download, diablo-found,
/// hellfire-partial, full (see services/mocks/MockScenarios.cpp).

// Must precede every SDL include: we provide plain main() ourselves.
#define SDL_MAIN_HANDLED

#include "Application.hpp"

#include "services/mocks/MockScenarios.hpp"

#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Args {
	std::optional<std::string> mockScenario;
	std::optional<launcher::Screen> screen;
	int windowWidth = 540;
	int windowHeight = 960;
};

Args ParseArgs(int argc, char **argv)
{
	Args args;
	for (int i = 1; i < argc; ++i) {
		const std::string arg = argv[i];
		if (arg.rfind("--mock-scenario=", 0) == 0) {
			args.mockScenario = arg.substr(std::strlen("--mock-scenario="));
		} else if (arg.rfind("--screen=", 0) == 0) {
			const std::string value = arg.substr(std::strlen("--screen="));
			if (value == "home") {
				args.screen = launcher::Screen::Home;
			} else if (value == "data") {
				args.screen = launcher::Screen::Data;
			} else if (value == "about") {
				args.screen = launcher::Screen::About;
			}
		} else if (arg.rfind("--window=", 0) == 0) {
			const std::string value = arg.substr(std::strlen("--window="));
			if (const char *x = std::strchr(value.c_str(), 'x'); x != nullptr) {
				args.windowWidth = std::atoi(value.c_str());
				args.windowHeight = std::atoi(x + 1);
			}
		} else if (arg == "--help" || arg == "-h") {
			spdlog::info("Usage: devilution_launcher [--mock-scenario=<name>] [--screen=home|data|about] [--window=<WxH>]");
			spdlog::info("Scenarios:");
			for (const auto &name : launcher::MockScenario::names()) {
				spdlog::info("  {}", name);
			}
			std::exit(0);
		}
	}
	return args;
}

} // namespace

int main(int argc, char **argv)
{
	const Args args = ParseArgs(argc, argv);

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		spdlog::error("SDL_Init failed: {}", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow("DevilutionX Launcher",
	    SDL_WINDOWPOS_CENTERED,
	    SDL_WINDOWPOS_CENTERED,
	    args.windowWidth,
	    args.windowHeight,
	    SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (window == nullptr) {
		spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	std::optional<launcher::ServiceBundle> services;
	if (args.mockScenario.has_value()) {
		try {
			const launcher::MockScenario scenario = launcher::MockScenario::byName(*args.mockScenario);
			spdlog::info("Mock scenario: {}", scenario.name());
			services = scenario.makeBundle();
		} catch (const std::invalid_argument &err) {
			spdlog::error("{}", err.what());
			SDL_DestroyWindow(window);
			SDL_Quit();
			return 1;
		}
	}

	auto app = services.has_value()
	    ? std::make_unique<App::Application>(window, std::move(*services))
	    : std::make_unique<App::Application>(window, "org.diasurgical", "devilutionx");
	if (args.screen.has_value()) {
		app->setInitialScreen(*args.screen);
	}

	const App::AppResult result = app->run();

	spdlog::info("Launcher finished: success={}, action={}",
	    result.success,
	    result.success ? static_cast<int>(result.action) : -1);

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
