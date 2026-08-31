#pragma once

#include "IPathProvider.hpp"

namespace launcher {

/// Portable path provider: everything lives under one base directory
/// (on desktop builds the SDL preference path, supplied by the caller —
/// this class stays free of SDL dependencies).
class DesktopPathProvider final : public IPathProvider {
public:
	explicit DesktopPathProvider(std::filesystem::path baseDir);

	[[nodiscard]] std::filesystem::path ConfigDir() override;
	[[nodiscard]] std::filesystem::path DownloadsDir() override;
	[[nodiscard]] std::vector<std::filesystem::path> CandidateDataDirs(const LauncherConfig &config) override;

private:
	std::filesystem::path m_baseDir;
};

} // namespace launcher
