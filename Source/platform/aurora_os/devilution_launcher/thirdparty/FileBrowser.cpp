#include "FileBrowser.h"

#include "imgui_internal.h"

#include "ui/Theme.hpp"

#include <unordered_map>

#include <ranges>

// Иконки FontAwesome
#define ICON_FA_ARROW_LEFT (const char*)(u8"\uf060\ufeff")
#define ICON_FA_ARROW_UP (const char*)(u8"\uf062\ufeff")
#define ICON_FA_SEARCH (const char*)(u8"\uf002\ufeff")
#define ICON_FA_STAR (const char*)(u8"\uf005\ufeff")
#define ICON_FA_ELLIPSIS_V (const char*)(u8"\uf142\ufeff")
#define ICON_FA_FOLDER (const char*)(u8"\uf07b\ufeff")
#define ICON_FA_FILE (const char*)(u8"\uf15b\ufeff")
#define ICON_FA_FILE_IMAGE (const char*)(u8"\uf1c5\ufeff")
#define ICON_FA_FILE_PDF (const char*)(u8"\uf1c1\ufeff")
#define ICON_FA_FILE_AUDIO (const char*)(u8"\uf1c7\ufeff")
#define ICON_FA_FILE_VIDEO (const char*)(u8"\uf1c8\ufeff")
#define ICON_FA_FILE_CODE (const char*)(u8"\uf1c9\ufeff")
#define ICON_FA_FILE_ARCHIVE (const char*)(u8"\uf1c6\ufeff")
#define ICON_FA_FILE_WORD (const char*)(u8"\uf1c2\ufeff")
#define ICON_FA_FILE_EXCEL (const char*)(u8"\uf1c3\ufeff")
#define ICON_FA_FILE_POWERPOINT (const char*)(u8"\uf1c4\ufeff")
#define ICON_FA_FILE_ZIP (const char*)(u8"\uf1c6\ufeff")
#define ICON_FA_TRASH (const char*)(u8"\uf1f8\ufeff")
#define ICON_FA_PEN (const char*)(u8"\uf304\ufeff")
#define ICON_FA_SHARE (const char*)(u8"\uf064\ufeff")
#define ICON_FA_INFO (const char*)(u8"\uf129\ufeff")
#define ICON_FA_DOWNLOAD (const char*)(u8"\uf019\ufeff")
#define ICON_FA_IMAGE (const char*)(u8"\uf03e\ufeff")
#define ICON_FA_MUSIC (const char*)(u8"\uf001\ufeff")
#define ICON_FA_VIDEO (const char*)(u8"\uf03d\ufeff")
#define ICON_FA_SLIDERS (const char*)(u8"\uf1de\ufeff")
#define ICON_FA_SORT (const char*)(u8"\uf0dc\ufeff")
#define ICON_FA_GEAR (const char*)(u8"\uf013\ufeff")
#define ICON_FA_LIST (const char*)(u8"\uf03a\ufeff")
#define ICON_FA_GRID (const char*)(u8"\uf00a\ufeff")
#define ICON_FA_HOME u8"\uf015\ufeff"
#define ICON_FA_REFRESH (const char*)(u8"\uf2f9\ufeff")
#define ICON_FA_ARROW_TURN_UP (const char*)(u8"\xef\x85\x88\ufeff") // U+f148

namespace ImGui {

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

const static std::filesystem::path DRIVES_PATH = "///Drives///";

} // namespace

FileBrowser::FileBrowser(ImGuiFileBrowserFlags flags, std::filesystem::path defaultDirectory)
   : width_(700)
   , height_(600)
   , posX_(0)
   , posY_(0)
   , flags_(flags)
   , defaultDirectory_(std::move(defaultDirectory))
   , shouldOpen_(false)
   , shouldClose_(false)
   , isOpened_(false)
   , isOk_(false)
   , isPosSet_(false)
   , rangeSelectionStart_(0)
   , editDir_(false)
   , setFocusToEditDir_(false)
   , viewMode_(ViewMode_List)
   , itemScale_(1.0f)
   , sortMode_(0)
   , showHiddenFiles_(false)
   , showExtensions_(true)
   , showBottomPanel_(true)
   , mobileBehaviorEnabled_(true)
   , touchScrollSensitivity_(1.0f)
{
    if (flags_ & ImGuiFileBrowserFlags_CreateNewDir) {
        newDirNameBuffer_.resize(32, '\0');
    }

    SetTitle("File Browser");

    typeFilters_.clear();
    typeFilterIndex_ = 0;
    hasAllFilter_ = false;
    bottomPanelHeight_ = ImGui::GetFrameHeight() * 2.5f;

#ifdef _WIN32
    // Убедимся, что DRIVES_PATH инициализирован корректно
    if (defaultDirectory.empty()) {
        defaultDirectory_ = DRIVES_PATH;
    } else if (defaultDirectory == DRIVES_PATH) {
        // Уже установлено
    } else if (std::filesystem::exists(defaultDirectory)) {
        // Нормализуем путь
        defaultDirectory_ = std::filesystem::absolute(defaultDirectory);
    }
#else
    if (defaultDirectory.empty()) {
        defaultDirectory_ = std::filesystem::current_path();
    }
#endif

    SetDirectory(defaultDirectory_, false);
}

void FileBrowser::SetTitle(std::string title)
{
    title_ = std::move(title);
    const std::string thisPtrStr = std::to_string(reinterpret_cast<size_t>(this));
    openLabel_ = title_ + "##filebrowser_" + thisPtrStr;
    openNewDirLabel_ = "new dir##new_dir_" + thisPtrStr;
}

void FileBrowser::SetMobileBehavior(bool enable)
{
    mobileBehaviorEnabled_ = enable;
}

void FileBrowser::SetTouchScrollSensitivity(float sensitivity)
{
    touchScrollSensitivity_ = sensitivity;
}

