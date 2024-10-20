#pragma once

#include <SDL2/SDL_render.h>

#include "zoe.h"

#include <filesystem>
#include <memory>
#include <optional>

class LauncherView;
struct LauncherMVVM;

class LauncherController {
public:
    enum class ExitState {
        kLaunchDemo,
        kLaunchDiablo,
        kContinue,
    };

    LauncherController(SDL_Renderer* renderer, const std::filesystem::path& downloadResourcesDir);
    ~LauncherController();

    ExitState Update();

private:
    void OnDemoButtonClicked();
    void OnResounrceSelected(std::filesystem::path const& path);

    std::filesystem::path m_downloadResourcesDir;
    std::optional<std::filesystem::path> m_diabdatDir;

    std::unique_ptr<class LauncherSettings> m_settings;
    std::unique_ptr<zoe::Zoe> m_zoeDownloader;
    std::shared_future<zoe::Result> m_res;
    std::unique_ptr<LauncherView> m_view;
    std::unique_ptr<LauncherMVVM> m_mvvm;

    std::optional<ExitState> m_state;
};
