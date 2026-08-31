#pragma once

#include "services/IConfigService.hpp"
#include "services/IGameFilesService.hpp"

#include "MockWorld.hpp"

#include <memory>

namespace launcher {

/// Настройки в памяти; запоминает последнюю сохранённую версию,
/// чтобы тест проверял, что именно Store записал.
class MockConfigService final : public IConfigService {
public:
	[[nodiscard]] LauncherConfig Load() override { return m_stored; }
	void Save(const LauncherConfig &config) override { m_stored = config; }

	[[nodiscard]] const LauncherConfig &LastSaved() const { return m_stored; }

private:
	LauncherConfig m_stored;
};

/// Файловая «система» поверх MockWorld: любая просканированная папка
/// выдаёт одно и то же предписанное содержимое, свободное место —
/// из «мира», удаление убирает запись. Держит долю владения «миром»
/// ради времени жизни набора сервисов.
class MockGameFilesService final : public IGameFilesService {
public:
	explicit MockGameFilesService(std::shared_ptr<MockWorld> world);

	[[nodiscard]] FileScanResult Scan(const std::vector<std::filesystem::path> &folders) override;
	[[nodiscard]] int64_t FreeSpace(const std::filesystem::path &dir) override;
	bool RemoveFile(const std::filesystem::path &dir, KnownFile file) override;

private:
	std::shared_ptr<MockWorld> m_world;
};

} // namespace launcher
