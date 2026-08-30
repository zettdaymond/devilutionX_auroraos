#pragma once

#include "IGameFilesService.hpp"

namespace launcher {

/// std::filesystem-based implementation, portable between Aurora OS
/// and desktop builds.
class GameFilesService final : public IGameFilesService {
public:
	[[nodiscard]] FileScanResult scan(const std::vector<std::filesystem::path> &folders) override;
	[[nodiscard]] int64_t freeSpace(const std::filesystem::path &dir) override;
	bool removeFile(const std::filesystem::path &dir, KnownFile file) override;
};

} // namespace launcher
