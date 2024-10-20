#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace zoe {
class Zoe;
}

class ILauncherView;
class GameModel;

class GamePresenter {
public:
    // Привязка к модели
    GamePresenter(std::shared_ptr<GameModel> model, std::shared_ptr<ILauncherView> view,
                  const std::filesystem::path& downloadResourcesDir);

    ~GamePresenter();

    // Обработчики событий от View
    void HandleDiabloClick();
    void HandleHellfireClick();
    void HandleDemoClick();
    void HandleDownloadClick();
    void HandleCancelDownloadClick();
    void HandleFilesClick();
    void HandleInfoClick();
    void HandlePathSelected(const std::filesystem::path& path);

private:
    enum class Status {
        kRunning,
        kLaunchDemo,
        kLaunchDiablo,
    };

    std::shared_ptr<GameModel> m_model;
    std::shared_ptr<ILauncherView> m_view;
    std::filesystem::path m_downloadResourcesDir;

    std::unique_ptr<zoe::Zoe> m_zoeDownloader;

    Status m_state = Status::kRunning;

    bool showFilesDialog = false;
    bool showInfoDialog = false;
    bool showDownloadDialog = false;
};
