#pragma once

#include "AppResult.hpp"
#include "GameFiles.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace launcher {

/// Top-level screen the launcher shows. Navigation is exclusive —
/// one screen at a time, plus at most one modal dialog.
enum class Screen : uint8_t {
	Home,
	Data,
	About,
};

/// Modal dialog on top of a screen.
enum class Dialog : uint8_t {
	None,
	ConfirmDownloadDemo,
	ConfirmDownloadRu,
	DownloadProgress,
	HellfireMissingFiles,
	Error,
};

/// Availability of one launchable game mode.
struct GameCardState {
	bool available = false;                     // every required file is present
	std::vector<KnownFile> missingFiles = {};   // what is missing (for the Hellfire warning)
};

/// State of a single download (demo or Russian voice pack).
struct DownloadState {
	KnownFile file = KnownFile::Spawn;
	bool active = false;
	float fraction = 0.0F;          // 0..1
	int64_t totalBytes = 0;
	int64_t downloadedBytes = 0;
	int64_t bytesPerSec = 0;
	std::string error;              // non-empty => failed panel in the overlay
};

/// Single source of truth for the whole launcher UI.
/// The view is a pure function of this struct; it is mutated
/// only by Store on the main thread.
struct LauncherState {
	// Navigation
	Screen screen = Screen::Home;
	Dialog dialog = Dialog::None;

	// Detected game files
	std::filesystem::path dataFolder;                                // user-selected folder (may be empty)
	std::array<int64_t, kKnownFileCount> fileSizes {};               // -1 = not found
	std::array<std::filesystem::path, kKnownFileCount> fileFolders {};// where the file was found
	int64_t freeDiskBytes = 0;

	// Derived availability
	GameCardState diablo;
	GameCardState hellfire;
	GameCardState demo;
	bool russianVoiceInstalled = false;

	// Download in flight
	std::optional<DownloadState> download;

	// Modal payload
	std::vector<KnownFile> hellfireMissing; // for Dialog::HellfireMissingFiles
	std::string errorText;                  // for Dialog::Error

	// Transient UI state
	bool fileBrowserOpen = false;
	bool exitRequested = false;             // window close / SDL_QUIT
	std::optional<ExitAction> pendingLaunch;// set => the UI loop ends and the game starts

	// Short-lived notification shown at the bottom of the screen
	std::optional<std::string> toast;

	[[nodiscard]] bool hasAnyFiles() const
	{
		for (int64_t size : fileSizes) {
			if (size >= 0)
				return true;
		}
		return false;
	}

	[[nodiscard]] bool downloadInProgress() const { return download.has_value() && download->active; }
};

} // namespace launcher