ImGuiWindowFlags FileBrowser::SetupWindowBehavior()
{
    // Определяем размеры экрана
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float screenWidth = viewport->Size.x;
    const float screenHeight = viewport->Size.y;
    const bool isLandscape = screenWidth > screenHeight;

    // Определяем размеры окна в зависимости от поведения
    float windowWidth = width_;
    float windowHeight = height_;
    bool noResize = false;
    bool noMove = false;

    // Поведение адаптивного размера
    if (flags_ & ImGuiFileBrowserFlags_AdaptiveSize) {
        if (isLandscape) {
            // Альбомная ориентация - 75% экрана
            windowWidth = screenWidth * 0.75f;
            windowHeight = screenHeight * 0.75f;
        } else {
            // Портретная ориентация - весь экран
            windowWidth = screenWidth;
            windowHeight = screenHeight;
        }
        noResize = true;
        noMove = true;
    }
    // Поведение полного экрана
    else if (flags_ & ImGuiFileBrowserFlags_Fullscreen) {
        windowWidth = screenWidth;
        windowHeight = screenHeight;
        noResize = true;
        noMove = true;
    }
    // Принудительное отключение изменения размера и перемещения
    else {
        noResize = flags_ & ImGuiFileBrowserFlags_NoResize;
        noMove = flags_ & ImGuiFileBrowserFlags_NoMove;
    }

    // Устанавливаем позицию и размер окна
    if (isPosSet_) {
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(posX_), static_cast<float>(posY_)));
    } else {
        // Центрируем окно
        ImGui::SetNextWindowPos(ImVec2((screenWidth - windowWidth) * 0.5f, (screenHeight - windowHeight) * 0.5f));
    }

    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));

    // Устанавливаем флаги окна
    ImGuiWindowFlags window_flags = 0;
    if (noResize) {
        window_flags |= ImGuiWindowFlags_NoResize;
    }
    if (noMove) {
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    if (flags_ & ImGuiFileBrowserFlags_NoTitleBar) {
        window_flags |= ImGuiWindowFlags_NoTitleBar;
    }

    // Для модального окна
    if (!(flags_ & ImGuiFileBrowserFlags_NoModal)) {
        window_flags |= ImGuiWindowFlags_NoCollapse;
    }

    // Запрещаем скролл для основного окна
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoScrollWithMouse;

    return window_flags;
}

