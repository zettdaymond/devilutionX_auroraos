#include "GamePresenter.hpp"

#include "GameModel.hpp"
#include "GameView.hpp"

#include <spdlog/spdlog.h>

#include <zoe/zoe.h>

GamePresenter::GamePresenter(std::shared_ptr<GameModel> model, std::shared_ptr<ILauncherView> view,
                             std::filesystem::path const& downloadResourcesDir)
   : m_model(model)
   , m_view(view)
   , m_downloadResourcesDir(downloadResourcesDir)
{
    m_view->onDiabloClicked = [this] {
        HandleDiabloClick();
    };
    m_view->onHellfireClicked = [this] {
        HandleHellfireClick();
    };
    m_view->onDemoClicked = [this] {
        HandleDemoClick();
    };
    m_view->onDownloadClicked = [this] {
        HandleDownloadClick();
    };
    m_view->onFilesClicked = [this] {
        HandleFilesClick();
    };
    m_view->onInfoClicked = [this] {
        HandleInfoClick();
    };
    m_view->onPathSelected = [this](auto path) {
        HandlePathSelected(path);
    };

    zoe::Zoe::GlobalInit();

    m_zoeDownloader = std::make_unique<zoe::Zoe>();
    m_zoeDownloader->setThreadNum(1);

    if(auto mpqPath = m_model->GetDiabloResourcesPath()) {
        if(std::filesystem::exists(*mpqPath)) {
            m_view->SetDiabloEnabled(true);
        }
    }
}

GamePresenter::~GamePresenter()
{
    zoe::Zoe::GlobalUnInit();

    m_view->onDiabloClicked = nullptr;
    m_view->onHellfireClicked = nullptr;
    m_view->onDemoClicked = nullptr;
    m_view->onDownloadClicked = nullptr;
    m_view->onFilesClicked = nullptr;
    m_view->onInfoClicked = nullptr;
    m_view->onPathSelected = nullptr;
}

void GamePresenter::HandleDiabloClick()
{}

void GamePresenter::HandleHellfireClick()
{}

void GamePresenter::HandleDemoClick()
{
    auto full_path = m_model->GetDemoResourcesPath();
    if (full_path && std::filesystem::exists(*full_path)) {
        m_state = Status::kLaunchDemo;
        return;
    }

    m_view->ShowConfirmDownloadDialog(true);
}

void GamePresenter::HandleDownloadClick()
{
    auto full_path = m_model->GetDemoResourcesPath();
    if (full_path && std::filesystem::exists(*full_path)) {
        m_state = Status::kLaunchDemo;
        return;
    }

    full_path = m_downloadResourcesDir / "spawn.mpq";

    auto res = m_zoeDownloader->start(
       "https://github.com/diasurgical/devilutionx-assets/releases/download/v4/spawn.mpq",
       full_path->string(),
       // nullptr,
       [this, full_path](zoe::ZoeResult result) {
           if (result == zoe::ZoeResult::SUCCESSED) {
               spdlog::info("file spawn.mpq successfully downloaded : {}", full_path->string());

               m_model->SetDemoResourcesPath(full_path->string());

               m_view->RunOnRenderThreadSync([this] {
                   m_view->SetDemoEnabled(true);
                   m_view->ShowDownloadDialog(false);
               });
           }
       },
       [this](int64_t total, int64_t downloaded) {
           auto downloadProgres = float(downloaded) / total;

           spdlog::info("downloaded : {} of {}", downloaded, total);

           m_view->RunOnRenderThreadSync([this, downloaded, downloadProgres, total] {
               m_view->SetDownloadProgress(downloadProgres,
                                           std::to_string(downloaded / 1'024) + " KB",
                                           std::to_string(total / 1'024) + " KB");
           });
       },
       nullptr);

    m_view->ShowDownloadDialog(true);
}

void GamePresenter::HandleCancelDownloadClick()
{}

void GamePresenter::HandleFilesClick()
{
    m_view->ShowFileDialog();
}

void GamePresenter::HandleInfoClick()
{
    m_view->ShowInfoDialog("");
}

void GamePresenter::HandlePathSelected(const std::filesystem::path& path)
{
    if (std::filesystem::exists(path)) {
        auto dirPath = path.parent_path();
        m_model->SetDiabloResourcesPath(dirPath.string());
        m_view->SetDiabloEnabled(true);
    }
}
