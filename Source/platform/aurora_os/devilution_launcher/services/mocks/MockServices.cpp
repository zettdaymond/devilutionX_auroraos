#include "MockServices.hpp"

namespace launcher {

MockGameFilesService::MockGameFilesService(std::shared_ptr<MockWorld> world)
    : m_world(std::move(world))
{
}

FileScanResult MockGameFilesService::scan(const std::vector<std::filesystem::path> &folders)
{
	const MockWorld::Snapshot snapshot = m_world->take();

	FileScanResult result;
	result.sizes = snapshot.files;
	if (!folders.empty()) {
		result.folders.fill(folders.front());
	}
	return result;
}

int64_t MockGameFilesService::freeSpace(const std::filesystem::path &)
{
	return m_world->take().freeSpaceBytes;
}

bool MockGameFilesService::removeFile(const std::filesystem::path &, KnownFile file)
{
	const bool existed = m_world->isPresent(file);
	m_world->setAbsent(file);
	return existed;
}

} // namespace launcher
