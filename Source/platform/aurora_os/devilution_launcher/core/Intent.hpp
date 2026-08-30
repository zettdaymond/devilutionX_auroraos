#pragma once

#include "AppResult.hpp"
#include "GameFiles.hpp"
#include "LauncherState.hpp"

#include <filesystem>
#include <string>
#include <variant>

namespace launcher {

/// User actions and internal events, expressed as small value types.
/// The view dispatches these into the Store; the Store is the only
/// place that translates them into state changes and side effects.
namespace intent {

// -- UI navigation --
struct UiNavigate {
	Screen screen;
};
struct UiOpenDialog {
	Dialog dialog;
};
struct UiCloseDialog {};
/// The view dismissed the transient toast notification.
struct UiDismissToast {};

// -- Game files --
/// Opens the file browser so the user can point the launcher at a folder
/// with their MPQ files.
struct SelectDataFolder {};
/// The user confirmed a folder in the file browser.
struct DataFolderSelected {
	std::filesystem::path dir;
};
/// The user closed the file browser without choosing a folder.
struct CancelFolderSelection {};
/// Re-scan all candidate folders (after external changes, downloads, deletions).
struct RescanFiles {};
/// Delete a previously downloaded file (spawn.mpq / ru.mpq).
struct DeleteDownloadedFile {
	KnownFile file;
};

// -- Launching --
/// The user tapped a game card.
struct LaunchGame {
	ExitAction game;
};

// -- Downloads --
/// Start downloading (after the user confirmed size / disk space).
struct StartDownload {
	KnownFile file;
};
struct CancelDownload {};

// -- Internal events marshalled from the download thread --
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
    intent::EvDownloadProgress,
    intent::EvDownloadFinished>;

} // namespace launcher
