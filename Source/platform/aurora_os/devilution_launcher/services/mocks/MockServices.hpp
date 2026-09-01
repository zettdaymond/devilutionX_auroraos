#pragma once

#include "services/IConfigService.hpp"
#include "services/IEngineOptionsService.hpp"
#include "services/IGameFilesService.hpp"

#include "MockWorld.hpp"

#include <memory>
#include <vector>

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

/// Настройки движка в памяти; помнит все вызовы SaveAll, чтобы тест
/// проверял и значения, и количество записей.
class MockEngineOptionsService final : public IEngineOptionsService {
public:
	[[nodiscard]] std::array<int, kSettingCount> Load() override { return m_values; }
	void SaveAll(const std::array<int, kSettingCount> &values) override
	{
		m_values = values;
		m_saves.push_back(values);
	}

	[[nodiscard]] const std::vector<std::array<int, kSettingCount>> &Saves() const { return m_saves; }

private:
	std::array<int, kSettingCount> m_values = DefaultSettingValues();
	std::vector<std::array<int, kSettingCount>> m_saves;
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
