#pragma once

#include <filesystem>
#include <optional>

namespace launcher {

/// Настройки лаунчера, живущие между запусками.
struct LauncherConfig {
/// Папка с файлами пользователя (DIABDAT.MPQ, hellfire*.mpq).
/// Пуста, пока пользователь не выбрал папку.
	std::optional<std::filesystem::path> dataFolder;
};

/// Хранилище LauncherConfig. Создание должно быть дешёвым;
/// Save() пишет файл целиком атомарно (временный файл + переименование).
class IConfigService {
public:
	virtual ~IConfigService() = default;

	[[nodiscard]] virtual LauncherConfig Load() = 0;
	virtual void Save(const LauncherConfig &config) = 0;
};

} // namespace launcher
