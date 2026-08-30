#include "AuroraPathProvider.hpp"

#include "StandartPaths.hpp"

#include <utility>

namespace launcher {

AuroraPathProvider::AuroraPathProvider(std::filesystem::path baseDir)
    : m_baseDir(std::move(baseDir))
{
}

std::filesystem::path AuroraPathProvider::configDir()
{
	return m_baseDir;
}

std::filesystem::path AuroraPathProvider::downloadsDir()
{
	return std::filesystem::path { devilution::AuroraOsStandartPaths::GetAdditionalMPQSearchPath() };
}

std::vector<std::filesystem::path> AuroraPathProvider::candidateDataDirs(const LauncherConfig &config)
{
	std::vector<std::filesystem::path> dirs;
	if (config.dataFolder) {
		dirs.push_back(*config.dataFolder);
	}
	dirs.push_back(downloadsDir());
	dirs.emplace_back(devilution::AuroraOsStandartPaths::GetBundledAssetsPath());
	return dirs;
}

} // namespace launcher
