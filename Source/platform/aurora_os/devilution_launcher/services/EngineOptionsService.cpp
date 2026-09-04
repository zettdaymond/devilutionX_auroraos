#include "EngineOptionsService.hpp"

#include <SimpleIni.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <utility>

namespace launcher {

namespace {

/// Настраивает парсер в точности как движок (Source/options.cpp GetIni):
/// без пробелов вокруг '=' и с поддержкой повторяющихся ключей — иначе
/// следующий SaveOptions() движка не смог бы перезаписать наши ключи.
void ConfigureIni(CSimpleIniA &ini)
{
	ini.SetSpaces(false);
	ini.SetMultiKey();
}

/// Загружает файл, если он существует и читается; false — начать
/// с пустого документа (файла может ещё не быть при первом запуске).
bool LoadIniFile(CSimpleIniA &ini, const std::filesystem::path &path)
{
	const SI_Error rc = ini.LoadFile(path.string().c_str());
	return rc >= SI_OK;
}

} // namespace

EngineOptionsService::EngineOptionsService(std::filesystem::path iniPath)
    : m_iniPath(std::move(iniPath))
{
}

void EngineOptionsService::SetResolutionAspect(int landscapeWidth, int landscapeHeight)
{
	if (landscapeWidth > 0 && landscapeHeight > 0) {
		m_aspectWidth = landscapeWidth;
		m_aspectHeight = landscapeHeight;
	}
}

std::array<int, kSettingCount> EngineOptionsService::Load()
{
	std::array<int, kSettingCount> values = DefaultSettingValues();

	CSimpleIniA ini;
	ConfigureIni(ini);
	if (!LoadIniFile(ini, m_iniPath)) {
		if (std::filesystem::exists(m_iniPath)) {
			spdlog::warn("Failed to parse engine ini {}, using defaults", m_iniPath.string());
		}
		return values;
	}

	for (size_t i = 0; i < kSettingCount; ++i) {
		const SettingSpec &spec = kSettingCatalog[i];
		switch (spec.kind) {
		case SettingKind::Toggle:
			values[i] = ini.GetBoolValue(spec.section.data(), spec.key.data(), spec.defaultInt != 0) ? 1 : 0;
			break;
		case SettingKind::PercentVolume:
			values[i] = VolumeIniToPct(static_cast<int>(ini.GetLongValue(spec.section.data(), spec.key.data(),
			                                   VolumePctToIni(spec.defaultInt))));
			break;
		case SettingKind::Slider:
		case SettingKind::Cycle:
			values[i] = static_cast<int>(ini.GetLongValue(spec.section.data(), spec.key.data(), spec.defaultInt));
			break;
		}
		// Разрешение (двухключевой Cycle) читается КАК ЕСТЬ: список
		// вариантов — зеркало игрового и всегда содержит текущее
		// значение; снап/clamp молча портил бы выбор пользователя.
	}
	return values;
}

void EngineOptionsService::SaveAll(const std::array<int, kSettingCount> &values)
{
	// Перечитываем файл: в нём лежат ключи движка, которые лаунчер
	// трогать не имеет права, — они должны пережить нашу запись.
	CSimpleIniA ini;
	ConfigureIni(ini);
	const bool hadFile = LoadIniFile(ini, m_iniPath);
	if (!hadFile && std::filesystem::exists(m_iniPath)) {
		spdlog::error("Failed to parse engine ini {}, refusing to overwrite", m_iniPath.string());
		return;
	}

	for (size_t i = 0; i < kSettingCount; ++i) {
		const SettingSpec &spec = kSettingCatalog[i];
		int value = values[i];
		if (spec.kind == SettingKind::PercentVolume) {
			value = VolumePctToIni(value);
		} else if (spec.secondaryKey.empty()) {
			// Разрешение не clamp-им: сырое значение из ini переживёт
			// запись любых других настроек нетронутым.
			value = std::clamp(value, spec.minValue, spec.maxValue);
		}
		// Как движок: десятичная запись, булевы — 1/0, дубликаты заменяются.
		ini.SetLongValue(spec.section.data(), spec.key.data(), value, nullptr, false, true);
		// Двухключевая настройка (разрешение): Width пересчитывается из
		// Height по аспекту экрана — ровно как строит список сам движок
		// при fitToScreen (options.cpp: size.width = size.height * mode.w / mode.h).
		if (!spec.secondaryKey.empty()) {
			const int width = static_cast<int>(
			    std::int64_t { value } * m_aspectWidth / m_aspectHeight);
			ini.SetLongValue(spec.section.data(), spec.secondaryKey.data(), width, nullptr, false, true);
		}
	}

	std::error_code ec;
	std::filesystem::create_directories(m_iniPath.parent_path(), ec);

	const std::string tmp = m_iniPath.string() + ".tmp";
	if (ini.SaveFile(tmp.c_str()) < SI_OK) {
		spdlog::error("Failed to write {}", tmp);
		return;
	}
	std::filesystem::rename(tmp, m_iniPath, ec);
	if (ec) {
		// Переименование поверх существующего файла иногда невозможно —
		// пишем напрямую (как ConfigService).
		if (ini.SaveFile(m_iniPath.string().c_str()) < SI_OK) {
			spdlog::error("Failed to write {}", m_iniPath.string());
		}
	}
}

} // namespace launcher
