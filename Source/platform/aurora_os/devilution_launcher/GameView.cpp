#include "GameView.hpp"

#include <algorithm>
#include <cstdio>
#include <future>
#include <vector>

#include <SDL.h>
#include <thirdparty/FileBrowser.h>

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include "FontStorage.hpp"
#include "thirdparty/FileBrowser.h"

namespace {

template<class Functor>
struct ScopeGuard {
    ScopeGuard(Functor&& t)
       : func(std::move(t))
    {}
    ~ScopeGuard()
    {
        func();
    }

private:
    Functor func;
};
} // namespace

GameView::GameView()
{
    ApplyDiabloStyle();
    lastUpdateTime = std::chrono::steady_clock::now();

    fileBrowser = std::make_unique<ImGui::FileBrowser>(
       ImGuiFileBrowserFlags_Fullscreen | ImGuiFileBrowserFlags_NoResize | ImGuiFileBrowserFlags_NoMove
       | ImGuiFileBrowserFlags_NoTitleBar // Отключаем стандартный заголовок
    );
    fileBrowser->SetTypeFilters({"*.mpq"});
    fileBrowser->SetTitle("CHOOSE DIABDAT.MPQ");
    fileBrowser->SetMobileBehavior(true);
}

GameView::~GameView()
{}

void GameView::ApplyDiabloStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(scaleFactor);

    ImGui::GetIO().FontGlobalScale = scaleFactor;

    style.FrameRounding = Scale(4.0f);
    style.GrabRounding = Scale(4.0f);
    style.WindowRounding = Scale(6.0f);
    style.WindowBorderSize = Scale(0.0f);
    style.FrameBorderSize = Scale(1.0f);
    style.PopupBorderSize = Scale(1.0f);

    // Цветовая схема Diablo
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.86f, 0.77f, 0.62f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.02f, 0.02f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.35f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.38f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.35f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.80f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.00f, 0.00f, 1.00f);
    // colors[ImGuiCol_ProgressBarBg] = ImVec4(0.20f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.7f);
}

void GameView::Update()
{
    auto now = std::chrono::steady_clock::now();
    deltaTime = std::chrono::duration<float>(now - lastUpdateTime).count();
    lastUpdateTime = now;

    UpdateAnimations();

    {
        std::lock_guard lock(m_mtx);
        while (!m_renderThreadRunners.empty()) {
            std::invoke(m_renderThreadRunners.front());
            m_renderThreadRunners.pop_front();
        }
    }
}

void GameView::SetBackgroundTexture(void* texture, ImVec2 size)
{
    backgroundTexture = texture;
    backgroundTextureSize = size;
}

void GameView::SetScaleFactor(float scale)
{
    scaleFactor = scale;
    ApplyDiabloStyle();
}

void GameView::RequestInfoDialog()
{
    infoDialogRequested = true;
}

void GameView::RequestDownloadDialog()
{
    downloadRequested = true;
}

void GameView::RequestConfirmDownloadDialog()
{
    confirmDownloadRequested = true;
}

void GameView::SetDiabloEnabled(bool enabled)
{
    isDiabloAvailable = enabled;
}

void GameView::SetHellfireEnabled(bool enabled)
{
    isHellfireAvailable = enabled;
}

void GameView::SetDemoEnabled(bool enabled)
{
    isHellfireAvailable = enabled;
}

void GameView::ShowDownloadDialog(bool show)
{
    if (!show && ImGui::IsPopupOpen("Downloading Demo", ImGuiPopupFlags_AnyPopup)) {
        ImGui::ClosePopupsOverWindow(ImGui::GetCurrentWindow(), true);
    }

    downloadRequested = show;
    isDownloading = show;
    downloadProgress = 0.0f;
    downloadedSize = "0.0 KB";
    totalSize = "???.? KB";
}

void GameView::ShowConfirmDownloadDialog(bool show)
{
    confirmDownloadRequested = show;
    isConfirmAnimating = show;
    confirmAlpha = 0.0f;
}

