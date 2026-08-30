#include "MockWorld.hpp"

#include <algorithm>
#include <utility>

namespace launcher {

namespace {

constexpr int64_t kFakeOriginalSize = 500LL * 1024 * 1024; // MPQs "from the user's CD"
constexpr int64_t kFakeFreeSpace = 8LL * 1024 * 1024 * 1024; // 8 GB

const std::vector<std::string> kPresets {
	"empty",
	"diablo-found",
	"hellfire-partial",
	"full",
};

} // namespace

MockWorld::MockWorld()
{
	m_files.fill(-1);
	m_freeSpaceBytes = kFakeFreeSpace;
}

void MockWorld::setPresent(KnownFile file, int64_t sizeBytes)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_files[static_cast<size_t>(file)] = sizeBytes;
}

void MockWorld::setAbsent(KnownFile file)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_files[static_cast<size_t>(file)] = -1;
}

bool MockWorld::isPresent(KnownFile file) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_files[static_cast<size_t>(file)] >= 0;
}

void MockWorld::setFreeSpace(int64_t bytes)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_freeSpaceBytes = bytes;
}

MockWorld::Snapshot MockWorld::take() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	Snapshot snapshot;
	snapshot.files = m_files;
	snapshot.freeSpaceBytes = m_freeSpaceBytes;
	return snapshot;
}

void MockWorld::applyDownloadFinished(KnownFile file, bool success)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (success) {
		const int64_t size = std::max<int64_t>(FileSpecOf(file).expectedSizeBytes, 1024);
		m_files[static_cast<size_t>(file)] = size;
		m_freeSpaceBytes = std::max<int64_t>(m_freeSpaceBytes - size, 0);
	} else {
		m_files[static_cast<size_t>(file)] = -1;
	}
}

bool MockWorld::applyPreset(const std::string &name)
{
	if (std::find(kPresets.begin(), kPresets.end(), name) == kPresets.end()) {
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	m_files.fill(-1);
	m_freeSpaceBytes = kFakeFreeSpace;

	if (name == "diablo-found") {
		m_files[static_cast<size_t>(KnownFile::Diabdat)] = kFakeOriginalSize;
	} else if (name == "hellfire-partial") {
		m_files[static_cast<size_t>(KnownFile::Diabdat)] = kFakeOriginalSize;
		m_files[static_cast<size_t>(KnownFile::Hellfire)] = kFakeOriginalSize;
		m_files[static_cast<size_t>(KnownFile::HfMonk)] = kFakeOriginalSize;
	} else if (name == "full") {
		for (size_t i = 0; i < kKnownFileCount; ++i) {
			const FileSpec &spec = kFileCatalog[i];
			m_files[i] = spec.expectedSizeBytes > 0 ? spec.expectedSizeBytes : kFakeOriginalSize;
		}
	}
	return true;
}

const std::vector<std::string> &MockWorld::presetNames()
{
	return kPresets;
}

} // namespace launcher
