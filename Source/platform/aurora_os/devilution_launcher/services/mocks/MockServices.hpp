#pragma once

#include "services/IConfigService.hpp"
#include "services/IGameFilesService.hpp"

#include "MockWorld.hpp"

#include <memory>

namespace launcher {

/// In-memory config; records the last saved value so tests can assert
/// on what the Store persisted.
class MockConfigService final : public IConfigService {
public:
	[[nodiscard]] LauncherConfig load() override { return m_stored; }
	void save(const LauncherConfig &config) override { m_stored = config; }

	[[nodiscard]] const LauncherConfig &lastSaved() const { return m_stored; }

private:
	LauncherConfig m_stored;
};

/// File "system" backed by MockWorld: every scanned folder reports the
/// same scripted contents, freeSpace mirrors the world, removeFile
/// clears the entry. Owns a share of the world so the bundle stays
/// valid regardless of the MockScenario's lifetime.
class MockGameFilesService final : public IGameFilesService {
public:
	explicit MockGameFilesService(std::shared_ptr<MockWorld> world);

	[[nodiscard]] FileScanResult scan(const std::vector<std::filesystem::path> &folders) override;
	[[nodiscard]] int64_t freeSpace(const std::filesystem::path &dir) override;
	bool removeFile(const std::filesystem::path &dir, KnownFile file) override;

private:
	std::shared_ptr<MockWorld> m_world;
};

} // namespace launcher
