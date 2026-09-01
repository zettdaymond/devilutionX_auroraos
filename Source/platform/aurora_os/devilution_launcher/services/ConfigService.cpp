#include "ConfigService.hpp"

#include <SimpleIni.h>

#include <spdlog/spdlog.h>

#include <utility>

namespace launcher {

namespace {

/// Те же флаги формата, что у движкового diablo.ini: одна библиотека,
/// один стиль на оба конфига лаунчера.
void ConfigureIni(CSimpleIniA &ini)
{
	ini.SetSpaces(false);
	ini.SetMultiKey();
}

} // namespace

ConfigService::ConfigService(std::filesystem::path configFilePath)
    : m_filePath(std::move(configFilePath))
{
}

LauncherConfig ConfigService::Load()
{
	LauncherConfig config;

	CSimpleIniA ini;
	ConfigureIni(ini);
	const SI_Error rc = ini.LoadFile(m_filePath.string().c_str());
	if (rc < SI_OK) {
		if (std::filesystem::exists(m_filePath)) {
			spdlog::warn("Failed to parse {}, using defaults", m_filePath.string());
		}
		return config;
	}

	const char *folder = ini.GetValue("Storage", "DataFolder", nullptr);
	if (folder != nullptr && folder[0] != '\0') {
		config.dataFolder = std::filesystem::path(folder);
	}
	return config;
}

void ConfigService::Save(const LauncherConfig &config)
{
	// Файл целиком наш — читаем перед записью только ради сохранения
	// чужих ключей, если их когда-нибудь добавят.
	CSimpleIniA ini;
	ConfigureIni(ini);
	const SI_Error rc = ini.LoadFile(m_filePath.string().c_str());
	if (rc < SI_OK && std::filesystem::exists(m_filePath)) {
		spdlog::error("Failed to parse {}, refusing to overwrite", m_filePath.string());
		return;
	}

	if (config.dataFolder.has_value()) {
		ini.SetValue("Storage", "DataFolder", config.dataFolder->string().c_str());
	} else {
		ini.Delete("Storage", "DataFolder");
	}

	std::error_code ec;
	std::filesystem::create_directories(m_filePath.parent_path(), ec);

	const std::string tmp = m_filePath.string() + ".tmp";
	if (ini.SaveFile(tmp.c_str()) < SI_OK) {
		spdlog::error("Failed to write {}", tmp);
		return;
	}
	std::filesystem::rename(tmp, m_filePath, ec);
	if (ec) {
		// Переименование поверх существующего файла иногда невозможно —
		// пишем напрямую (как EngineOptionsService).
		if (ini.SaveFile(m_filePath.string().c_str()) < SI_OK) {
			spdlog::error("Failed to write {}", m_filePath.string());
		}
	}
}

} // namespace launcher
