#pragma once

#include "IGameFilesService.hpp"

namespace launcher {

/// std::filesystem-based implementation, portable between Aurora OS
/// and desktop builds.
class GameFilesService final : public IGameFilesService {
public:
	[[nodiscard]] FileScanResult Scan(const std::vector<std::filesystem::path> &folders) override;
	[[nodiscard]] int64_t FreeSpace(const std::filesystem::path &dir) override;
	bool RemoveFile(const std::filesystem::path &dir, KnownFile file) override;
};

} // namespace launcher
