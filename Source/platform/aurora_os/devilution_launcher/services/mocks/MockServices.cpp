#include "MockServices.hpp"

namespace launcher {

MockGameFilesService::MockGameFilesService(std::shared_ptr<MockWorld> world)
    : m_world(std::move(world))
{
}

FileScanResult MockGameFilesService::Scan(const std::vector<std::filesystem::path> &folders)
{
	const MockWorld::Snapshot snapshot = m_world->Take();

	FileScanResult result;
	result.sizes = snapshot.files;
	if (!folders.empty()) {
		result.folders.fill(folders.front());
	}
	return result;
}

int64_t MockGameFilesService::FreeSpace(const std::filesystem::path &)
{
	return m_world->Take().freeSpaceBytes;
}

bool MockGameFilesService::RemoveFile(const std::filesystem::path &, KnownFile file)
{
	const bool existed = m_world->IsPresent(file);
	m_world->SetAbsent(file);
	return existed;
}

} // namespace launcher
