#pragma once

#include "core/GameFiles.hpp"

#include <filesystem>
#include <vector>

namespace launcher {

/// Result of scanning one or more folders for known game files.
/// sizes[i] is the file size in bytes, or -1 when the file was not found.
struct FileScanResult {
	std::array<int64_t, kKnownFileCount> sizes {};
	std::array<std::filesystem::path, kKnownFileCount> folders {}; // folder the file was found in
};

/// File system access for game data. All methods are synchronous and
/// safe to call on the main thread (folders are small).
class IGameFilesService {
public:
	virtual ~IGameFilesService() = default;

	/// Scan folders in order; the first hit per file wins.
	/// Comparison of file names is case-insensitive.
	[[nodiscard]] virtual FileScanResult Scan(const std::vector<std::filesystem::path> &folders) = 0;

	/// Free space on the filesystem the path lives on, in bytes.
	[[nodiscard]] virtual int64_t FreeSpace(const std::filesystem::path &dir) = 0;

	/// Remove a known file from dir. Returns true when the file existed
	/// and was removed.
	virtual bool RemoveFile(const std::filesystem::path &dir, KnownFile file) = 0;
};

} // namespace launcher