void GameView::SetDownloadProgress(float progress, const std::string& downloaded, const std::string& total)
{
    downloadProgress = progress;
    downloadedSize = downloaded;
    totalSize = total;
}

void GameView::ShowInfoDialog(const std::string& message)
{}

void GameView::ShowFileDialog()
{}

void GameView::RunOnRenderThread(std::function<void()> task)
{
    std::lock_guard lock(m_mtx);
    m_renderThreadRunners.push_back(task);
}

void GameView::RunOnRenderThreadSync(std::function<void()> task)
{
    std::promise<void> promise;
    auto future = promise.get_future();

    {
        std::lock_guard lock(m_mtx);
        m_renderThreadRunners.push_back(task);

        m_renderThreadRunners.push_back([&promise]{
            promise.set_value();
        });
    }

    future.wait();
}

void GameView::UpdateAnimations()
{
    // Анимация Info
    if (isInfoAnimating) {
        if (infoAlpha < 1.0f) {
            infoAlpha = std::min(infoAlpha + deltaTime * 4.0f, 1.0f);
        } else {
            isInfoAnimating = false;
        }
    } else if (infoAlpha > 0.0f && !ImGui::IsPopupOpen("Game Information", ImGuiPopupFlags_AnyPopup)) {
        // Плавное исчезновение при закрытии
        infoAlpha = std::max(infoAlpha - deltaTime * 4.0f, 0.0f);
    }

    // Анимация Download
    if (isDownloadAnimating) {
        if (downloadAlpha < 1.0f) {
            downloadAlpha = std::min(downloadAlpha + deltaTime * 4.0f, 1.0f);
        } else {
            isDownloadAnimating = false;
        }
    } else if (downloadAlpha > 0.0f && !ImGui::IsPopupOpen("Downloading Demo", ImGuiPopupFlags_AnyPopup)) {
        downloadAlpha = std::max(downloadAlpha - deltaTime * 4.0f, 0.0f);
    }

    // Анимация Confirm
    if (isConfirmAnimating) {
        if (confirmAlpha < 1.0f) {
            confirmAlpha = std::min(confirmAlpha + deltaTime * 4.0f, 1.0f);
        } else {
            isConfirmAnimating = false;
        }
    } else if (confirmAlpha > 0.0f && !ImGui::IsPopupOpen("Confirm Download", ImGuiPopupFlags_AnyPopup)) {
        confirmAlpha = std::max(confirmAlpha - deltaTime * 4.0f, 0.0f);
    }

    // Автоматическое закрытие диалогов при alpha=0
    if (infoAlpha <= 0.0f && ImGui::IsPopupOpen("Game Information")) {
        ImGui::CloseCurrentPopup();
    }
    if (downloadAlpha <= 0.0f && ImGui::IsPopupOpen("Downloading Demo")) {
        ImGui::CloseCurrentPopup();
    }
    if (confirmAlpha <= 0.0f && ImGui::IsPopupOpen("Confirm Download")) {
        ImGui::CloseCurrentPopup();
    }
}

GameView::Orientation GameView::DetectOrientation() const
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    return (windowSize.y > windowSize.x) ? Orientation::Portrait : Orientation::Landscape;
}

void GameView::RenderButton(const char* label, bool enabled, std::function<void()> onClick, const ImVec2& size)
{
    const ImVec2 scaledSize = size;
    const float rounding = Scale(5.0f);
    const ImVec4 enabledColor = ImVec4(0.35f, 0.00f, 0.00f, 1.00f);
    const ImVec4 disabledColor = ImVec4(0.15f, 0.05f, 0.05f, 1.00f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);

    if (!enabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabledColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabledColor);
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    }

    if (ImGui::Button(label, scaledSize)) {
        onClick();
    }

    if (!enabled) {
        ImGui::PopItemFlag();
        ImGui::PopStyleColor(3);
    }

    ImGui::PopStyleVar();
}

