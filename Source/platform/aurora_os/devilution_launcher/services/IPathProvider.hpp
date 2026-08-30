#pragma once

#include "IConfigService.hpp"

#include <filesystem>
#include <vector>

namespace launcher {

/// Platform-specific locations for the launcher. This is the single
/// seam between portable launcher code and the host platform
/// (Aurora OS with Qt vs desktop SDL).
class IPathProvider {
public:
	virtual ~IPathProvider() = default;

	/// Directory for launcher housekeeping files (launcher.conf, imgui.ini).
	[[nodiscard]] virtual std::filesystem::path configDir() = 0;

	/// Writable directory downloadable content (spawn.mpq, ru.mpq)
	/// is placed into. It is also an MPQ search path of the game itself.
	[[nodiscard]] virtual std::filesystem::path downloadsDir() = 0;

	/// Folders scanned for game files, in priority order:
	/// user-selected folder first, then platform locations.
	[[nodiscard]] virtual std::vector<std::filesystem::path> candidateDataDirs(const LauncherConfig &config) = 0;
};

} // namespace launcher
