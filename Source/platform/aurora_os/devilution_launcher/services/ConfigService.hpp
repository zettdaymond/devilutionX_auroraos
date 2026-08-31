#pragma once

#include "IConfigService.hpp"

namespace launcher {

/// Настройки в TOML (launcher.conf). Файл записывается атомарно:
/// сначала во временный файл рядом с целевым, затем переименовывается.
class ConfigService final : public IConfigService {
public:
	explicit ConfigService(std::filesystem::path configFilePath);

	[[nodiscard]] LauncherConfig Load() override;
	void Save(const LauncherConfig &config) override;

private:
	std::filesystem::path m_filePath;
};

} // namespace launcher
