#include "LauncherController.hpp"

#include "LauncherView.hpp"

#include <spdlog/spdlog.h>
#include <zoe.h>

LauncherController::LauncherController(SDL_Renderer* renderer, std::filesystem::path const& downloadResourcesDir,
                                       std::filesystem::path const& diabdatDir)
   : m_downloadResourcesDir(downloadResourcesDir)
   , m_diabdatDir(diabdatDir)
{
    zoe::Zoe::GlobalInit();

    m_zoeDownloader = std::make_unique<zoe::Zoe>();

    m_zoeDownloader->setThreadNum(1);
    m_zoeDownloader->setVerboseOutput([](const zoe::utf8string& verbose) { // Optional
        spdlog::info("{}", verbose.c_str());
    });

    m_view = std::make_unique<LauncherView>(renderer);
    m_mvvm = std::make_unique<LauncherMVVM>();

    m_mvvm->readyForDiablo = std::filesystem::exists(m_diabdatDir / "diabdat.mpq")
                             || std::filesystem::exists(m_diabdatDir / "DIABDAT.MPQ");

    using namespace std::string_literals;

    m_view->OnDemoClicked = [this]() {
        OnDemoButtonClicked();
    };
}

LauncherController::~LauncherController()
{}

LauncherController::ExitState LauncherController::Update()
{
    if (m_state) {
        return *m_state;
    }

    m_view->Update(*m_mvvm);

    return ExitState::kContinue;
}

void LauncherController::OnDemoButtonClicked()
{
    auto full_path = m_downloadResourcesDir / "spawn.mpq";
    if (std::filesystem::exists(full_path)) {
        m_state = ExitState::kLaunchDemo;
        return;
    }

    m_mvvm->downloadValue = 0.0f;
    m_mvvm->keepPopup = true;

    m_res = m_zoeDownloader->start(
       "https://github.com/diasurgical/devilutionx-assets/releases/download/v4/spawn.mpq",
       full_path.string(),
       [this](zoe::Result result) {
           if (result == zoe::Result::SUCCESSED) {
               m_mvvm->downloadValue = 1.0;
               m_mvvm->keepPopup = false;
               spdlog::info("downloaded : {}", m_mvvm->downloadValue);
           }
       },
       [this](int64_t total, int64_t downloaded) {
           m_mvvm->downloadValue = float(downloaded) / total;
           spdlog::info("downloaded : {}", m_mvvm->downloadValue);
       },
       nullptr);
    m_view->ShowDownloadingPopup();
}
