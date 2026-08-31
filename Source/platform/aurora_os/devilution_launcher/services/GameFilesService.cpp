#include "GameFilesService.hpp"

#include "core/GameFiles.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <system_error>

namespace launcher {

FileScanResult GameFilesService::Scan(const std::vector<std::filesystem::path> &folders)
{
	FileScanResult result;
	result.sizes.fill(-1);

	for (const auto &folder : folders) {
		std::error_code ec;
		if (!std::filesystem::exists(folder, ec) || ec) {
			continue;
		}

		for (auto it = std::filesystem::directory_iterator(folder, ec);
		     it != std::filesystem::directory_iterator(); it.increment(ec)) {
			if (ec) {
				break;
			}

			const auto &entry = *it;
			if (!entry.is_regular_file(ec)) {
				continue;
			}

			for (size_t i = 0; i < kKnownFileCount; ++i) {
				const KnownFile id = static_cast<KnownFile>(i);
				if (!IsKnownFileName(entry.path().filename().string(), id)) {
					continue;
				}
				if (result.sizes[i] >= 0) {
					continue; // already found in an earlier folder
				}
				result.sizes[i] = static_cast<int64_t>(entry.file_size(ec));
				if (ec) {
					result.sizes[i] = 0;
					ec.clear();
				}
				result.folders[i] = folder;
			}
		}
	}

	return result;
}

int64_t GameFilesService::FreeSpace(const std::filesystem::path &dir)
{
	std::error_code ec;
	const auto space = std::filesystem::space(dir, ec);
	if (ec) {
		spdlog::warn("freeSpace failed for {}: {}", dir.string(), ec.message());
		return -1;
	}
	return static_cast<int64_t>(space.available);
}

bool GameFilesService::RemoveFile(const std::filesystem::path &dir, KnownFile file)
{
	const auto path = dir / FileSpecOf(file).canonical.data();
	std::error_code ec;
	if (!std::filesystem::exists(path, ec) || ec) {
		return false;
	}
	std::filesystem::remove(path, ec);
	if (ec) {
		spdlog::error("Failed to remove {}: {}", path.string(), ec.message());
		return false;
	}
	return true;
}

} // namespace launcher
