#include "LauncherView.hpp"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"

#include <imgui.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rwops.h>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(assets);

#include "widgets/imgui/ImguiExtension.h"

#include "DPIHandler.hpp"

#include "thirdparty/FileBrowser.h"

namespace {

auto Tr(std::u8string_view str)
{
    return reinterpret_cast<const char*>(str.data());
}

ImVec2 MainButtonSize()
{
    return ImVec2{160 * App::DPIHandler::get_scale(), 40 * App::DPIHandler::get_scale()};
}

ImVec2 SecondaryButtonSize()
{
    return MainButtonSize();
}

bool MainButton(std::u8string_view label)
{
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF02025C);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF02025C);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF00003B);

    auto ret = ImGui::Button(Tr(label), MainButtonSize());

    ImGui::PopStyleColor(3);

    return ret;
}

bool SecondaryButton(std::u8string_view label)
{
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF111314);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF111314);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF0C0C0C);

    auto ret = ImGui::Button(Tr(label), SecondaryButtonSize());

    ImGui::PopStyleColor(3);

    return ret;
}

ImVec2 GetButtonsBlockSize()
{
    ImGuiStyle& style = ImGui::GetStyle();
    float buttonSizeX = MainButtonSize().x + style.FramePadding.x * 2.0f;
    float buttonGroupSizeY = (MainButtonSize().y + style.FramePadding.y * 2.0f + 16.0f) * 4.0;

    return {buttonSizeX, buttonGroupSizeY};
}

bool DialogButton(std::u8string_view label)
{
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF02025C);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF02025C);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF00003B);

    auto ret = ImGui::SmallButton(Tr(label));

    ImGui::PopStyleColor(3);

    return ret;
}

} // namespace

LauncherView::LauncherView(SDL_Renderer* renderer)
   : m_renderer(renderer)
{
    auto fs = cmrc::assets::get_filesystem();

    ImGuiIO& io{ImGui::GetIO()};

    auto bg = fs.open("assets/bg.png");
    auto bg_rw = SDL_RWFromMem((void*)bg.begin(), bg.size());
    m_bg_surface = IMG_Load_RW(bg_rw, 0);

    m_bg_texture = SDL_CreateTextureFromSurface(m_renderer, m_bg_surface);

    m_file_browser = std::make_unique<ImGui::FileBrowser>();
    m_file_browser->SetTypeFilters({".mpq", ".MPQ"});
    m_file_browser->SetTitle("Choose diabdat.mpq");
    m_file_browser->SetWindowPos(0, 0);
}

LauncherView::~LauncherView()
{}

void LauncherView::FillPopupContent(const LauncherMVVM& mvvm, ImVec2 size)
{
    const float scaling_factor{App::DPIHandler::get_scale()};

    const auto downloadingBarHeight = 8 * scaling_factor;
    const auto spacingHeight = 4;
    auto textHeight = ImGui::CalcTextSize(Tr(u8"Зазрузка демо ресурсов...")).y;

    const ImU32 circularColor = 0xffffffff;
    const ImU32 col = 0xFF00003B;
    const ImU32 bg = 0xFF0C0C0C;

    const auto leftMargin = std::min(size.x * 0.1, 32.0) * scaling_factor;
    const auto contentWidth = size.x - (leftMargin * 2);
    const auto contentHeight = textHeight + downloadingBarHeight + spacingHeight;
    const auto topMargin = (size.y - contentHeight) * 0.65;

    ImGui::Dummy(ImVec2(0, topMargin * 0.5));

    {
        ImGui::BeginGroup();

        ImGui::Dummy(ImVec2(leftMargin, 0));
        ImGui::SameLine();
        ImGui::Text("%s", Tr(u8"Зазрузка демо ресурсов..."));

        ImGui::Dummy(ImVec2(0, 4 * scaling_factor));

        ImGui::Dummy(ImVec2(leftMargin, 0));
        ImGui::SameLine();
        ImGui::BufferingBar("##buffer_bar", mvvm.downloadValue, ImVec2(contentWidth, 7 * scaling_factor), bg, col);

        ImGui::EndGroup();
    }

    const auto cancelMargin = 16.0;
    const auto cancelSizes = ImGui::CalcTextSize(Tr(u8"Отмена"));

    ImGui::SetCursorPosX(size.x - cancelSizes.x - cancelMargin * 2.0);
    ImGui::SetCursorPosY(size.y - cancelSizes.y - cancelMargin);

    if (DialogButton(u8"Отмена")) {
        if (OnDownloadCanceled) {
            OnDownloadCanceled();
        }
    }
}

