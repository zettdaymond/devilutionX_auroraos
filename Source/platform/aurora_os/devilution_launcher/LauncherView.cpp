#include "LauncherView.hpp"
#include "imgui_internal.h"
#include "spdlog/spdlog.h"

#include <imgui.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rwops.h>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(assets);

#include "widgets/imgui/ImguiExtension.h"

#include <imfilebrowser.h>

namespace {
ImVec2 MainButtonSize()
{
    return ImVec2{380, 80};
}

ImVec2 SecondaryButtonSize()
{
    return MainButtonSize();
}

bool MainButton(std::string_view label)
{
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF02025C);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF02025C);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF00003B);

    auto ret = ImGui::Button(label.data(), MainButtonSize());

    ImGui::PopStyleColor(3);

    return ret;
}

bool SecondaryButton(std::string_view label)
{
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF111314);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF111314);
    ImGui::PushStyleColor(ImGuiCol_Button, 0xFF0C0C0C);

    auto ret = ImGui::Button(label.data(), SecondaryButtonSize());

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

} // namespace

LauncherView::LauncherView(SDL_Renderer* renderer)
   : m_renderer(renderer)
{
    auto fs = cmrc::assets::get_filesystem();

    auto bg = fs.open("assets/bg.png");
    auto bg_rw = SDL_RWFromMem((void*)bg.begin(), bg.size());
    m_bg_surface = IMG_Load_RW(bg_rw, 0);

    m_bg_texture = SDL_CreateTextureFromSurface(m_renderer, m_bg_surface);

    m_file_browser = std::make_unique<ImGui::FileBrowser>(ImGuiFileBrowserFlags_NoTitleBar);
    m_file_browser->SetTypeFilters({".mpq", ".MPQ"});
    m_file_browser->SetTitle("Choose diabdat.mpq");
    m_file_browser->SetWindowPos(0,0);
}

LauncherView::~LauncherView()
{}

void LauncherView::FillPopupContent(const LauncherMVVM& mvvm)
{
    const ImU32 circularColor = 0xffffffff;
    const ImU32 col = 0xFF00003B;
    const ImU32 bg = 0xFF0C0C0C;

    {
        ImGui::Dummy(ImVec2(0, 64));

        ImGui::Dummy(ImVec2(32, 0));
        ImGui::SameLine();
        ImGui::Spinner("##spinner", 24, 4, circularColor);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(8, 0));
        ImGui::SameLine();

        {
            ImGui::BeginGroup();
            ImGui::Text("Downloading resources of demo version...");
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::BufferingBar("##buffer_bar", mvvm.downloadValue, ImVec2(320, 6), bg, col);
            ImGui::EndGroup();
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(32, 0));

        ImGui::Dummy(ImVec2(0, 64));
    }
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
            if (MainButton("DIABLO")) {
                if (OnDiabloClicked) {
                    OnDiabloClicked();
                }
            }
        } else {
            if (SecondaryButton("DIABLO")) {
                // do nothing
            }
        }

        ImGui::Dummy(ImVec2(0, 16));

        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (SecondaryButton("FOLDER")) {
            m_file_browser->Open();
            if (OnFolderClicked) {
                OnFolderClicked();
            }
        }

        ImGui::Dummy(ImVec2(0, 16));

        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (SecondaryButton("DEMO")) {
            if (OnDemoClicked) {
                OnDemoClicked();
            }
        }

        ImGui::Dummy(ImVec2(0, 16));

        if (buttonsOffsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + buttonsOffsetX);
        }

        if (SecondaryButton("INFO")) {
            if (OnFolderClicked) {
                OnFolderClicked();
            }
        }

        ImGui::SetNextWindowPos(
           ImVec2(ImGui::GetMainViewport()->Size.x * 0.5f, ImGui::GetMainViewport()->Size.y * 0.5f),
           0,
           ImVec2(0.5f, 0.5f));

        bool val = mvvm.keepPopup;
        if (ImGui::BeginPopupModal("ThePopup", &val, win_flags | ImGuiWindowFlags_AlwaysAutoResize)) {
            FillPopupContent(mvvm);

            ImGui::EndPopup();
        }

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF02025C);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF02025C);
        ImGui::PushStyleColor(ImGuiCol_Button, 0xFF00003B);

        m_file_browser->Display();

        ImGui::PopStyleColor(3);

        if (m_file_browser->HasSelected()) {
            const auto path = m_file_browser->GetSelected();

            spdlog::info("Selected file path: {}", path.string());
            m_file_browser->ClearSelected();

            if(OnResourceSelected) {
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
