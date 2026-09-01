#pragma once

#include "core/EngineOptions.hpp"

#include <array>

namespace launcher {

/// Доступ к движковым настройкам (diablo.ini). Лаунчер пишет их до
/// запуска игры, движок читает при старте — изменения применяются
/// к следующему запуску автоматически.
class IEngineOptionsService {
public:
	virtual ~IEngineOptionsService() = default;

	/// Читает значения всех настроек каталога; отсутствующие файл или
	/// ключи дают значения по умолчанию.
	[[nodiscard]] virtual std::array<int, kSettingCount> Load() = 0;

	/// Записывает все настройки, не трогая чужие ключи и секции
	/// (их пишет сам движок: раскладки, сеть, язык и т.д.).
	virtual void SaveAll(const std::array<int, kSettingCount> &values) = 0;
};

} // namespace launcher
