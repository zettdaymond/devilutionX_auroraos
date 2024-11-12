#include "LauncherController.hpp"

#include "LauncherView.hpp"

#include <spdlog/spdlog.h>
#include <zoe.h>

#include <fstream>

class LauncherSettings {
public:
    LauncherSettings(std::filesystem::path const& path_to_file)
       : m_path_to_file(path_to_file)
    {
        if (std::filesystem::exists(path_to_file)) {
            try {
                std::ifstream fs(path_to_file);

                std::filesystem::path path;
                fs >> path;

                fs.close();

                m_user_defined_path = path;
            } catch (std::exception const& e) {
                spdlog::error("Error during file read : {}", e.what());
                try {
                    std::filesystem::remove(path_to_file);
                } catch (const std::exception& e) {
                    spdlog::error("Error during file remove : {} : {}", path_to_file.string(), e.what());
                }
            }
        }
    }

    auto GetUserDifinedPath() const -> std::optional<std::filesystem::path>
    {
        return m_user_defined_path;
    }

    void SetUserDifinedPath(std::filesystem::path const& path)
    {
        if (std::filesystem::exists(m_path_to_file)) {
            try {
                std::filesystem::remove(m_path_to_file);
            } catch (const std::exception& e) {
                spdlog::error("Error during file remove : {} : {}", m_path_to_file.string(), e.what());
            }
        }

        try {
            std::ofstream fs(m_path_to_file);
            fs << path;

            fs.close();
        } catch (std::exception const& e) {
            spdlog::error("Error during file write : {}", e.what());
        }

        m_user_defined_path = path;
    }

private:
    std::filesystem::path m_path_to_file;
    std::optional<std::filesystem::path> m_user_defined_path;
};

LauncherController::LauncherController(SDL_Renderer* renderer, std::filesystem::path const& downloadResourcesDir)
   : m_downloadResourcesDir(downloadResourcesDir)
{
    m_settings = std::make_unique<LauncherSettings>(downloadResourcesDir / "launcher.conf");
    zoe::Zoe::GlobalInit();

    m_zoeDownloader = std::make_unique<zoe::Zoe>();

    m_zoeDownloader->setThreadNum(1);
    m_zoeDownloader->setVerboseOutput([](const zoe::utf8string& verbose) { // Optional
        spdlog::info("{}", verbose.c_str());
    });

    m_view = std::make_unique<LauncherView>(renderer);
    m_mvvm = std::make_unique<LauncherMVVM>();

    auto diabdat_dir = m_settings->GetUserDifinedPath();
    m_mvvm->readyForDiablo = diabdat_dir ? (std::filesystem::exists(*diabdat_dir / "diabdat.mpq")
                                            || std::filesystem::exists(*diabdat_dir / "DIABDAT.MPQ"))
                                         : false;

    using namespace std::string_literals;

    m_view->OnDemoClicked = [this]() {
        OnDemoButtonClicked();
    };

    m_view->OnFolderClicked = [this]() {
        m_view->ShowFileDialogPopup();
    };

    m_view->OnResourceSelected = [this](const auto& path) {
        OnResounrceSelected(path);
    };
}

LauncherController::~LauncherController()
{
    m_view = nullptr;

    m_zoeDownloader->stop();
    m_zoeDownloader = nullptr;

    zoe::Zoe::GlobalUnInit();
}

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

void LauncherController::OnResounrceSelected(const std::filesystem::path& path)
{
    if (std::filesystem::exists(path) && (path.filename() == "diabdat.mpq" || path.filename() == "DIABDAT.MPQ")) {
        const auto directory_path = path.parent_path();

        spdlog::info("User selected directory : {}", directory_path.string());

        m_mvvm->readyForDiablo = true;
        m_settings->SetUserDifinedPath(directory_path);
    }
}