void FileBrowser::ApplyMobileScrolling()
{
    if (!mobileBehaviorEnabled_) {
        return;
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    const bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    const bool isScrollable = window->ScrollMax.y > 0.0f;
    const float currentTime = static_cast<float>(ImGui::GetTime());

    // Обработка состояний
    switch (scrollState_) {
    case MobileScrollState::Idle:
        if (isWindowHovered && isScrollable && io.MouseDown[0]) {
            // Начало потенциального жеста
            touchStartPos_ = io.MousePos;
            touchStartScrollY_ = window->Scroll.y;
            touchStartTime_ = currentTime;
            scrollState_ = MobileScrollState::Potential;
        }
        break;

    case MobileScrollState::Potential:
        if (!io.MouseDown[0]) {
            // Кнопка мыши отпущена до начала скролла - сброс
            scrollState_ = MobileScrollState::Idle;
        } else {
            const float moveDeltaSqr = ImLengthSqr(io.MousePos - touchStartPos_);
            const float timeDelta = currentTime - touchStartTime_;

            // Если движение больше порога или прошло достаточно времени - начинаем скроллинг
            if (moveDeltaSqr > 25.0f || timeDelta > 0.2f) {
                scrollState_ = MobileScrollState::Scrolling;
            }
        }
        break;

    case MobileScrollState::Scrolling:
        if (io.MouseDown[0]) {
            // Непосредственно скроллинг
            const float delta = (touchStartPos_.y - io.MousePos.y) * touchScrollSensitivity_;
            const float newScrollY = touchStartScrollY_ + delta;
            window->Scroll.y = ImClamp(newScrollY, 0.0f, window->ScrollMax.y);
            ImGui::SetScrollY(window, window->Scroll.y);
        } else {
            // Кнопка мыши отпущена - завершаем скроллинг
            scrollState_ = MobileScrollState::Idle;
        }
        break;
    }
}

void FileBrowser::Open()
{
    UpdateFileRecords();
    ClearSelected();
    statusStr_ = std::string();
    shouldOpen_ = true;
    shouldClose_ = false;

    // if((flags_ & ImGuiFileBrowserFlags_EnterNewFilename) && !customizedInputName_.empty()) {
    //     AssignToArrayStyleString(inputNameBuffer_, customizedInputName_);
    //     selectedFilenames_ = { u8StrToPath(inputNameBuffer_.data()) };
    // }

    navigationStack_.clear();
    currentNavIndex_ = 0;
    navAnimationProgress_ = 0.0f;
    lastDirectory_ = currentDirectory_;
}

void FileBrowser::Close()
{
    shouldOpen_ = false;
    shouldClose_ = true;
    ClearSelected();
}

bool FileBrowser::IsOpened() const noexcept
{
    return isOpened_;
}

void FileBrowser::Display()
{
    PushID(this);
    ScopeGuard exitThis([this] {
        shouldOpen_ = false;
        shouldClose_ = false;
        PopID();
    });

    if (shouldOpen_) {
        OpenPopup(openLabel_.c_str());
    }
    if (shouldClose_) {
        CloseCurrentPopup();
    }
    isOpened_ = false;

    // Настройка поведения окна
    ImGuiWindowFlags window_flags = SetupWindowBehavior();

    // Открываем окно
    if (flags_ & ImGuiFileBrowserFlags_NoModal) {
        if (!BeginPopup(openLabel_.c_str(), window_flags)) {
            return;
        }
    } else if (!BeginPopupModal(openLabel_.c_str(), nullptr, window_flags)) {
        return;
    }

    isOpened_ = true;
    ScopeGuard endPopup([] {
        EndPopup();
    });

    const float titleBarHeight = ImGui::GetFontSize() * 2.7f;
    RenderCustomTitleBar(titleBarHeight);

    const float subtitleBarHeight = ImGui::GetFontSize() * 1.5f;
    RenderSubHeader(titleBarHeight, subtitleBarHeight);

    ImGui::SetCursorPosY(titleBarHeight + subtitleBarHeight); // Сдвигаем контент ниже

    // ====================== РАСЧЕТ РАЗМЕРОВ ======================
    constexpr static auto kForceBottomPanelLandscapeMode = true;

    const float availableHeight = ImGui::GetContentRegionAvail().y;
    float bottomPanelHeight = CalculateBottomPanelHeight(kForceBottomPanelLandscapeMode);
    float contentHeight = availableHeight - bottomPanelHeight;

    // Проверка на минимальную высоту контента
    const float minContentHeight = ImGui::GetFontSize() * 5.0f;
    if (contentHeight < minContentHeight) {
        // Уменьшаем нижнюю панель при нехватке места
        bottomPanelHeight = std::min(bottomPanelHeight, availableHeight - minContentHeight);
        contentHeight = availableHeight - bottomPanelHeight;

        RenderCompactView(contentHeight);
    } else {
        // ====================== ОСНОВНОЙ КОНТЕНТ ======================
        if (viewMode_ == ViewMode_List) {
            RenderListView(contentHeight);
        } else {
            RenderGridView(contentHeight);
        }
    }
    // ====================== КОНЕЦ КОНТЕНТА ======================

    // ====================== НИЖНЯЯ ПАНЕЛЬ ======================
    if (showBottomPanel_ && bottomPanelHeight > 0) {
        // Фиксируем позицию в самом низу
        const float yPos = ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - bottomPanelHeight;
        ImGui::SetCursorPosY(yPos);

        // Рисуем панель с гарантированно достаточной высотой
        RenderBottomPanel(bottomPanelHeight, kForceBottomPanelLandscapeMode);
    }
    // ====================== КОНЕЦ ПАНЕЛИ ======================

    // Обработка закрытия по ESC
    if ((flags_ & ImGuiFileBrowserFlags_CloseOnEsc) && IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
        && IsKeyPressed(ImGuiKey_Escape)) {
        CloseCurrentPopup();
    }
}

bool FileBrowser::HasSelected() const noexcept
{
    return isOk_;
}

// ====================== МОБИЛЬНЫЕ UX МЕТОДЫ ======================

bool FileBrowser::isLandscapeMode() const
{
    return GetWindowWidth() > GetWindowHeight() * 1.2f;
}

int FileBrowser::GetWindowWidth() const
{
    return static_cast<int>(ImGui::GetWindowSize().x);
}

int FileBrowser::GetWindowHeight() const
{
    return static_cast<int>(ImGui::GetWindowSize().y);
}

std::string FileBrowser::GetCurrentFolderDisplayName() const
{
    // Режим просмотра дисков
    if (IsDrivesView()) {
        return "My Computer";
    }

#ifdef _WIN32
    if (IsDriveRoot()) {
        return currentDirectory_.root_name().string() + "\\";
    }
#endif

    if (currentDirectory_ == defaultDirectory_) {
        return u8StrToStr(ICON_FA_HOME + std::u8string(u8" Home"));
    }

    std::string dirName = u8StrToStr(currentDirectory_.filename().u8string());
    if (dirName.empty()) {
#ifdef _WIN32
        return u8StrToStr(currentDirectory_.root_name().u8string()) + "\\";
#else
        return "/";
#endif
    }

    const int maxLength = isLandscapeMode() ? 25 : 15;
    if (dirName.length() > maxLength) {
        dirName = dirName.substr(0, maxLength - 3) + "...";
    }

    return dirName;
}

float FileBrowser::GetFileSizeMB(const FileRecord& rsc) const
{
    if (rsc.isDir) {
        return 0.0f;
    }

    try {
        auto size = std::filesystem::file_size(currentDirectory_ / rsc.name);
        return static_cast<float>(size) / (1024.0f * 1024.0f);
    } catch (...) {
        return 0.0f;
    }
}

std::string FileBrowser::GetDisplayName(const FileRecord& rsc) const
{
    if (rsc.name == "..") {
        return u8StrToStr(std::u8string(u8".."));
    }

    std::string name = u8StrToStr(rsc.name.u8string());

    int maxLength;
    if (isLandscapeMode()) {
        maxLength = (viewMode_ == ViewMode_List) ? 30 : 15;
    } else {
        maxLength = (viewMode_ == ViewMode_List) ? 20 : 12;
    }

    if (name.length() > static_cast<size_t>(maxLength)) {
        name = name.substr(0, maxLength - 3) + "...";
    }

    return name;
}

const char* FileBrowser::GetIconForRecord(const FileRecord& rsc) const
{
    if (rsc.isDir) {
        if (rsc.name == "Downloads") {
            return ICON_FA_DOWNLOAD;
        }
        if (rsc.name == "Pictures") {
            return ICON_FA_IMAGE;
        }
        if (rsc.name == "Documents") {
            return ICON_FA_FILE;
        }
        if (rsc.name == "Music") {
            return ICON_FA_MUSIC;
        }
        if (rsc.name == "Videos") {
            return ICON_FA_VIDEO;
        }
        if (rsc.name == "..") {
            return ICON_FA_ARROW_TURN_UP;
        }
        return ICON_FA_FOLDER;
    }

    static const std::unordered_map<std::string, const char*> iconMap = {
       {".pdf", ICON_FA_FILE_PDF},         {".doc", ICON_FA_FILE_WORD},   {".docx", ICON_FA_FILE_WORD},
       {".xls", ICON_FA_FILE_EXCEL},       {".xlsx", ICON_FA_FILE_EXCEL}, {".ppt", ICON_FA_FILE_POWERPOINT},
       {".pptx", ICON_FA_FILE_POWERPOINT}, {".zip", ICON_FA_FILE_ZIP},    {".rar", ICON_FA_FILE_ZIP},
       {".7z", ICON_FA_FILE_ZIP},          {".jpg", ICON_FA_FILE_IMAGE},  {".jpeg", ICON_FA_FILE_IMAGE},
       {".png", ICON_FA_FILE_IMAGE},       {".gif", ICON_FA_FILE_IMAGE},  {".bmp", ICON_FA_FILE_IMAGE},
       {".mp3", ICON_FA_FILE_AUDIO},       {".wav", ICON_FA_FILE_AUDIO},  {".flac", ICON_FA_FILE_AUDIO},
       {".mp4", ICON_FA_FILE_VIDEO},       {".avi", ICON_FA_FILE_VIDEO},  {".mov", ICON_FA_FILE_VIDEO},
       {".cpp", ICON_FA_FILE_CODE},        {".h", ICON_FA_FILE_CODE},     {".hpp", ICON_FA_FILE_CODE},
       {".js", ICON_FA_FILE_CODE},         {".ts", ICON_FA_FILE_CODE},    {".py", ICON_FA_FILE_CODE},
       {".html", ICON_FA_FILE_CODE},       {".css", ICON_FA_FILE_CODE},   {".json", ICON_FA_FILE_CODE},
       {".xml", ICON_FA_FILE_CODE},        {".md", ICON_FA_FILE_CODE},
    };

    auto ext = rsc.extension.u8string();
    auto it = iconMap.find(u8StrToStr(ext));
    return (it != iconMap.end()) ? it->second : ICON_FA_FILE;
}

bool FileBrowser::IsSelected(const FileRecord& rsc) const
{
    return selectedFilenames_.find(rsc.name) != selectedFilenames_.end();
}

void FileBrowser::RenderItemContextMenu()
{
    if (ImGui::BeginPopup("##ItemContextMenu")) {
        const bool isDir = std::filesystem::is_directory(currentDirectory_ / selectedContextItem_);

        // if (ImGui::MenuItem(ICON_FA_TRASH, " Delete")) ShowDeleteConfirmation();
        // if (ImGui::MenuItem(ICON_FA_PEN, " Rename")) StartRenamingItem();
        // if (!isDir && ImGui::MenuItem(ICON_FA_SHARE, " Share")) ShareSelectedItem();
        // ImGui::Separator();
        // if (ImGui::MenuItem(ICON_FA_INFO, " Properties")) ShowItemProperties();
        ImGui::EndPopup();
    }
}

bool FileBrowser::ShouldDisplay(const FileRecord& rsc) const
{
    if (rsc.name == "..") {
        return true;
    }

    const bool hideFiles = (flags_ & ImGuiFileBrowserFlags_HideRegularFiles)
                           && (flags_ & ImGuiFileBrowserFlags_SelectDirectory);
    if (!rsc.isDir && hideFiles) {
        return false;
    }

    if (!rsc.isDir && !IsExtensionMatched(rsc.extension)) {
        return false;
    }

    if (!showHiddenFiles_ && !rsc.name.empty() && rsc.name.c_str()[0] == '.') {
        return false;
    }

    if (!searchFilter_.empty()) {
        std::string name = ToLower(u8StrToStr(rsc.name.u8string()));
        if (name.find(searchFilter_) == std::string::npos) {
            return false;
        }
    }

    return true;
}

void FileBrowser::FilterFiles(const std::string& filter)
{
    searchFilter_ = ToLower(filter);
    filteredRecords_.clear();

    for (const auto& record : fileRecords_) {
        if (ShouldDisplay(record)) {
            filteredRecords_.push_back(record);
        }
    }
}

void FileBrowser::RenderListView(float height)
{
    // Адаптивная высота элементов
    itemHeight_ = ImGui::GetFontSize() * 2.4f;

    // Гарантируем что высота не превышает доступную
    const int maxItems = static_cast<int>(height / itemHeight_);
    if (maxItems < 1) {
        return;
    }

    ImGui::BeginChild("##FileList", ImVec2(0, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // Применяем мобильный скроллинг ТОЛЬКО к области списка файлов
    auto isScrolling = (scrollState_ == MobileScrollState::Scrolling);
    ApplyMobileScrolling();

    // Фиксированные отступы
    const float horizontalPadding = 16.0f;

    const auto& records = filteredRecords_.empty() ? fileRecords_ : filteredRecords_;

    auto iconFont = launcher::ui::Theme::font(launcher::ui::FontRole::IconBig);

    for (int i = 0; i < records.size(); i++) {
        const auto& rsc = records[i];

        if (!ShouldDisplay(rsc)) {
            continue;
        }

        const bool selected = IsSelected(rsc);
        const char* icon = GetIconForRecord(rsc);
        const std::string name = GetDisplayName(rsc);

        ImGui::PushID(i);

        // Создаем селектабл для всего элемента
        if (ImGui::Selectable("##item",
                              selected,
                              ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns,
                              ImVec2(0, itemHeight_))) {
            HandleItemSelect(rsc, isScrolling);
        }

        if (iconFont) {
            ImGui::PushFont(iconFont);
        }
        const float textHeight = ImGui::GetTextLineHeight();
        const float verticalOffset = (itemHeight_ - textHeight) / 2.0f;

        // Получаем позицию элемента
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();

        const float iconWidth = ImGui::CalcTextSize(icon).x;
        const float iconHeight = ImGui::CalcTextSize(icon).y;

        // Рассчет позиций
        const float iconX = itemMin.x + horizontalPadding;
        const float iconY = itemMin.y + verticalOffset;

        // Bounding box для иконки
        const ImVec2 iconMin(iconX, itemMin.y);
        const ImVec2 iconMax(iconX + iconWidth, itemMin.y + itemHeight_);

        // Рендеринг иконки
        ImGui::GetWindowDrawList()->AddText(ImVec2(iconX, iconY), ImGui::GetColorU32(ImGuiCol_Text), icon);

        if (iconFont) {
            ImGui::PopFont();
        }

        // Текст начинается после фиксированной колонки иконок

        // Рассчет позиции текста с отступом от иконки
        const float textX = iconX + iconMax.x - (iconWidth * 0.5);
        const float textY = itemMin.y + (itemHeight_ - ImGui::GetTextLineHeight()) / 2.0f;

        // Рассчет доступной ширины для текста
        const float availableWidth = ImGui::GetContentRegionAvail().x - textX - horizontalPadding;

        // Обрезаем текст если нужно
        const std::string clippedName = FitTextToWidth(name, availableWidth);

        ImGui::GetWindowDrawList()->AddText(ImVec2(textX, textY),
                                            ImGui::GetColorU32(ImGuiCol_Text),
                                            clippedName.c_str());

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void FileBrowser::RenderGridView(float height)
{
    // Адаптивный размер элементов
    const float minItemWidth = ImGui::GetFontSize() * 5.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;

    // Рассчитываем количество колонок с ограничением
    int columns = static_cast<int>(availableWidth / minItemWidth);
    columns = ImClamp(columns, 1, 10);

    // Рассчитываем реальную ширину элемента
    const float itemWidth = availableWidth / columns;
    const float itemHeight = itemWidth * 0.9f;

    // Гарантируем что высота не превышает доступную
    const int maxRows = static_cast<int>(height / itemHeight);
    if (maxRows < 1) {
        return;
    }

    ImGui::BeginChild("##GridFiles", ImVec2(0, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // Применяем мобильный скроллинг ТОЛЬКО к области списка файлов
    auto isScrolling = (scrollState_ == MobileScrollState::Scrolling);
    ApplyMobileScrolling();

    // Фиксированные отступы
    const float horizontalPadding = 8.0f;
    const float verticalPadding = 8.0f;

    const auto& records = filteredRecords_.empty() ? fileRecords_ : filteredRecords_;

    std::decay_t<decltype(records)> shouldDisplayRecords;
    shouldDisplayRecords.reserve(records.size());

    std::copy_if(records.begin(), records.end(), std::back_inserter(shouldDisplayRecords), [this](const auto& value) {
        return ShouldDisplay(value);
    });

    // Рассчитываем общее количество строк
    const int totalRows = static_cast<int>((records.size() + columns - 1) / columns);

    for (int row = 0; row < totalRows; row++) {
        for (int col = 0; col < columns; col++) {
            const size_t index = row * columns + col;
            if (index >= shouldDisplayRecords.size()) {
                break;
            }
            const auto& rsc = shouldDisplayRecords[index];

            const bool selected = IsSelected(rsc);
            const char* icon = GetIconForRecord(rsc);
            const std::string name = GetDisplayName(rsc);

            ImGui::PushID(static_cast<int>(index));

            // Начинаем группу для элемента
            ImGui::BeginGroup();

            // Рассчитываем размер иконки
            const float iconSize = itemHeight * 0.4f;

            // Создаем селектабл для всего элемента
            if (ImGui::Selectable("##griditem",
                                  selected,
                                  ImGuiSelectableFlags_AllowDoubleClick,
                                  ImVec2(itemWidth, itemHeight))) {
                HandleItemSelect(rsc, isScrolling);
            }

            // Получаем позицию элемента
            ImVec2 itemMin = ImGui::GetItemRectMin();

            // Центрируем иконку по горизонтали
            const float iconX = itemMin.x + (itemWidth - iconSize) / 2.0f;
            // Центрируем иконку
            const float iconY = itemMin.y + verticalPadding;

            // Рисуем иконку
            auto largeIconFont = launcher::ui::Theme::font(launcher::ui::FontRole::IconBig);
            if (largeIconFont) {
                ImGui::PushFont(largeIconFont);
            }
            ImGui::GetWindowDrawList()->AddText(ImVec2(iconX, iconY), ImGui::GetColorU32(ImGuiCol_Text), icon);
            if (largeIconFont) {
                ImGui::PopFont();
            }

            // Рисуем текст (центрированный по горизонтали)
            const std::string displayName = FitTextToWidth(name, itemWidth - horizontalPadding * 2);
            const float textWidth = ImGui::CalcTextSize(displayName.c_str()).x;
            const float textX = itemMin.x + (itemWidth - textWidth) / 2.0f;

            ImGui::GetWindowDrawList()->AddText(ImVec2(textX, itemMin.y + itemHeight * 0.6f),
                                                ImGui::GetColorU32(ImGuiCol_Text),
                                                displayName.c_str());

            ImGui::EndGroup();

            // Переходим к следующей колонке
            if (col < columns - 1) {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
}

void FileBrowser::HandleItemSelect(const FileRecord& rsc, bool isPerformScroling)
{

    if (isPerformScroling) {
        return;
    }

    if (rsc.isDir) {
        // // Мобильное поведение: одинарный тап для входа в папку
        // if (mobileBehaviorEnabled_) {
        //     SetDirectory((rsc.name != "..") ? (currentDirectory_ / rsc.name) : currentDirectory_.parent_path());
        // }
        // // Десктопное поведение: двойной клик для входа
        // else if (ImGui::IsMouseDoubleClicked(0)) {
        //     SetDirectory((rsc.name != "..") ? (currentDirectory_ / rsc.name) : currentDirectory_.parent_path());
        // }
        // Обработка специальных случаев
        if (rsc.name == "..") {
            NavigateUp();
            return;
        }

#ifdef _WIN32
        // Обработка перехода к диску
        if (IsDrivesView()) {
            SetDirectory(rsc.name);
            return;
        }
#endif
        // Обычный переход в папку
        SetDirectory(currentDirectory_ / rsc.name);
    } else {
        // Обработка выбора файла
        if (flags_ & ImGuiFileBrowserFlags_MultipleSelection) {
            if (GetIO().KeyCtrl) {
                if (IsSelected(rsc)) {
                    selectedFilenames_.erase(rsc.name);
                } else {
                    selectedFilenames_.insert(rsc.name);
                }
            } else {
                selectedFilenames_ = {rsc.name};
            }
        } else {
            // Мобильное поведение: одинарный тап для выбора файла
            if (mobileBehaviorEnabled_) {
                selectedFilenames_ = {rsc.name};
                isOk_ = true;
                CloseCurrentPopup();
            }
            // Десктопное поведение: двойной клик для выбора
            else if (ImGui::IsMouseDoubleClicked(0)) {
                selectedFilenames_ = {rsc.name};
                isOk_ = true;
                CloseCurrentPopup();
            }
        }
    }
}

void FileBrowser::SortFiles()
{
    // Сортировка: сначала папки, потом файлы, по имени
    std::sort(fileRecords_.begin() + 1, fileRecords_.end(), [](const FileRecord& a, const FileRecord& b) {
        if (a.isDir != b.isDir) {
            return a.isDir;
        }
        return a.name < b.name;
    });
}

void FileBrowser::ConfirmSelection()
{
    if (!selectedFilenames_.empty()
        || ((flags_ & ImGuiFileBrowserFlags_SelectDirectory) && !(flags_ & ImGuiFileBrowserFlags_EnterNewFilename))) {
        isOk_ = true;
        CloseCurrentPopup();
    }
}

// ====================== ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ======================

void FileBrowser::UpdateFileRecords()
{
    fileRecords_.clear();

#ifdef _WIN32
    // Режим просмотра дисков (только Windows)
    if (IsDrivesView()) {
        for (char drive = 'A'; drive <= 'Z'; ++drive) {
            std::string root = std::string(1, drive) + ":\\";
            if (std::filesystem::exists(root)) {
                FileRecord rcd;
                rcd.isDir = true;
                rcd.name = root;
                rcd.showName = root;
                fileRecords_.push_back(rcd);
            }
        }
        return;
    }
#endif

    // Добавляем ".." только если не в корне
    if (!IsFileSystemRoot() || IsDriveRoot()) {
        fileRecords_.push_back(FileRecord{true, "..", "[D] ..", ""});
    }

    for (auto& p : std::filesystem::directory_iterator(currentDirectory_)) {
        FileRecord rcd;
        try {
            if (p.is_regular_file()) {
                rcd.isDir = false;
            } else if (p.is_directory()) {
                rcd.isDir = true;
            } else {
                continue;
            }

            rcd.name = p.path().filename();
            if (rcd.name.empty()) {
                continue;
            }

            rcd.extension = p.path().filename().extension();
            rcd.showName = u8StrToStr(p.path().filename().u8string());

            // Пропускаем файлы, не соответствующие фильтру
            if (!rcd.isDir && !IsExtensionMatched(rcd.extension)) {
                continue;
            }
        } catch (...) {
            if (!(flags_ & ImGuiFileBrowserFlags_SkipItemsCausingError)) {
                throw;
            }
            continue;
        }
        fileRecords_.push_back(rcd);
    }

    SortFiles();
    if (!searchFilter_.empty()) {
        FilterFiles(searchFilter_);
    } else {
        filteredRecords_.clear();
    }
    ClearRangeSelectionState();
}

bool FileBrowser::SetDirectory(const std::filesystem::path& dir, bool updateCachePath)
{
    try {
        // Обычная обработка
        SetCurrentDirectoryUncatched(dir);

        // Добавляем в историю навигации
        if (!navigationStack_.empty() && navigationStack_.back() != currentDirectory_) {
            navigationStack_.push_back(currentDirectory_);
            currentNavIndex_ = navigationStack_.size() - 1;
        }
    } catch (...) {
        // Обработка ошибок
    }

    // Сбрасываем скролл при смене директории
    pathScrollOffset_ = 0.0f;
    isPathDragging_ = false;

    // Обновляем кэш пути
    if(updateCachePath){
        UpdateFullPathCache();
    }

    return true;
}

bool FileBrowser::SetPwd(const std::filesystem::path& dir)
{
    return SetDirectory(dir);
}

std::filesystem::path FileBrowser::GetSelected() const
{
    return currentDirectory_ / *selectedFilenames_.begin();
}

std::string FileBrowser::ToLower(const std::string& s)
{
    std::string ret = s;
    for (char& c : ret) {
        c = static_cast<char>(std::tolower(c));
    }
    return ret;
}

void FileBrowser::ToolTip(const std::string_view& s)
{
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", s.data());
    }
}

// Остальные методы остаются без существенных изменений (SetWindowSize, HasSelected, и т.д.)

// ... [Остальная реализация стандартных методов FileBrowser] ...

#ifdef _WIN32
std::uint32_t FileBrowser::GetDrivesBitMask()
{
    std::uint32_t ret = 0;
    for (int i = 0; i < 26; ++i) {
        const char rootName[4] = {static_cast<char>('A' + i), ':', '\\', '\0'};
        try {
            if (std::filesystem::exists(rootName)) {
                ret |= (1 << i);
            }
        } catch (...) {
        }
    }
    return ret;
}
#endif

void FileBrowser::SetWindowPos(int posX, int posY) noexcept
{
    posX_ = posX;
    posY_ = posY;
    isPosSet_ = true;
}

void FileBrowser::SetWindowSize(int width, int height) noexcept
{
    assert(width > 0 && height > 0);
    width_ = width;
    height_ = height;
}

std::filesystem::path FileBrowser::u8StrToPath(const char* str)
{
#if defined(__cpp_lib_char8_t)
    // With C++20/23, it's impossible to efficiently convert a `char*` string to a `char8_t*` string without violating
    // the strict aliasing rule. Bad joke!
    const size_t len = std::strlen(str);
    std::u8string u8Str;
    u8Str.resize(len);
    std::memcpy(u8Str.data(), str, len);
    return std::filesystem::path(u8Str);
#else
    // u8path is deprecated in C++20
    return std::filesystem::u8path(str);
#endif
}

void FileBrowser::ClearSelected()
{
    selectedFilenames_.clear();
    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename) {
        inputNameBuffer_[0] = '\0';
    }
    isOk_ = false;
}

#if defined(__cpp_lib_char8_t)
std::string FileBrowser::u8StrToStr(std::u8string s)
{
    std::string result;
    result.resize(s.length());
    std::memcpy(result.data(), s.data(), s.length());
    return result;
}
#endif

std::string FileBrowser::u8StrToStr(std::string s)
{
    return s;
}

void FileBrowser::ClearRangeSelectionState()
{
    rangeSelectionStart_ = 9999999;
    const bool dir = flags_ & ImGuiFileBrowserFlags_SelectDirectory;
    for (unsigned int i = 1; i < fileRecords_.size(); ++i) {
        if (fileRecords_[i].isDir == dir) {
            if (!dir && !IsExtensionMatched(fileRecords_[i].extension)) {
                continue;
            }
            rangeSelectionStart_ = i;
            break;
        }
    }
}

const std::filesystem::path& FileBrowser::GetDirectory() const noexcept
{
    return currentDirectory_;
}

const std::filesystem::path& FileBrowser::GetPwd() const noexcept
{
    return GetDirectory();
}

bool FileBrowser::IsExtensionMatched(const std::filesystem::path& _extension) const
{
#ifdef _WIN32
    std::filesystem::path extension = ToLower(u8StrToStr(_extension.string()));
#else
    auto& extension = _extension;
#endif

    // no type filters
    if (typeFilters_.empty()) {
        return true;
    }

    if (hasAllFilter_ && typeFilterIndex_ == 0) {
        return true;
    }

    // Получаем текущий фильтр
    const std::string& currentFilter = typeFilters_[typeFilterIndex_];

    // Специальный случай: фильтр "All Files" не первый
    if (currentFilter == ".*" || currentFilter == "*" || currentFilter == "*.*") {
        return true;
    }

    // Преобразуем расширение в нижний регистр
    std::string ext = ToLower(extension.string());

    // Обрабатываем различные форматы фильтров:
    // 1. Фильтр без точки: "mpq" -> проверяем на "mpq"
    // 2. Фильтр с точкой: ".mpq" -> проверяем на ".mpq"
    // 3. Фильтр с маской: "*.mpq" -> проверяем на ".mpq"

    std::string pattern = currentFilter;
    if (pattern.size() > 1 && pattern[0] == '*' && pattern[1] == '.') {
        pattern = pattern.substr(1); // Преобразуем "*.mpq" в ".mpq"
    }

    // Удаляем лишние пробелы
    pattern.erase(remove(pattern.begin(), pattern.end(), ' '), pattern.end());

    // Проверяем совпадение
    if (!pattern.empty() && pattern[0] != '.' && ext.size() > 0 && ext[0] == '.') {
        // Сравниваем без точки
        return ToLower(pattern) == ext.substr(1);
    }

    return ToLower(pattern) == ext;
}

bool FileBrowser::SetCurrentDirectoryInternal(const std::filesystem::path& dir,
                                              const std::filesystem::path& preferredFallback)
{
    try {
        SetCurrentDirectoryUncatched(dir);
        return true;
    } catch (const std::exception& err) {
        statusStr_ = std::string("error: ") + err.what();
    } catch (...) {
        statusStr_ = "unknown error";
    }

    if (preferredFallback != defaultDirectory_) {
        try {
            SetCurrentDirectoryUncatched(preferredFallback);
        } catch (...) {
            SetCurrentDirectoryUncatched(defaultDirectory_);
        }
    } else {
        SetCurrentDirectoryUncatched(defaultDirectory_);
    }

    return false;
}

void FileBrowser::SetCurrentDirectoryUncatched(const std::filesystem::path& pwd)
{
#ifdef _WIN32
    // Обработка специального пути дисков
    if (pwd == DRIVES_PATH) {
        currentDirectory_ = pwd;
        UpdateFileRecords();
        return;
    }
#endif

    currentDirectory_ = absolute(pwd);
    UpdateFileRecords();
    selectedFilenames_.clear();
    if (flags_ & ImGuiFileBrowserFlags_EnterNewFilename) {
        inputNameBuffer_[0] = '\0';
    }
}

int FileBrowser::ExpandInputBuffer(ImGuiInputTextCallbackData* callbackData)
{
    if (callbackData && callbackData->EventFlag & ImGuiInputTextFlags_CallbackResize) {
        auto buffer = static_cast<std::vector<char>*>(callbackData->UserData);
        size_t newSize = buffer->size();
        while (newSize < static_cast<size_t>(callbackData->BufSize)) {
            newSize <<= 1;
        }
        buffer->resize(newSize, '\0');
        callbackData->Buf = buffer->data();
        callbackData->BufDirty = true;
    }
    return 0;
}

// Новые вспомогательные функции

void FileBrowser::RenderCompactView(float height)
{
    const float itemHeight = ImGui::GetFontSize() * 2.0f;
    ImGui::BeginChild("##CompactList", ImVec2(0, height), true);

    const auto& records = filteredRecords_.empty() ? fileRecords_ : filteredRecords_;

    // Упрощенный список с минимальной высотой
    for (size_t i = 0; i < records.size() && i < 5; i++) {
        const auto& rsc = records[i];
        if (!ShouldDisplay(rsc)) {
            continue;
        }

        const bool selected = IsSelected(rsc);
        const std::string label = std::string(GetIconForRecord(rsc)) + " " + GetDisplayName(rsc);

        if (ImGui::Selectable(label.c_str(), selected, 0, ImVec2(0, itemHeight))) {
            HandleItemSelect(rsc);
        }
    }

    ImGui::EndChild();

    // Кнопки в компактном режиме
    const float buttonHeight = ImGui::GetFontSize() * 2.0f;
    if (ImGui::Button("Cancel", ImVec2(-1, buttonHeight))) {
        CloseCurrentPopup();
    }
    if (ImGui::Button("Select", ImVec2(-1, buttonHeight))) {
        ConfirmSelection();
    }
}

float FileBrowser::CalculateBottomPanelHeight(bool forceLandscapeMode) const
{
    if (!showBottomPanel_) {
        return 0.0f;
    }
    const bool landscape = forceLandscapeMode || isLandscapeMode();
    const float buttonHeight = landscape ? ImGui::GetFontSize() * 1.8f
                                         : ImGui::GetFontSize() * 2.5f; // Увеличили для портрета
    const float padding = ImGui::GetStyle().ItemSpacing.y;
    const float innerPadding = padding * 2;
    if (landscape) {
        return buttonHeight + innerPadding;
    } else {
        return (buttonHeight * 2) + (padding * 3); // 2 кнопки и 3 отступа (сверху, между, снизу)
    }
}

std::string FileBrowser::FitTextToWidth(const std::string& text, float maxWidth)
{
    if (maxWidth <= 0) {
        return "";
    }

    const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    if (textSize.x <= maxWidth) {
        return text;
    }

    // Вычисляем сколько символов поместится
    float width = 0.0f;
    const char* end = text.c_str();
    while (*end && width < maxWidth) {
        const char* next = end + 1;
        width += ImGui::CalcTextSize(end, next).x;
        end = next;
    }

    // Возвращаем обрезанный текст с многоточием
    const size_t len = end - text.c_str();
    if (len > 3) {
        return text.substr(0, len - 3) + "...";
    }
    return text.substr(0, len);
}

void FileBrowser::RenderBottomPanel(float height, bool forceLandscapeMode)
{
    const bool landscape = forceLandscapeMode || isLandscapeMode();
    const float padding = ImGui::GetStyle().ItemSpacing.y;
    const float buttonHeight = ImGui::GetFontSize() * 2.0f; // Фиксированная высота

    // Убираем скролл в панели
    ImGui::BeginChild("##BottomPanel",
                      ImVec2(0, height),
                      false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Вертикальный центр
    const float verticalOffset = (height - (landscape ? buttonHeight : buttonHeight * 2 + padding)) / 2;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);

    if (landscape) {
        const float buttonWidth = (ImGui::GetContentRegionAvail().x - padding) / 2.0f;

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, buttonHeight))) {
            CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Select", ImVec2(buttonWidth, buttonHeight))) {
            ConfirmSelection();
        }
    } else {
        const float buttonWidth = ImGui::GetContentRegionAvail().x;

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, buttonHeight))) {
            CloseCurrentPopup();
        }
        ImGui::Dummy(ImVec2(0, padding));
        if (ImGui::Button("Select", ImVec2(buttonWidth, buttonHeight))) {
            ConfirmSelection();
        }
    }

    ImGui::EndChild();
}

void FileBrowser::RenderCustomTitleBar(float height) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();

           // Фон заголовка
    drawList->AddRectFilled(
        windowPos,
        ImVec2(windowPos.x + windowSize.x, windowPos.y + height),
        ImGui::GetColorU32(ImVec4(0.15f, 0.05f, 0.05f, 1.0f))
        );

           // Разделительная линия
    drawList->AddLine(
        ImVec2(windowPos.x, windowPos.y + height),
        ImVec2(windowPos.x + windowSize.x, windowPos.y + height),
        ImGui::GetColorU32(ImVec4(0.6f, 0.1f, 0.1f, 1.0f)),
        2.0f
        );

           // Кнопка "назад"
    const float buttonSize = height * 0.7f;
    const float padding = height * 0.15f;
    ImGui::SetCursorPos(ImVec2(padding, (height - buttonSize) * 0.5f));
    if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(buttonSize, buttonSize))) {
        NavigateUp();
    }

    // Заголовок по центру
    std::string displayTitle = title_;
    ImVec2 textSize = ImGui::CalcTextSize(displayTitle.c_str());
    ImVec2 textPos(
        windowPos.x + (windowSize.x - textSize.x) * 0.5f,
        windowPos.y + (height - textSize.y) * 0.5f
        );
    drawList->AddText(
        textPos,
        ImGui::GetColorU32(ImVec4(0.86f, 0.77f, 0.62f, 1.0f)),
        displayTitle.c_str()
        );

    const auto menuButtonPosX = windowSize.x - buttonSize - padding;

    // Кнопка меню (три точки) справа
    ImGui::SetCursorPos(ImVec2(
        menuButtonPosX,
        (height - buttonSize) * 0.5f
        ));
    if (ImGui::Button(ICON_FA_ELLIPSIS_V, ImVec2(buttonSize, buttonSize))) {
        showViewSettingsPopup = true;
    }

    // Всплывающее меню
    if (showViewSettingsPopup) {
        ImGui::OpenPopup("##ViewSettingsPopup");
        showViewSettingsPopup = false;
    }
    RenderViewSettingsPopup();
}

