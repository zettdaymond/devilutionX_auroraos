#include "Application.hpp"

#include "DPIHandler.hpp"
#include "core/Store.hpp"
#include "ui/LauncherView.hpp"
#include "ui/Theme.hpp"

#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <cmrc/cmrc.hpp>

#include <filesystem>
#include <utility>

CMRC_DECLARE(assets);

namespace App {

Application::Application(SDL_Window *window, const std::string &companyNamespace, const std::string &appName)
    : m_window(window)
    , m_companyNamespace(companyNamespace)
    , m_appName(appName)
{
	const char *prefPath = SDL_GetPrefPath(m_companyNamespace.c_str(), m_appName.c_str());
	m_services = launcher::MakeRealServices(prefPath != nullptr ? std::filesystem::path(prefPath)
	                                                            : std::filesystem::temp_directory_path());
}

Application::Application(SDL_Window *window, launcher::ServiceBundle services)
    : m_window(window)
    , m_services(std::move(services))
{
}

Application::~Application()
{
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (m_backgroundTexture != nullptr) {
		SDL_DestroyTexture(m_backgroundTexture);
	}
	if (m_renderer != nullptr) {
		SDL_DestroyRenderer(m_renderer);
	}
}

bool Application::setup()
{
	m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (m_renderer == nullptr) {
		spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
		return false;
	}

	SDL_RendererInfo info;
	SDL_GetRendererInfo(m_renderer, &info);
	spdlog::info("Launcher SDL_Renderer: {}", info.name);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
	io.IniFilename = nullptr; // no imgui.ini — the launcher layout is fixed

	launcher::ui::Theme::init(DPIHandler::get_scale());

	ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
	ImGui_ImplSDLRenderer2_Init(m_renderer);

	// Background artwork from the embedded assets.
	try {
		auto file = cmrc::assets::get_filesystem().open("assets/bg.png");
		SDL_RWops *rw = SDL_RWFromMem(const_cast<void *>(static_cast<const void *>(file.begin())),
		    static_cast<int>(file.size()));
		SDL_Surface *surface = IMG_Load_RW(rw, 1);
		if (surface != nullptr) {
			m_backgroundTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
			m_backgroundSize = ImVec2(static_cast<float>(surface->w), static_cast<float>(surface->h));
			SDL_FreeSurface(surface);
		} else {
			spdlog::warn("IMG_Load_RW(bg.png) failed: {}", IMG_GetError());
		}
	} catch (const std::system_error &err) {
		spdlog::warn("bg.png asset missing: {}", err.what());
	}

	return true;
}

AppResult Application::run()
{
	m_store = std::make_unique<launcher::Store>(
	    *m_services.config, *m_services.files, *m_services.downloads, *m_services.paths);

	// setup() creates the ImGui context and loads fonts; the view (and
	// its FileBrowser, which queries the font atlas) must come after it.
	if (m_renderer == nullptr && !setup()) {
		AppResult result;
		result.success = false;
		return result;
	}
	m_view = std::make_unique<launcher::ui::LauncherView>(DPIHandler::get_scale());
	if (m_backgroundTexture != nullptr) {
		m_view->SetBackgroundTexture(m_backgroundTexture, m_backgroundSize);
	}

	m_store->init();
	if (m_initialScreen.has_value()) {
		m_store->dispatch(launcher::intent::UiNavigate { *m_initialScreen });
	}
	if (m_initialBrowser) {
		m_store->dispatch(launcher::intent::SelectDataFolder {});
	}
	if (m_initialDialog.has_value()) {
		m_store->dispatch(launcher::intent::UiOpenDialog { *m_initialDialog });
	}

	auto dispatch = [this](launcher::Intent intent) {
		m_store->dispatch(std::move(intent));
	};

	auto processEvent = [this](const SDL_Event &event) {
		ImGui_ImplSDL2_ProcessEvent(&event);

		if (event.type == SDL_QUIT) {
			stop();
		}
		if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(m_window)) {
			on_event(event.window);
		}
	};

	m_running = true;
	while (m_running && !m_store->state().pendingLaunch.has_value()) {
		// В фоне (свёрнуто/скрыто) цикл продолжает обслуживать события и
		// загрузки, но не рендерит. Вместо слепого сна ждём событие в ОС:
		// разворачивание обрабатывается мгновенно, а таймаут 250 мс равен
		// периоду троттлинга прогресса загрузок — просыпаемся ровно в такт
		// прибытию интентов из фоновой очереди (они приходят не как
		// SDL-события, поэтому бесконечное ожидание недопустимо).
		const Uint32 windowFlags = SDL_GetWindowFlags(m_window);
		const bool hidden = (windowFlags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) != 0;

		if (hidden) {
			SDL_Event wake {};
			if (SDL_WaitEventTimeout(&wake, 250) == 1) {
				processEvent(wake);
			}
		}

		SDL_Event event {};
		while (SDL_PollEvent(&event) == 1) {
			processEvent(event);
		}

		m_store->poll();
		if (hidden) {
			continue;
		}

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		m_view->Render(m_store->state(), dispatch);

		ImGui::Render();

		SDL_SetRenderDrawColor(m_renderer, 10, 7, 5, 255);
		SDL_RenderClear(m_renderer);
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
		SDL_RenderPresent(m_renderer);
	}

	AppResult result;
	if (const auto launch = m_store->state().pendingLaunch) {
		result.success = true;
		result.action = *launch;
		result.dataPath = m_store->state().dataFolder;
	}
	return result;
}

void Application::stop()
{
	m_running = false;
}

void Application::on_event(const SDL_WindowEvent &event)
{
	if (event.event == SDL_WINDOWEVENT_CLOSE) {
		stop();
	}
}

} // namespace App
