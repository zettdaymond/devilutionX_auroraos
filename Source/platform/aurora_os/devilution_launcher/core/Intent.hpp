#pragma once

#include "AppResult.hpp"
#include "GameFiles.hpp"
#include "LauncherState.hpp"

#include <filesystem>
#include <string>
#include <variant>

namespace launcher {

/// Действия пользователя и внутренние события — маленькие типы-значения.
/// Интерфейс отправляет их в Store; только Store превращает их
/// в изменения состояния и побочные эффекты.
namespace intent {

// -- UI navigation --
struct UiNavigate {
	Screen screen;
};
struct UiOpenDialog {
	Dialog dialog;
};
struct UiCloseDialog {};
/// Интерфейс закрыл короткое всплывающее уведомление.
struct UiDismissToast {};

// -- Game files --
/// Открыть файловый браузер, чтобы пользователь указал папку
/// со своими MPQ-файлами.
struct SelectDataFolder {};
/// Пользователь выбрал папку в файловом браузере.
struct DataFolderSelected {
	std::filesystem::path dir;
};
/// Пользователь закрыл браузер, не выбрав папку.
struct CancelFolderSelection {};
/// Пересканировать все папки-кандидаты (после изменений, загрузок, удалений).
struct RescanFiles {};
/// Удалить ранее скачанный файл (spawn.mpq / ru.mpq).
struct DeleteDownloadedFile {
	KnownFile file;
};

// -- Launching --
/// Пользователь нажал на карточку игры.
struct LaunchGame {
	ExitAction game;
};

// -- Downloads --
/// Начать загрузку (после подтверждения размера и свободного места).
struct StartDownload {
	KnownFile file;
};
struct CancelDownload {};

// -- Настройки движка --
/// Пользователь поменял одну настройку; value — в единицах каталога
/// (bool 0/1, проценты для громкостей).
struct SettingChanged {
	SettingId setting;
	int value;
};
/// Сбросить все настройки к значениям по умолчанию.
struct SettingsReset {};

// -- Внутренние события, переданные из потока загрузки --
struct EvDownloadProgress {
	int64_t totalBytes;
	int64_t downloadedBytes;
	int64_t bytesPerSec;
};
struct EvDownloadFinished {
	bool success;
	std::string error;
};

} // namespace intent

using Intent = std::variant<
    intent::UiNavigate,
    intent::UiOpenDialog,
    intent::UiCloseDialog,
    intent::UiDismissToast,
    intent::SelectDataFolder,
    intent::DataFolderSelected,
    intent::CancelFolderSelection,
    intent::RescanFiles,
    intent::DeleteDownloadedFile,
    intent::LaunchGame,
    intent::StartDownload,
    intent::CancelDownload,
    intent::SettingChanged,
    intent::SettingsReset,
    intent::EvDownloadProgress,
    intent::EvDownloadFinished>;

} // namespace launcher