void FileBrowser::RenderViewSettingsPopup()
{
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    const float titleBarHeight = ImGui::GetFontSize() * 2.5f;

    // Позиция меню - справа под кнопкой
    ImGui::SetNextWindowPos(ImVec2(windowPos.x + windowSize.x - Scale(200.0f), windowPos.y + titleBarHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scale(ImVec2(10, 10)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Scale(ImVec2(10, 10)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Scale(5.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.06f, 0.06f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));

    if (ImGui::BeginPopup("##ViewSettingsPopup", ImGuiWindowFlags_NoMove)) {
        // Заголовок меню
        ImGui::TextColored(ImVec4(0.86f, 0.77f, 0.62f, 1.0f), "VIEW SETTINGS");
        ImGui::Separator();

        // Пункт для скрытых файлов
        if (ImGui::MenuItem("Show Hidden Files", nullptr, showHiddenFiles_)) {
            showHiddenFiles_ = !showHiddenFiles_;
            UpdateFileRecords();
        }

        // Подменю для режима просмотра
        if (ImGui::BeginMenu("View Mode")) {
            if (ImGui::MenuItem("List View", nullptr, viewMode_ == ViewMode_List)) {
                viewMode_ = ViewMode_List;
            }
            if (ImGui::MenuItem("Grid View", nullptr, viewMode_ == ViewMode_Grid)) {
                viewMode_ = ViewMode_Grid;
            }
            ImGui::EndMenu();
        }

        // Раздел фильтров (если есть)
        if (!typeFilters_.empty()) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.86f, 0.77f, 0.62f, 1.0f), "FILE TYPE");

            for (size_t i = 0; i < typeFilters_.size(); ++i) {
                bool isSelected = (typeFilterIndex_ == i);
                if (ImGui::MenuItem(typeFilters_[i].c_str(), nullptr, isSelected)) {
                    typeFilterIndex_ = static_cast<unsigned int>(i);
                    UpdateFileRecords();
                }
            }
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}

void FileBrowser::RenderSubHeader(float titleBarHeight, float subHeaderHeight) {
    RenderAdaptivePath(titleBarHeight, subHeaderHeight);
}

bool FileBrowser::IsAtRoot() const {
#ifdef _WIN32
    // В Windows корень - это список дисков ИЛИ корень диска (C:\)
    // без возможности подняться выше
    return currentDirectory_ == DRIVES_PATH ||
        (currentDirectory_.has_root_name() &&
            currentDirectory_.parent_path() == currentDirectory_);
#else
    return currentDirectory_ == "/";
#endif
}

bool FileBrowser::IsDriveRoot() const {
#ifdef _WIN32
    // Проверяем, находимся ли мы в корне диска (C:\)
    return currentDirectory_.has_root_name() &&
        currentDirectory_.has_root_directory() &&
        std::distance(currentDirectory_.begin(), currentDirectory_.end()) == 2;
#else
    return false;
#endif
}

bool FileBrowser::IsDrivesView() const {
#ifdef _WIN32
    return currentDirectory_ == DRIVES_PATH;
#else
    return false;
#endif
}

void FileBrowser::NavigateUp() {
    if (IsDrivesView()) {
        CloseCurrentPopup();
        return;
    }

#ifdef _WIN32
    if (IsDriveRoot()) {
        // Переход из корня диска в список дисков
        SetDirectory(DRIVES_PATH);
        return;
    }
#endif

    if (!IsAtRoot()) {
        SetDirectory(currentDirectory_.parent_path());
    } else {
        // Для корня файловой системы (не-Windows) закрываем диалог
        CloseCurrentPopup();
    }
}

// FileBrowser.cpp
bool FileBrowser::IsFileSystemRoot() const {
#ifdef _WIN32
    return false; // В Windows нет единого корня
#else
    return currentDirectory_ == "/";
#endif
}

// FileBrowser.cpp
void FileBrowser::UpdateFullPathCache() const {
#ifdef _WIN32
    if (IsDrivesView()) {
        cachedFullPath_ = "My Computer";
        fullPathWidth_ = ImGui::CalcTextSize(cachedFullPath_.c_str()).x;
        return;
    }

    // Для Windows заменяем слеши на обратные
    std::string path = currentDirectory_.string();
    std::replace(path.begin(), path.end(), '/', '\\');
    cachedFullPath_ = path;
#else
    cachedFullPath_ = currentDirectory_.string();
#endif

    fullPathWidth_ = ImGui::CalcTextSize(cachedFullPath_.c_str()).x;
}

void FileBrowser::RenderAdaptivePath(float titleBarHeight, float subHeaderHeight) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    const float padding = Scale(16.0f);

           // Позиция подзаголовка
    ImVec2 subHeaderPos(
        windowPos.x,
        windowPos.y + titleBarHeight
        );

           // Область для пути (с возможностью скролла)
    ImVec2 pathAreaMin(subHeaderPos.x + padding, subHeaderPos.y);
    ImVec2 pathAreaMax(
        subHeaderPos.x + windowSize.x - padding,
        subHeaderPos.y + subHeaderHeight
        );
    const float pathAreaWidth = pathAreaMax.x - pathAreaMin.x;

           // Обновляем кэш пути
    UpdateFullPathCache();

    // Обработка скролла
    ImGuiIO& io = ImGui::GetIO();
    const bool isPathHovered = ImGui::IsWindowHovered() &&
        ImRect(pathAreaMin, pathAreaMax).Contains(io.MousePos);

    if (isPathHovered && io.MouseDown[0] && !isPathDragging_) {
        isPathDragging_ = true;
        pathDragStartPos_ = io.MousePos;
        pathDragStartOffset_ = pathScrollOffset_;
    }

    if (isPathDragging_ && io.MouseDown[0]) {
        const float dragDelta = pathDragStartPos_.x - io.MousePos.x;
        pathScrollOffset_ = pathDragStartOffset_ + dragDelta;
    } else {
        isPathDragging_ = false;
    }

    // Ограничение скролла
    const float maxScroll = std::max(0.0f, fullPathWidth_ - pathAreaWidth);
    pathScrollOffset_ = ImClamp(pathScrollOffset_, 0.0f, maxScroll);

    // Рисуем путь с обрезкой
    drawList->PushClipRect(pathAreaMin, pathAreaMax, true);

    // Позиция текста (с учетом скролла)
    ImVec2 textPos(
        pathAreaMin.x - pathScrollOffset_,
        pathAreaMin.y + (subHeaderHeight - ImGui::GetTextLineHeight()) * 0.5f
        );

    // Основной текст пути
    drawList->AddText(
        textPos,
        ImGui::GetColorU32(ImVec4(0.86f, 0.77f, 0.62f, 0.8f)),
        cachedFullPath_.c_str()
        );

    // Визуальные индикаторы скролла
    if (fullPathWidth_ > pathAreaWidth) {
        // Левый индикатор (если есть контент слева)
        if (pathScrollOffset_ > 0) {
            drawList->AddRectFilledMultiColor(
                pathAreaMin,
                ImVec2(pathAreaMin.x + 20, pathAreaMax.y),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 1.0f)),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 0.0f)),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 0.0f)),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 1.0f))
                );
        }

        // Правый индикатор (если есть контент справа)
        if (pathScrollOffset_ < maxScroll) {
            drawList->AddRectFilledMultiColor(
                ImVec2(pathAreaMax.x - 20, pathAreaMin.y),
                pathAreaMax,
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 0.0f)),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 1.0f)),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 1.0f)),
                ImGui::GetColorU32(ImVec4(0.12f, 0.06f, 0.06f, 0.0f))
                );
        }
    }

    drawList->PopClipRect();

    // Разделительная линия внизу подзаголовка
    drawList->AddLine(
        ImVec2(windowPos.x, pathAreaMax.y),
        ImVec2(windowPos.x + windowSize.x, pathAreaMax.y),
        ImGui::GetColorU32(ImVec4(0.6f, 0.1f, 0.1f, 1.0f)),
        1.0f
        );
}

void FileBrowser::SetTypeFilters(const std::vector<std::string>& typeFilters) {
    typeFilters_ = typeFilters;
    typeFilterIndex_ = 0;
    hasAllFilter_ = false;

    // Проверяем наличие фильтра "All Files"
    if (!typeFilters_.empty()) {
        for (const auto& filter : typeFilters_) {
            if (filter == ".*" || filter == "*" || filter == "*.*") {
                hasAllFilter_ = true;
                break;
            }
        }
    }

    UpdateFileRecords();
}

void FileBrowser::SetCurrentTypeFilterIndex(int index) {
    if (index >= 0 && index < static_cast<int>(typeFilters_.size())) {
        typeFilterIndex_ = static_cast<unsigned int>(index);
        UpdateFileRecords();
    }
}

float FileBrowser::Scale(float value) const
{
    return value * ImGui::GetIO().FontGlobalScale;
}

ImVec2 FileBrowser::Scale(ImVec2 vec) const
{
    return vec * ImGui::GetIO().FontGlobalScale;
}

} // namespace ImGui
