#pragma once

#include "IEngineOptionsService.hpp"

#include <filesystem>

namespace launcher {

/// Портативная реализация поверх SimpleIni — той же библиотеки, которой
/// пользуется движок, с теми же флагами формата.
class EngineOptionsService : public IEngineOptionsService {
public:
	/// Путь до diablo.ini движка (платформозависимую папку вычисляет вызывающий).
	explicit EngineOptionsService(std::filesystem::path iniPath);

	[[nodiscard]] std::array<int, kSettingCount> Load() override;
	void SaveAll(const std::array<int, kSettingCount> &values) override;

private:
	std::filesystem::path m_iniPath;
};

} // namespace launcher
