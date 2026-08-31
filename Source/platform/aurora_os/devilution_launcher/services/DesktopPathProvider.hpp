#pragma once

#include "IPathProvider.hpp"

namespace launcher {

/// Переносимый вариант путей: всё живёт в одной базовой папке
/// (в десктопной сборке — путь SDL, его передаёт вызывающий код;
/// сам класс не зависит от SDL).
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
