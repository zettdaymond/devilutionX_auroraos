#pragma once

#include "core/GameFiles.hpp"

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace launcher {

/// Shared fake environment for mock services: which files "exist",
/// their sizes and the fake free disk space. MockGameFilesService
/// reads it; MockDownloadService updates it on success/cancel.
/// Thread-safe because the download simulation runs on its own thread.
class MockWorld {
public:
	MockWorld();

	void setPresent(KnownFile file, int64_t sizeBytes);
	void setAbsent(KnownFile file);
	[[nodiscard]] bool isPresent(KnownFile file) const;

	void setFreeSpace(int64_t bytes);

	struct Snapshot {
		std::array<int64_t, kKnownFileCount> files {};
		int64_t freeSpaceBytes = 0;
	};

	/// Consistent copy of the whole world state.
	[[nodiscard]] Snapshot take() const;

	void applyDownloadFinished(KnownFile file, bool success);

	/// Apply a named preset ("empty", "diablo-found", "hellfire-partial",
	/// "full"). Returns false for unknown names.
	bool applyPreset(const std::string &name);

	/// All valid preset names, for --help output.
	static const std::vector<std::string> &presetNames();

private:
	mutable std::mutex m_mutex;
	std::array<int64_t, kKnownFileCount> m_files {};
	int64_t m_freeSpaceBytes = 0;
};

} // namespace launcher
