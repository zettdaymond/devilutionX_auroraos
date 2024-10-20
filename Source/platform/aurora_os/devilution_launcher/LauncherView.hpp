#pragma once

#include "SDL_render.h"

#include <filesystem>
#include <functional>
#include <memory>

namespace ImGui {
class FileBrowser;
}

struct LauncherMVVM
{
    bool readyForDiablo = false;
    float downloadValue = 0.0f;
    bool keepPopup = true;
};

class LauncherView
{
public:
    LauncherView(SDL_Renderer* renderer);
    ~LauncherView();

    void FillPopupContent(const LauncherMVVM &mvvm);
    void Update(const LauncherMVVM& mvvm);

    void ShowDownloadingPopup();
    void CloseDownloadingPopup();

    std::function<void()> OnDiabloClicked;
    std::function<void()> OnDemoClicked;
    std::function<void()> OnFolderClicked;
    std::function<void()> OnInfoClicked;
    std::function<void(std::filesystem::path)> OnResourceSelected;

private:
    SDL_Renderer* m_renderer{nullptr};
    SDL_Surface* m_bg_surface{nullptr};
    SDL_Texture* m_bg_texture{nullptr};

    std::unique_ptr<ImGui::FileBrowser> m_file_browser;
};
