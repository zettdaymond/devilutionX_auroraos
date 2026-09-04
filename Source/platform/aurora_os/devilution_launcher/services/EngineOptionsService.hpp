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

	/// Аспект экрана в ЛАНДШАФТНОЙ ориентации для вычисления Width по
	/// Height у двухключевой настройки «Разрешение» (как пересчитывает
	/// список разрешений сам движок при fitToScreen). Дефолт 16:9.
	void SetResolutionAspect(int landscapeWidth, int landscapeHeight) override;

private:
	std::filesystem::path m_iniPath;
	int m_aspectWidth = 16;
	int m_aspectHeight = 9;
};

} // namespace launcher
