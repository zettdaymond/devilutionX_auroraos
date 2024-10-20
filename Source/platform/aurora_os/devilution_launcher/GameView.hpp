#pragma once

#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <filesystem>

#include <deque>
#include <imgui.h>
#include <mutex>

class GameModel;
class GamePresenter;

namespace ImGui {
class FileBrowser;
}

class ILauncherView
{
public:
    // Колбэки для Presenter'а
    std::function<void()> onDiabloClicked;
    std::function<void()> onHellfireClicked;
    std::function<void()> onDemoClicked;
    std::function<void()> onDownloadClicked;
    std::function<void()> onFilesClicked;
    std::function<void()> onInfoClicked;
    std::function<void(const std::filesystem::path&)> onPathSelected;

    // Методы обновления состояния
    virtual void SetDiabloEnabled(bool enabled) = 0;
    virtual void SetHellfireEnabled(bool enabled) = 0;
    virtual void SetDemoEnabled(bool enabled) = 0;
    virtual void ShowDownloadDialog(bool show) = 0;
    virtual void ShowConfirmDownloadDialog(bool show) = 0;
    virtual void SetDownloadProgress(float progress, const std::string& downloaded, const std::string& total) = 0;
    virtual void ShowInfoDialog(const std::string& message) = 0;
    virtual void ShowFileDialog() = 0;
    virtual void RunOnRenderThread(std::function<void()> task) = 0;
    virtual void RunOnRenderThreadSync(std::function<void ()> task) = 0;
};

class GameView : public ILauncherView {
public:
    GameView();
    ~GameView();

    void RenderBackgroundImage(const ImGuiViewport*& viewport);
    void Render();
    void Update();
    void SetBackgroundTexture(void* texture, ImVec2 size);
    void SetScaleFactor(float scale);

    // Состояния для ViewModel
    void RequestInfoDialog();
    void RequestDownloadDialog();
    void RequestConfirmDownloadDialog();

    // Методы обновления состояния
    void SetDiabloEnabled(bool enabled) override;
    void SetHellfireEnabled(bool enabled) override;
    void SetDemoEnabled(bool enabled) override;
    void ShowDownloadDialog(bool show) override;
    void ShowConfirmDownloadDialog(bool show) override;
    void SetDownloadProgress(float progress, const std::string& downloaded, const std::string& total) override;
    void ShowInfoDialog(const std::string& message) override;
    void ShowFileDialog() override;

    void RunOnRenderThread(std::function<void()> task) override;
    void RunOnRenderThreadSync(std::function<void ()> task) override;
private:
    // Ориентация
    enum class Orientation {
        Portrait,
        Landscape,
    };

    // Элементы интерфейса
    void RenderMainMenu();
    void RenderFilesDialog();
    void RenderDownloadPopup();
    void RenderInfoPopup();
    void RenderConfirmDownloadDialog();
    void ApplyDiabloStyle();
    void UpdateAnimations();
    void DrawCircularProgressBar(ImVec2 center, float radius, float thickness, float progress);
    auto DetectOrientation() const -> Orientation;
    void RenderVersionInfo();
    void RenderLogo();

    // Вспомогательные методы
    float Scale(float value) const;
    ImVec2 Scale(ImVec2 vec) const;
    void RenderButton(const char* label, bool enabled, std::function<void()> onClick, const ImVec2& size);

    void* backgroundTexture = nullptr;
    ImVec2 backgroundTextureSize = ImVec2{0, 0};
    float scaleFactor = 1.0f;

    // Состояния UI
    bool filesDialogOpen = false;
    bool infoDialogRequested = false;
    bool downloadRequested = false;
    bool confirmDownloadRequested = false;
    bool isDemoAvailable = false;
    bool isDiabloAvailable = false;
    bool isHellfireAvailable = false;
    bool isDownloading = false;

    // Анимации
    float infoAlpha = 0.0f;
    float downloadAlpha = 0.0f;
    float confirmAlpha = 0.0f;
    bool isInfoAnimating = false;
    bool isDownloadAnimating = false;
    bool isConfirmAnimating = false;

    // Прогресс загрузки
    float downloadProgress = 0.0f;
    std::string downloadedSize = "0.0 MB";
    std::string totalSize = "0.0 MB";

    // Время для анимаций
    std::chrono::steady_clock::time_point lastUpdateTime = std::chrono::steady_clock::now();
    float deltaTime = 0.0f;

    // Прокрутка для Info
    float infoScrollY = 0.0f;
    bool isInfoDragging = false;
    ImVec2 infoDragStartPos;
    bool confirmDownloadDialogOpen = false;

    bool showDebugInfo = false;
    std::unique_ptr<ImGui::FileBrowser> fileBrowser;

    std::string versionText = "1.5.0";

    std::deque<std::function<void()>>  m_renderThreadRunners;
    std::mutex m_mtx;
};