void GameView::RenderBackgroundImage(const ImGuiViewport*& viewport)
{
    // auto viewportSize = viewport->Size;

    // // Calculate the aspect ratio of the image and the content region
    // float imageAspectRatio = backgroundTextureSize.x / backgroundTextureSize.y;
    // float contentRegionAspectRatio = viewportSize.x / viewportSize.y;

    // auto scaleX = viewportSize.x / backgroundTextureSize.x;
    // auto startU = (-0.5 * scaleX) + 0.5f;
    // auto endU = (0.5 * scaleX) + 0.5f;

    if (backgroundTexture) {
        ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)backgroundTexture,
                                                 viewport->WorkPos,
                                                 viewport->WorkSize,
                                                 ImVec2(0, 0),
                                                 ImVec2(1, 1));
    }
}
void GameView::Render()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus
                                    | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Diablo Launcher", nullptr, window_flags);

    // Отрисовка фонового изображения
    RenderBackgroundImage(viewport);

    // Затемнение фона для активных диалогов
    float maxAlpha = std::max({infoAlpha, downloadAlpha, confirmAlpha});
    if (maxAlpha > 0.0f) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(viewport->WorkPos,
                                                      viewport->WorkSize,
                                                      IM_COL32(0, 0, 0, static_cast<int>(200 * maxAlpha)));
    }

    RenderLogo();
    RenderVersionInfo();

    // Рендерим UI поверх затемнения
    RenderMainMenu();

    // Условный рендеринг диалогов
    RenderInfoPopup();
    RenderDownloadPopup();
    RenderConfirmDownloadDialog();

    RenderFilesDialog(); // Файловый диалог не использует alpha-анимацию

    ImGui::End();
    ImGui::PopStyleVar();
}

void GameView::RenderMainMenu()
{
    Orientation orientation = DetectOrientation();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Адаптивные размеры с ограничением максимального размера
    const float maxButtonHeight = Scale(70.0f); // Максимальная высота кнопки
    const float maxButtonWidth = Scale(300.0f); // Максимальная ширина кнопки

    // Рассчитываем базовые размеры
    const float refSize = std::sqrt(windowSize.x * windowSize.y);
    float buttonWidth = std::min(Scale(0.25f * refSize), maxButtonWidth);
    float buttonHeight = std::min(Scale(0.06f * refSize), maxButtonHeight);

    // Гарантируем минимальный размер
    buttonWidth = std::max(buttonWidth, Scale(120.0f));
    buttonHeight = std::max(buttonHeight, Scale(40.0f));

    const ImVec2 buttonSize(buttonWidth, buttonHeight);

    // Отступы
    const float verticalSpacing = std::min(Scale(0.015f * refSize), windowSize.y * 0.02f);
    const float buttonSpacing = verticalSpacing;
    const float bottomMargin = std::min(Scale(0.06f * refSize), windowSize.y * 0.25f);
    const float sideMargin = std::min(Scale(0.02f * refSize), windowSize.x * 0.03f);

    // Рассчитываем позицию меню
    const float menuHeight = (buttonSize.y * 5) + (verticalSpacing * 4);
    ImVec2 menuPosition;

    if (orientation == Orientation::Portrait) {
        // Центрирование по горизонтали
        menuPosition = ImVec2((windowSize.x - buttonSize.x) * 0.5f, windowSize.y - menuHeight - bottomMargin);
    } else {
        // Правый нижний угол
        menuPosition = ImVec2(windowSize.x - buttonSize.x - sideMargin, windowSize.y - menuHeight - bottomMargin);

        // Защита от выхода за пределы экрана
        if (menuPosition.x < 0) {
            menuPosition.x = 10;
        }
        if (menuPosition.y < 0) {
            menuPosition.y = 10;
        }
    }

    // Устанавливаем позицию
    ImGui::SetCursorPos(menuPosition);

    // Рендерим группу кнопок
    ImGui::BeginGroup();
    {
        // Diablo
        RenderButton(
           isDiabloAvailable ? "DIABLO" : "DIABLO (LOCKED)",
           isDiabloAvailable,
           [this] {
               if (onDiabloClicked) {
                   onDiabloClicked();
               }
           },
           buttonSize);

        ImGui::Dummy(ImVec2(0, buttonSpacing));

        // Hellfire
        // RenderButton(
        //    isHellfireAvailable ? "HELLFIRE" : "HELLFIRE (LOCKED)",
        //    isHellfireAvailable,
        //    [this] {
        //        if (onHellfireClicked) {
        //            onHellfireClicked();
        //        }
        //    },
        //    buttonSize);

        // ImGui::Dummy(ImVec2(0, buttonSpacing));

        // Demo
        RenderButton(
           "DEMO",
           true,
           [this] {
               if (onDemoClicked) {
                   onDemoClicked();
               }
           },
           buttonSize);

        ImGui::Dummy(ImVec2(0, buttonSpacing));

        // Files
        RenderButton(
           "FILES",
           true,
           [this] {
               filesDialogOpen = true;
               fileBrowser->Open();
           },
           buttonSize);

        ImGui::Dummy(ImVec2(0, buttonSpacing));

        // Info
        RenderButton(
           "INFO",
           true,
           [this] {
               infoDialogRequested = true;
               isInfoAnimating = true;
               infoAlpha = 0.0f;
           },
           buttonSize);
    }
    ImGui::EndGroup();

    // Отладочная информация (можно удалить после тестирования)
    if (showDebugInfo) {
        ImGui::SetCursorPos(ImVec2(10, 10));
        ImGui::Text("Window: %.0f x %.0f", windowSize.x, windowSize.y);
        ImGui::Text("Button: %.0f x %.0f", buttonSize.x, buttonSize.y);
        ImGui::Text("Menu Pos: %.0f, %.0f", menuPosition.x, menuPosition.y);
        ImGui::Text("Menu Height: %.0f", menuHeight);
    }
}

