#pragma once

#include <filesystem>

namespace launcher {

/// What the launcher was asked to start when its UI loop finished.
enum class ExitAction {
	LaunchDiablo,
	LaunchHellfire,
	LaunchDemo,
};

/// Final answer of the launcher to the host application
/// (see Source/main.cpp — it maps the action to engine CLI flags).
struct AppResult {
	bool success = false;
	ExitAction action = ExitAction::LaunchDiablo;
	std::filesystem::path dataPath; // folder containing the user's MPQ files
};

} // namespace launcher
