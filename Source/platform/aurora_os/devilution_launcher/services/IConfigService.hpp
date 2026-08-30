#pragma once

#include <filesystem>
#include <optional>

namespace launcher {

/// Launcher settings persisted between runs.
struct LauncherConfig {
	/// Folder with the user's game files (DIABDAT.MPQ, hellfire*.mpq).
	/// Empty when the user has not picked one yet.
	std::optional<std::filesystem::path> dataFolder;
};

/// Persistence for LauncherConfig. Implementations must be cheap to
/// construct; save() writes the whole file atomically (temp + rename).
class IConfigService {
public:
	virtual ~IConfigService() = default;

	[[nodiscard]] virtual LauncherConfig load() = 0;
	virtual void save(const LauncherConfig &config) = 0;
};

} // namespace launcher