void GameView::RenderFilesDialog()
{
    if (!filesDialogOpen) {
        return;
    }

    fileBrowser->Display();

    if(fileBrowser->HasSelected()) {
        if(onPathSelected) {
            onPathSelected(fileBrowser->GetSelected());
        }
        fileBrowser->ClearSelected();
    }
}

void GameView::DrawCircularProgressBar(ImVec2 center, float radius, float thickness, float progress)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImU32 bgColor = ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 0.5f));

    // Динамический цвет - от красного к зеленому
    ImVec4 progressColor = ImVec4(0.8f - progress * 0.7f, 0.1f + progress * 0.7f, 0.1f, 1.0f);
    const ImU32 fgColor = ImGui::GetColorU32(progressColor);

    // Тень
    drawList->AddCircle(center, radius + 1.5f, IM_COL32(0, 0, 0, 150), 0, thickness + 1.5f);

    // Фоновый круг
    drawList->AddCircle(center, radius, bgColor, 0, thickness);

    // Прогресс в виде дуги
    const float angleMin = -IM_PI * 0.5f;
    const float angleMax = angleMin + 2.0f * IM_PI * progress;

    // Плавная анимация заполнения
    const int segments = 50 + static_cast<int>(radius * 0.5f);
    drawList->PathArcTo(center, radius, angleMin, angleMax, segments);
    drawList->PathStroke(fgColor, false, thickness);

    // Анимированная точка на конце
    if (progress > 0.01f && progress < 0.99f) {
        const float dotAngle = angleMax;
        const ImVec2 dotPos(center.x + cosf(dotAngle) * radius, center.y + sinf(dotAngle) * radius);
        drawList->AddCircleFilled(dotPos, thickness * 1.5f, fgColor);
    }
}

float GameView::Scale(float value) const
{
    return value * scaleFactor;
}

ImVec2 GameView::Scale(ImVec2 vec) const
{
    return ImVec2(vec.x * scaleFactor, vec.y * scaleFactor);
}

