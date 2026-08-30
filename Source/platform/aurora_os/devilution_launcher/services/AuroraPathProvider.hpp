#pragma once

#include "IPathProvider.hpp"

namespace launcher {

/// Aurora OS path provider. Wraps AuroraOsStandartPaths (Qt-based) so
/// the rest of the launcher stays Qt-free:
/// - configDir: base dir passed in (SDL preference path on device);
/// - downloadsDir: the engine's additional MPQ search path
///   (~/.local/share/org.diasurgical/devilutionx), so downloaded
///   spawn.mpq / ru.mpq are found by the game without extra setup;
/// - candidate dirs also include the read-only bundled assets folder.
class AuroraPathProvider final : public IPathProvider {
public:
	explicit AuroraPathProvider(std::filesystem::path baseDir);

	[[nodiscard]] std::filesystem::path configDir() override;
	[[nodiscard]] std::filesystem::path downloadsDir() override;
	[[nodiscard]] std::vector<std::filesystem::path> candidateDataDirs(const LauncherConfig &config) override;

private:
	std::filesystem::path m_baseDir;
};

} // namespace launcher
