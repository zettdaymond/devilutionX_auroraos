#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <imgui.h>

using ImGuiFileBrowserFlags = std::uint32_t;

enum ImGuiFileBrowserFlags_ : std::uint32_t {
    ImGuiFileBrowserFlags_SelectDirectory = 1 << 0,
    ImGuiFileBrowserFlags_EnterNewFilename = 1 << 1,
    ImGuiFileBrowserFlags_NoModal = 1 << 2,
    ImGuiFileBrowserFlags_NoTitleBar = 1 << 3,
    ImGuiFileBrowserFlags_NoStatusBar = 1 << 4,
    ImGuiFileBrowserFlags_CloseOnEsc = 1 << 5,
    ImGuiFileBrowserFlags_CreateNewDir = 1 << 6,
    ImGuiFileBrowserFlags_MultipleSelection = 1 << 7,
    ImGuiFileBrowserFlags_HideRegularFiles = 1 << 8,
    ImGuiFileBrowserFlags_ConfirmOnEnter = 1 << 9,
    ImGuiFileBrowserFlags_SkipItemsCausingError = 1 << 10,
    ImGuiFileBrowserFlags_EditPathString = 1 << 11,

    ImGuiFileBrowserFlags_NoResize = 1 << 12, // Запрет изменения размера
    ImGuiFileBrowserFlags_NoMove = 1 << 13,   // Запрет перемещения
    ImGuiFileBrowserFlags_AdaptiveSize = 1 << 14, // Адаптивный размер (портрет: весь экран, альбом: 75%)
    ImGuiFileBrowserFlags_Fullscreen = 1 << 15, // Всегда на весь экран
};

namespace ImGui {

class FileBrowser {
public:
    explicit FileBrowser(ImGuiFileBrowserFlags flags = 0,
                         std::filesystem::path defaultDirectory = std::filesystem::current_path());

    FileBrowser(const FileBrowser& copyFrom);
    FileBrowser& operator=(const FileBrowser& copyFrom);

    void SetWindowPos(int posX, int posY) noexcept;
    void SetWindowSize(int width, int height) noexcept;
    void SetTitle(std::string title);
    void Open();
    void Close();
    bool IsOpened() const noexcept;
    void Display();
    bool HasSelected() const noexcept;
    bool SetDirectory(const std::filesystem::path& dir = std::filesystem::current_path(), bool updateCachePath = true);
    bool SetPwd(const std::filesystem::path& dir = std::filesystem::current_path());
    auto GetDirectory() const noexcept -> const std::filesystem::path&;
    auto GetPwd() const noexcept -> const std::filesystem::path&;
    auto GetSelected() const -> std::filesystem::path;
    auto GetMultiSelected() const -> std::vector<std::filesystem::path>;
    void ClearSelected();
    void SetTypeFilters(const std::vector<std::string>& typeFilters);
    void SetCurrentTypeFilterIndex(int index);
    void SetInputName(std::string_view input);
    void SetMobileBehavior(bool enable);
    void SetTouchScrollSensitivity(float sensitivity);

private:
    struct FileRecord {
        bool isDir = false;
        std::filesystem::path name;
        std::filesystem::path fullPath;
        std::string showName;
        std::filesystem::path extension;
    };

    enum ViewMode_ {
        ViewMode_List,
        ViewMode_Grid,
    };

    enum class MobileScrollState {
        Idle,           // Бездействие
        Potential,     // Начало касания
        Scrolling,      // Активный скроллинг
        Deceleration    // Замедление после отпускания
    };

#ifdef _WIN32
    static std::uint32_t GetDrivesBitMask();
#endif

#if defined(__cpp_lib_char8_t)
    static std::string u8StrToStr(std::u8string s);
#endif
    static std::string u8StrToStr(std::string s);
    static std::filesystem::path u8StrToPath(const char* str);
    static void AssignToArrayStyleString(std::vector<char>& arr, std::string_view content);
    static int ExpandInputBuffer(ImGuiInputTextCallbackData* callbackData);
    static std::string ToLower(const std::string& s);

