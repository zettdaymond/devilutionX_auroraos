#pragma once

#include <SDL2/SDL.h>

#include <memory>
#include <string>
#include <vector>

namespace App {

enum class ExitStatus : int {
    SUCCESS = 0,
    FAILURE = 1,
};

class Application {
public:
    explicit Application(SDL_Window* window, std::string const& company_namespace, std::string const& app_name);
    ~Application();

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application other) = delete;
    Application& operator=(Application&& other) = delete;

    ExitStatus run();
    void stop();

    void on_event(const SDL_WindowEvent& event);
    void on_minimize();
    void on_shown();
    void on_close();

private:
    ExitStatus m_exit_status{ExitStatus::SUCCESS};
    SDL_Window* m_window{nullptr};
    std::string m_company_namespace;
    std::string m_app_name;

    SDL_Renderer* m_renderer{nullptr};

    bool m_running{true};
    bool m_minimized{false};
    bool m_show_some_panel{true};
    bool m_show_debug_panel{false};
    bool m_show_demo_panel{false};
};

} // namespace App
