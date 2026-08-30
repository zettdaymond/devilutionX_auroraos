#include "DesktopPathProvider.hpp"

#include <utility>

namespace launcher {

DesktopPathProvider::DesktopPathProvider(std::filesystem::path baseDir)
    : m_baseDir(std::move(baseDir))
{
}

std::filesystem::path DesktopPathProvider::configDir()
{
	return m_baseDir;
}

std::filesystem::path DesktopPathProvider::downloadsDir()
{
	return m_baseDir;
}

std::vector<std::filesystem::path> DesktopPathProvider::candidateDataDirs(const LauncherConfig &config)
{
	std::vector<std::filesystem::path> dirs;
	if (config.dataFolder) {
		dirs.push_back(*config.dataFolder);
	}
	dirs.push_back(m_baseDir);
	return dirs;
}

} // namespace launcher