    // Мобильные UX методы
    bool isLandscapeMode() const;
    auto GetWindowWidth() const -> int;
    auto GetWindowHeight() const -> int;
    auto GetCurrentFolderDisplayName() const -> std::string;
    auto GetFileSizeMB(const FileRecord& rsc) const -> float;
    auto GetDisplayName(const FileRecord& rsc) const -> std::string;
    auto GetIconForRecord(const FileRecord& rsc) const -> const char*;
    bool IsSelected(const FileRecord& rsc) const;
    void RenderItemContextMenu();
    bool ShouldDisplay(const FileRecord& rsc) const;
    void FilterFiles(const std::string& filter);
    void RenderListView(float height);
    void RenderGridView(float height);
    void SortFiles();
    void UpdateFileRecords();
    void SetCurrentDirectoryUncatched(const std::filesystem::path& pwd);
    bool SetCurrentDirectoryInternal(const std::filesystem::path& dir, const std::filesystem::path& preferredFallback);
    bool IsExtensionMatched(const std::filesystem::path& extension) const;
    void ClearRangeSelectionState();
    void ToolTip(const std::string_view& s);
    void HandleItemSelect(const FileRecord& rsc, bool isPerformScroling = false);
    void ShowDeleteConfirmation();
    void StartRenamingItem();
    void ShareSelectedItem();
    void ShowItemProperties();
    void ConfirmSelection();
    auto FitTextToWidth(const std::string& text, float maxWidth) -> std::string;
    auto SetupWindowBehavior() -> ImGuiWindowFlags;
    void ApplyMobileScrolling();
    auto CalculateBottomPanelHeight(bool forceLandscapeMode = false) const -> float;
    void RenderCompactView(float height);
    void RenderBottomPanel(float height, bool forceLandscapeMode = false);
    void RenderCustomTitleBar(float height);
    void RenderViewSettingsPopup();
    void RenderSubHeader(float titleBarHeight, float subHeaderHeight);
    bool IsAtRoot() const; // Новая функция проверки корня
    bool IsDrivesView() const; // Проверка режима просмотра дисков
    void NavigateUp(); // Новая функция навигации вверх
    bool IsDriveRoot() const;
    bool IsFileSystemRoot() const;
    // Методы
    void RenderAdaptivePath(float titleBarHeight, float subHeaderHeight);
    void UpdateFullPathCache() const;
    float Scale(float value) const;
    ImVec2 Scale(ImVec2 vec) const;

    // Основные параметры
    int width_;
    int height_;
    int posX_;
    int posY_;
    ImGuiFileBrowserFlags flags_;
    std::filesystem::path defaultDirectory_;
    std::string title_;
    std::string openLabel_;

    bool shouldOpen_;
    bool shouldClose_;
    bool isOpened_;
    bool isOk_;
    bool isPosSet_;
    std::string statusStr_;

    std::vector<std::string> typeFilters_;
    unsigned int typeFilterIndex_ = 0;
    bool hasAllFilter_ = false;

    std::filesystem::path currentDirectory_;
    std::vector<FileRecord> fileRecords_;
    std::vector<FileRecord> filteredRecords_;

    unsigned int rangeSelectionStart_;
    std::set<std::filesystem::path> selectedFilenames_;

    std::string openNewDirLabel_;
    std::vector<char> newDirNameBuffer_;
    std::vector<char> inputNameBuffer_;
    std::string customizedInputName_;

    bool editDir_;
    bool setFocusToEditDir_;
    std::vector<char> currDirBuffer_;

    // Мобильные UX параметры
    std::vector<std::filesystem::path> navigationStack_;
    int currentNavIndex_ = 0;
    float navAnimationProgress_ = 0.0f;
    float itemHeight_ = 0.0f;
    ViewMode_ viewMode_ = ViewMode_List;
    float itemScale_ = 1.0f;
    int sortMode_ = 0;
    bool showHiddenFiles_ = false;
    bool showExtensions_ = true;
    bool showBottomPanel_ = true;
    std::string searchFilter_;
    std::filesystem::path selectedContextItem_;
    std::filesystem::path lastDirectory_;
    mutable std::string cachedDisplayName_;
    mutable std::filesystem::file_time_type lastPathWriteTime_;
    bool useBreadcrumbs_ = false;
    float bottomPanelHeight_ = 0.0f;
    bool mobileBehaviorEnabled_ = true;
    float touchScrollSensitivity_ = 1.0f;
    ImVec2 touchStartPos_;
    float touchStartTime_ = 0.0;
    float touchStartScrollY_ = 0.0f;
    MobileScrollState scrollState_ = MobileScrollState::Idle;
    bool showViewSettingsPopup = false;

    // Для адаптивного пути
    mutable std::string cachedFullPath_;
    mutable float fullPathWidth_ = 0.0f;
    float pathScrollOffset_ = 0.0f;
    bool isPathDragging_ = false;
    ImVec2 pathDragStartPos_;
    float pathDragStartOffset_ = 0.0f;


#ifdef _WIN32
    std::uint32_t drives_;
#endif
};

} // namespace ImGui