void GameView::RenderDownloadPopup()
{
    // if (!downloadRequested && !isDownloading) return;

    if (downloadRequested) {
        ImGui::OpenPopup("Downloading Demo");
        downloadRequested = false;
        isDownloadAnimating = true;
        downloadAlpha = 0.0f;
    }

    Orientation orientation = DetectOrientation();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Размеры окна
    ImVec2 popupSize(orientation == Orientation::Portrait ? windowSize.x * 0.85f : windowSize.x * 0.6f,
                     orientation == Orientation::Portrait ? windowSize.y * 0.6f : windowSize.y * 0.5f);

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(popupSize);

    // Стилизация
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scale(ImVec2(20, 20)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Scale(15.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, Scale(2.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.05f, 0.05f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, downloadAlpha);

    if (ImGui::BeginPopupModal("Downloading Demo",
                               nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // Фиксированный заголовок
        const float headerHeight = Scale(50.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();

        // Фон заголовка
        drawList->AddRectFilled(windowPos,
                                ImVec2(windowPos.x + popupSize.x, windowPos.y + headerHeight),
                                ImGui::GetColorU32(ImVec4(0.15f, 0.05f, 0.05f, 1.0f)));

        // Разделительная линия
        drawList->AddLine(ImVec2(windowPos.x, windowPos.y + headerHeight),
                          ImVec2(windowPos.x + popupSize.x, windowPos.y + headerHeight),
                          ImGui::GetColorU32(ImVec4(0.6f, 0.1f, 0.1f, 1.0f)),
                          2.0f);

        // Заголовок по центру
        const char* title = "DOWNLOADING DEMO";
        ImVec2 textSize = ImGui::CalcTextSize(title);
        ImVec2 textPos(windowPos.x + (popupSize.x - textSize.x) * 0.5f,
                       windowPos.y + (headerHeight - textSize.y) * 0.5f);
        drawList->AddText(textPos, ImGui::GetColorU32(ImVec4(0.86f, 0.77f, 0.62f, 1.0f)), title);

        // Основное содержимое
        ImGui::SetCursorPosY(headerHeight + Scale(30.0f));

        // Центр индикатора
        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        ImVec2 center(windowPos.x + popupSize.x * 0.5f, windowPos.y + headerHeight + contentAvail.y * 0.4f);

        // Радиус индикатора
        float radius = std::min(contentAvail.x, contentAvail.y) * 0.25f;
        radius = std::max(radius, Scale(30.0f));

        // Круговой индикатор прогресса
        DrawCircularProgressBar(center, radius, Scale(8.0f), downloadProgress);

        // Текст прогресса в центре круга
        char progressText[32];
        snprintf(progressText, sizeof(progressText), "%.0f%%", downloadProgress * 100);
        ImVec2 progressSize = ImGui::CalcTextSize(progressText);
        ImVec2 textCenter(center.x - progressSize.x * 0.5f, center.y - progressSize.y * 0.5f);
        drawList->AddText(textCenter, ImGui::GetColorU32(ImGuiCol_Text), progressText);

        // Информация о размерах файла
        ImGui::SetCursorPosY(headerHeight + contentAvail.y * 0.4f + radius + Scale(20.0f));

        char sizeText[64];
        snprintf(sizeText, sizeof(sizeText), "%s / %s", downloadedSize.c_str(), totalSize.c_str());
        ImVec2 sizeTextSize = ImGui::CalcTextSize(sizeText);
        ImGui::SetCursorPosX((popupSize.x - sizeTextSize.x) * 0.5f);
        ImGui::Text("%s", sizeText);

        // Статус загрузки
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale(10.0f));
        const char* statusText = downloadProgress < 1.0f ? "Downloading resources..." : "Verifying files...";
        ImVec2 statusSize = ImGui::CalcTextSize(statusText);
        ImGui::SetCursorPosX((popupSize.x - statusSize.x) * 0.5f);
        ImGui::Text("%s", statusText);

        // Кнопка отмены
        ImGui::SetCursorPosY(popupSize.y - Scale(60.0f));
        ImGui::SetCursorPosX((popupSize.x - Scale(150.0f)) * 0.5f);
        if (ImGui::Button("Cancel", ImVec2(Scale(150.0f), Scale(40.0f)))) {
            isDownloading = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Восстановление стилей
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

void GameView::RenderInfoPopup()
{
    // if (!infoDialogRequested && !isInfoAnimating) return;

    if (infoDialogRequested) {
        ImGui::OpenPopup("Game Information");
        infoDialogRequested = false;
        isInfoAnimating = true;
        infoAlpha = 0.0f;
        infoScrollY = 0.0f;
    }

    Orientation orientation = DetectOrientation();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Размеры окна совпадают с главным окном
    ImVec2 popupSize = windowSize;
    ImVec2 center = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(popupSize);

    // Стилизация
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.05f, 0.05f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, infoAlpha);

    if (ImGui::BeginPopupModal("Game Information",
                               nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // Фиксированный заголовок
        const float headerHeight = Scale(60.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();

        // Фон заголовка
        drawList->AddRectFilled(windowPos,
                                ImVec2(windowPos.x + popupSize.x, windowPos.y + headerHeight),
                                ImGui::GetColorU32(ImVec4(0.15f, 0.05f, 0.05f, 1.0f)));

        // Разделительная линия
        drawList->AddLine(ImVec2(windowPos.x, windowPos.y + headerHeight),
                          ImVec2(windowPos.x + popupSize.x, windowPos.y + headerHeight),
                          ImGui::GetColorU32(ImVec4(0.6f, 0.1f, 0.1f, 1.0f)),
                          2.0f);

        // Кнопка "назад" в левом верхнем углу
        const float buttonSize = Scale(40.0f);
        ImGui::SetCursorPos(ImVec2(Scale(10.0f), Scale(10.0f)));
        if (ImGui::Button((const char*)u8"", ImVec2(buttonSize, buttonSize))) {
            ImGui::CloseCurrentPopup();
            isInfoAnimating = false; // Начинаем анимацию закрытия
            infoAlpha = 1.0f;        // Для плавного исчезновения
        }

        // Заголовок по центру
        const char* title = "DIABLO LAUNCHER";
        ImVec2 textSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPos(ImVec2((popupSize.x - textSize.x) * 0.5f, (headerHeight - textSize.y) * 0.5f));
        ImGui::Text("%s", title);

        // Область с прокруткой (под заголовком)
        ImGui::SetCursorPosY(headerHeight + Scale(10.0f));
        float contentHeight = popupSize.y - headerHeight - Scale(60.0f);
        ImGui::BeginChild("InfoContent", ImVec2(popupSize.x - Scale(20.0f), contentHeight), true);

        // Обработка сенсорной прокрутки
        if (ImGui::IsWindowHovered()) {
            if (ImGui::IsMouseClicked(0)) {
                isInfoDragging = true;
                infoDragStartPos = ImGui::GetMousePos();
            }

            if (isInfoDragging && ImGui::IsMouseDragging(0)) {
                ImVec2 dragDelta = ImGui::GetMouseDragDelta(0);
                infoScrollY -= dragDelta.y;
                ImGui::ResetMouseDragDelta(0);
                infoScrollY = ImClamp(infoScrollY, 0.0f, ImGui::GetScrollMaxY());
            }

            if (ImGui::IsMouseReleased(0)) {
                isInfoDragging = false;
            }
        }

        // Применяем прокрутку
        ImGui::SetScrollY(infoScrollY);

        // Текст информации
        const char* infoText =
           "Diablo Launcher v1.0\n\n"
           "This is an open-source port of the classic game Diablo (1996) and its expansion Hellfire. "
           "Developed by fans for fans, this project aims to preserve and enhance the original experience.\n\n"
           "Features:\n"
           "• Modern rendering backend (DirectX 11, OpenGL, Vulkan)\n"
           "• High-resolution support (up to 4K)\n"
           "• Widescreen aspect ratio support\n"
           "• Quality of life improvements\n"
           "• Multi-platform support (Windows, macOS, Linux, Android, iOS)\n"
           "• Controller and touchscreen support\n\n"
           "Credits:\n"
           "• Original game by Blizzard North\n"
           "• DevilutionX development team\n"
           "• Open-source contributors worldwide\n\n"
           "Special Thanks:\n"
           "To all the Diablo fans who have kept this classic alive for over 25 years!";

        // Добавляем отступы по бокам
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scale(ImVec2(20, 20)));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(infoText);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleVar();

        // Фиктивный элемент для расширения области прокрутки
        ImGui::Dummy(ImVec2(0, Scale(20.0f)));
        ImGui::EndChild();

        // Кнопка закрытия внизу
        ImGui::SetCursorPosY(headerHeight + contentHeight + Scale(15.0f));
        ImGui::SetCursorPosX((popupSize.x - Scale(150.0f)) * 0.5f);
        if (ImGui::Button("Close", ImVec2(Scale(150.0f), Scale(40.0f)))) {
            ImGui::CloseCurrentPopup();
            isDownloadAnimating = false;
            downloadAlpha = 1.0f;
        }

        ImGui::EndPopup();
    }

    // Восстановление стилей
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

void GameView::RenderConfirmDownloadDialog()
{
    // if (!confirmDownloadRequested && !isConfirmAnimating) return;

    if (confirmDownloadRequested) {
        ImGui::OpenPopup("Confirm Download");
        confirmDownloadRequested = false;
        isConfirmAnimating = true;
        confirmAlpha = 0.0f;
    }

    Orientation orientation = DetectOrientation();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Размеры окна
    ImVec2 popupSize(orientation == Orientation::Portrait ? windowSize.x * 0.85f : windowSize.x * 0.6f,
                     orientation == Orientation::Portrait ? windowSize.y * 0.4f : windowSize.y * 0.35f);

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(popupSize);

    // Стилизация
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Scale(15.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, Scale(2.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.05f, 0.05f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, confirmAlpha);

    if (ImGui::BeginPopupModal("Confirm Download",
                               nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // Фиксированный заголовок
        const float headerHeight = Scale(50.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();

        // Фон заголовка
        drawList->AddRectFilled(windowPos,
                                ImVec2(windowPos.x + popupSize.x, windowPos.y + headerHeight),
                                ImGui::GetColorU32(ImVec4(0.15f, 0.05f, 0.05f, 1.0f)));

        // Разделительная линия
        drawList->AddLine(ImVec2(windowPos.x, windowPos.y + headerHeight),
                          ImVec2(windowPos.x + popupSize.x, windowPos.y + headerHeight),
                          ImGui::GetColorU32(ImVec4(0.6f, 0.1f, 0.1f, 1.0f)),
                          2.0f);

        // Заголовок по центру
        const char* title = "DOWNLOAD DEMO";
        ImVec2 textSize = ImGui::CalcTextSize(title);
        ImVec2 textPos(windowPos.x + (popupSize.x - textSize.x) * 0.5f,
                       windowPos.y + (headerHeight - textSize.y) * 0.5f);
        drawList->AddText(textPos, ImGui::GetColorU32(ImVec4(0.86f, 0.77f, 0.62f, 1.0f)), title);

        // Основное содержимое
        ImGui::SetCursorPosY(headerHeight + Scale(20.0f));
        ImGui::SetCursorPosX(Scale(20.0f));

        // Основное сообщение с переносом
        const char* message =
           "The demo version will be downloaded from the internet. The download size is approximately 250 "
           "MB.";
        ImGui::PushTextWrapPos(popupSize.x - Scale(40.0f));
        ImGui::TextWrapped("%s", message);
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Spacing();

        // Предупреждение с переносом
        const char* note = "Note: A stable internet connection is required.";
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
        ImGui::PushTextWrapPos(popupSize.x - Scale(40.0f));
        ImGui::TextWrapped("%s", note);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        // Кнопки
        ImGui::SetCursorPosY(popupSize.y - Scale(60.0f));

        // Рассчитываем размер кнопок с ограничением
        const float maxButtonWidth = Scale(200.0f);
        float buttonWidth = (popupSize.x - Scale(50.0f)) / 2;
        buttonWidth = std::min(buttonWidth, maxButtonWidth);

        // Центрируем кнопки
        float buttonsTotalWidth = buttonWidth * 2 + Scale(10.0f);
        float startX = (popupSize.x - buttonsTotalWidth) * 0.5f;

        ImGui::SetCursorPosX(startX);

        // Кнопка "Download"
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.7f, 0.4f, 1.0f));
        if (ImGui::Button("Download", ImVec2(buttonWidth, Scale(40.0f)))) {
            ImGui::CloseCurrentPopup();
            confirmDownloadDialogOpen = false;
            onDownloadClicked();
            // downloadRequested = true;
            // isDownloading = true;
            // downloadProgress = 0.0f;
            // downloadedSize = "0.0 MB";
            // totalSize = "250.0 MB";
        }
        ImGui::PopStyleColor(3);

        // Кнопка "Cancel"
        ImGui::SameLine(0, Scale(10.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, Scale(40.0f)))) {
            ImGui::CloseCurrentPopup();
            confirmDownloadDialogOpen = false;
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }

    // Восстановление стилей
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

void GameView::RenderLogo()
{
    Orientation orientation = DetectOrientation();
    ImVec2 windowSize = ImGui::GetWindowSize();

    ImFont* font = FontStorage::getFont("diabloFont");
    ImGui::PushFont(font);
    ScopeGuard popFont = []() {
        ImGui::PopFont();
    };

    // Параметры для разных ориентаций
    float fontSize;
    const char* logoText = "DEVILUTIONX";

    if (orientation == Orientation::Portrait) {
        fontSize = Scale(36.0f);
    } else {
        fontSize = Scale(42.0f); // Увеличенный размер для альбомной
    }

    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, logoText);

    // Позиция лого
    ImVec2 logoPos;
    if (orientation == Orientation::Portrait) {
        logoPos = ImVec2((windowSize.x - textSize.x) * 0.5f, windowSize.y * 0.08f);
    } else {
        logoPos = ImVec2(Scale(30.0f), Scale(20.0f));
    }

    // Стилизованный рендер текстового лого
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Эффект металлического текста
    const ImU32 shadowColor = IM_COL32(80, 0, 0, 255);
    const ImU32 mainColor = IM_COL32(180, 120, 60, 255);
    const ImU32 highlightColor = IM_COL32(220, 180, 100, 255);

    // Тень (смещение вниз и вправо)
    drawList->AddText(font, fontSize, ImVec2(logoPos.x + Scale(3.0f), logoPos.y + Scale(3.0f)), shadowColor, logoText);

    // Основной текст
    drawList->AddText(font, fontSize, logoPos, mainColor, logoText);

    // Блики (верхние части букв)
    for (int i = 0; i < 3; i++) {
        drawList->AddText(font, fontSize, ImVec2(logoPos.x, logoPos.y - Scale(i * 0.7f)), highlightColor, logoText);
    }
}

void GameView::RenderVersionInfo()
{
    ImFont* font = FontStorage::getFont("diabloFont");
    ImGui::PushFont(font);
    ScopeGuard popFont = []() {
        ImGui::PopFont();
    };

    Orientation orientation = DetectOrientation();
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Версия приложения
    std::string version = "v" + versionText;
    float fontSize;
    ImVec2 textPos;

    if (orientation == Orientation::Portrait) {
        fontSize = Scale(18.0f);
        ImVec2 logoSize = font->CalcTextSizeA(Scale(36.0f), FLT_MAX, 0.0f, "DEVILUTIONX");
        textPos = ImVec2((windowSize.x - font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, version.c_str()).x) * 0.5f,
                         windowSize.y * 0.08f + Scale(45.0f));
    } else {
        fontSize = Scale(18.0f);
        ImVec2 logoSize = font->CalcTextSizeA(Scale(42.0f), FLT_MAX, 0.0f, "DEVILUTIONX");
        textPos = ImVec2(Scale(30.0f) + logoSize.x + Scale(15.0f), Scale(25.0f));
    }

    // Стиль текста версии
    const ImU32 versionColor = ImGui::GetColorU32(ImVec4(0.86f, 0.77f, 0.62f, 0.7f));

    // Рендер текста с тенью
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Тень
    drawList->AddText(font, fontSize, ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 150), version.c_str());

    // Основной текст
    drawList->AddText(font, fontSize, textPos, versionColor, version.c_str());
}
