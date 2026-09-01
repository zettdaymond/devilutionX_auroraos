/// Десктопная входная точка лаунчера DevilutionX (сборка для разработки).
///
/// Позволяет запускать лаунчер отдельно — для быстрой работы над интерфейсом
/// и прохождения пользовательских сценариев по скриптам-имитациям,
/// сеть не нужна:
///
///   devilution_launcher [--mock-scenario=<имя>] [--window=<ШxВ]
///
/// Сценарии: empty, slow-download, fail-download, diablo-found,
/// hellfire-partial, full (см. services/mocks/MockScenarios.cpp).

// Должен стоять раньше всех включений SDL: свой обычный main().
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
	bool openBrowser = false;
	std::optional<launcher::Dialog> dialog;
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
			} else if (value == "settings") {
				args.screen = launcher::Screen::Settings;
			} else if (value == "about") {
				args.screen = launcher::Screen::About;
			}
		} else if (arg == "--open-browser") {
			args.openBrowser = true;
		} else if (arg.rfind("--dialog=", 0) == 0) {
			const std::string value = arg.substr(std::strlen("--dialog="));
			if (value == "confirm-demo") {
				args.dialog = launcher::Dialog::ConfirmDownloadDemo;
			} else if (value == "confirm-ru") {
				args.dialog = launcher::Dialog::ConfirmDownloadRu;
			} else if (value == "hellfire-missing") {
				args.dialog = launcher::Dialog::HellfireMissingFiles;
			} else if (value == "confirm-reset-settings") {
				args.dialog = launcher::Dialog::ConfirmResetSettings;
			} else if (value == "error") {
				args.dialog = launcher::Dialog::Error;
			}
		} else if (arg.rfind("--window=", 0) == 0) {
			const std::string value = arg.substr(std::strlen("--window="));
			if (const char *x = std::strchr(value.c_str(), 'x'); x != nullptr) {
				args.windowWidth = std::atoi(value.c_str());
				args.windowHeight = std::atoi(x + 1);
			}
		} else if (arg == "--help" || arg == "-h") {
			spdlog::info("Usage: devilution_launcher [--mock-scenario=<name>] [--screen=home|data|settings|about] [--dialog=confirm-reset-settings] [--window=<WxH>]");
			spdlog::info("Scenarios:");
			for (const auto &name : launcher::MockScenario::Names()) {
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
			const launcher::MockScenario scenario = launcher::MockScenario::ByName(*args.mockScenario);
			spdlog::info("Mock scenario: {}", scenario.name());
			services = scenario.MakeBundle();
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
		app->SetInitialScreen(*args.screen);
	}
	if (args.openBrowser) {
		app->SetInitialBrowser(true);
	}
	if (args.dialog.has_value()) {
		app->SetInitialDialog(*args.dialog);
	}

	const App::AppResult result = app->Run();

	spdlog::info("Launcher finished: success={}, action={}",
	    result.success,
	    result.success ? static_cast<int>(result.action) : -1);

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
