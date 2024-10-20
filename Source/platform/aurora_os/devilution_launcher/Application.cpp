#include "Application.hpp"

#include <SDL2/SDL.h>
#include <SDL_image.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <imgui.h>

#include <memory>
#include <string>

#include "DPIHandler.hpp"

#include <spdlog/spdlog.h>

#include "FontStorage.hpp"
#include "GameModel.hpp"
#include "GameView.hpp"
#include "GamePresenter.hpp"

namespace App {

Application::Application(SDL_Window* window, std::string const& company_namespace, std::string const& app_name)
   : m_window(window)
   , m_company_namespace(company_namespace)
   , m_app_name(app_name)
{
    Uint32 renderer_flags{SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED};
    m_renderer = SDL_CreateRenderer(m_window, -1, renderer_flags);

    if (m_renderer == nullptr) {
        spdlog::error("Error creating SDL_Renderer!");
        return;
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(m_renderer, &info);

    spdlog::info("Current SDL_Renderer: {}", info.name);
}

Application::~Application()
{
    SDL_DestroyRenderer(m_renderer);
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

ExitStatus App::Application::run()
{
    if (m_exit_status == ExitStatus::FAILURE) {
        return m_exit_status;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io{ImGui::GetIO()};

    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    const std::string user_config_path{SDL_GetPrefPath(m_company_namespace.c_str(), m_app_name.c_str())};

    // Absolute imgui.ini path to preserve settings independent of app location.
    static const std::string imgui_ini_filename{user_config_path + "imgui.ini"};
    io.IniFilename = imgui_ini_filename.c_str();

    auto fs = cmrc::assets::get_filesystem();

    // ImGUI font
    FontStorage::loadFonts();

    io.FontDefault = FontStorage::getFont("defaultFont");

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);

    auto bg = fs.open("assets/bg.png");
    auto bg_rw = SDL_RWFromMem((void*)bg.begin(), bg.size());
    auto m_bg_surface = IMG_Load_RW(bg_rw, 0);

    auto m_bg_texture = SDL_CreateTextureFromSurface(m_renderer, m_bg_surface);

    auto model = std::make_shared<GameModel>(std::filesystem::path(user_config_path) / "launcher.conf");
    auto view = std::make_shared<GameView>();
    auto presenter = std::make_shared<GamePresenter>(model, view, std::filesystem::path(user_config_path));

    view->SetBackgroundTexture(m_bg_texture, ImVec2(m_bg_surface->w, m_bg_surface->h));
    view->SetScaleFactor(DPIHandler::get_scale());

    m_running = true;
    while (m_running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) == 1) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                stop();
            }

            if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(m_window)) {
                on_event(event.window);
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        view->Update();
        view->Render();

        // Rendering
        ImGui::Render();

        SDL_RenderSetScale(m_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(m_renderer, 100, 100, 100, 255);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
        SDL_RenderPresent(m_renderer);
    }

    return m_exit_status;
}

void App::Application::stop()
{
    m_running = false;
}

void Application::on_event(const SDL_WindowEvent& event)
{
    switch (event.event) {
        case SDL_WINDOWEVENT_CLOSE:
            return on_close();
        case SDL_WINDOWEVENT_MINIMIZED:
            return on_minimize();
        case SDL_WINDOWEVENT_SHOWN:
            return on_shown();
        default:
            // Do nothing otherwise
            return;
    }
}

void Application::on_minimize()
{
    m_minimized = true;
}

void Application::on_shown()
{
    m_minimized = false;
}

void Application::on_close()
{
    stop();
}

} // namespace App
