#pragma once

#include "IConfigService.hpp"

namespace launcher {

/// TOML-backed config (launcher.conf). The file is written atomically:
/// first to a temporary file next to the target, then renamed over it.
class ConfigService final : public IConfigService {
public:
	explicit ConfigService(std::filesystem::path configFilePath);

	[[nodiscard]] LauncherConfig Load() override;
	void Save(const LauncherConfig &config) override;

private:
	std::filesystem::path m_filePath;
};

} // namespace launcher