void LauncherView::DisplayDownloadPopup(const LauncherMVVM& mvvm, auto win_flags)
{
    const float scaling_factor{App::DPIHandler::get_scale()};

    auto winSize = ImGui::GetMainViewport()->Size;

    bool verticalViewport = winSize.x < winSize.y;

    ImVec2 size;
    if (verticalViewport) {
        size.x = winSize.x * 0.95;
        size.y = size.x / 16.0 * 9.0;
    } else {
        size.x = winSize.x * 0.3;
        size.y = size.x / 16.0 * 9.0;
    }

    size.x = std::min(size.x, 320.0f * scaling_factor);
    size.y = std::min(size.y, 180.0f * scaling_factor);

    ImVec2 pos = {
       (winSize.x - size.x) * 0.5f,
       (winSize.y - size.y) * 0.5f,
    };
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::BeginPopupModal("ThePopup", nullptr, win_flags)) {
        ImGui::PopStyleVar();
        return;
    }

    FillPopupContent(mvvm, size);
    ImGui::EndPopup();

    ImGui::PopStyleVar();
}

void LauncherView::Update(const LauncherMVVM& mvvm)
{
    auto win_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                     | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav;

    int w, h;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);

    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    if (auto window = ImGui::Begin("Window", nullptr, win_flags)) {
        auto viewportSize = ImGui::GetContentRegionAvail();

        // Calculate the aspect ratio of the image and the content region
        float imageAspectRatio = m_bg_surface->w / m_bg_surface->h;
        float contentRegionAspectRatio = viewportSize.x / viewportSize.y;

        auto scaleX = viewportSize.x / m_bg_surface->w;
        auto startU = (-0.5 * scaleX) + 0.5f;
        auto endU = (0.5 * scaleX) + 0.5f;

        m_file_browser->SetWindowSize(ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y);

        ImGui::Image((ImTextureID)(intptr_t)m_bg_texture,
                     ImVec2(viewportSize.x, viewportSize.y),
                     ImVec2(startU, 0),
                     ImVec2(endU, 1));

        auto [buttonSizeX, buttonGroupSizeY] = GetButtonsBlockSize();

        float windowAvailableX = ImGui::GetContentRegionAvail().x;
        float windowAvailableY = viewportSize.y;

        ImGui::SetCursorPosY((windowAvailableY - buttonGroupSizeY) * 0.90f);

        float buttonsOffsetX = (windowAvailableX - buttonSizeX) * 0.5f;
        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (mvvm.readyForDiablo) {
            if (MainButton(u8"DIABLO")) {
                if (OnDiabloClicked) {
                    OnDiabloClicked();
                }
            }
        } else {
            if (SecondaryButton(u8"DIABLO")) {
                // do nothing
            }
        }

        ImGui::Dummy(ImVec2(0, 16));

        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (SecondaryButton(u8"Папка")) {
            m_file_browser->Open();
            if (OnFolderClicked) {
                OnFolderClicked();
            }
        }

        ImGui::Dummy(ImVec2(0, 16));

        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (SecondaryButton(u8"DEMO")) {
            if (OnDemoClicked) {
                OnDemoClicked();
            }
        }

        ImGui::Dummy(ImVec2(0, 16));

        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (SecondaryButton(u8"INFO")) {
            if (OnFolderClicked) {
                OnFolderClicked();
            }
        }

        DisplayDownloadPopup(mvvm, win_flags);

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF02025C);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF02025C);
        ImGui::PushStyleColor(ImGuiCol_Button, 0xFF00003B);

        m_file_browser->Display();

        ImGui::PopStyleColor(3);

        if (m_file_browser->HasSelected()) {
            const auto path = m_file_browser->GetSelected();

            spdlog::info("Selected file path: {}", path.string());
            m_file_browser->ClearSelected();

            if (OnResourceSelected) {
                OnResourceSelected(path);
            }
        }

        ImGui::End();
    }
}

void LauncherView::ShowDownloadingPopup()
{
    ImGui::OpenPopup("ThePopup");
}

void LauncherView::CloseDownloadingPopup()
{
    ImGui::CloseCurrentPopup();
}

void LauncherView::ShowFileDialogPopup()
{
    m_file_browser->Open();
}
