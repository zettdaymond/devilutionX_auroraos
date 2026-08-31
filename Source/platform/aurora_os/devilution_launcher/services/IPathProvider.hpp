#pragma once

#include "IConfigService.hpp"

#include <filesystem>
#include <vector>

namespace launcher {

/// Пути, зависящие от платформы. Это единственный шов между
/// переносимым кодом лаунчера и платформой-хостом
/// (Aurora OS с Qt против десктопа на SDL).
class IPathProvider {
public:
	virtual ~IPathProvider() = default;

	/// Папка служебных файлов лаунчера (launcher.conf и прочие).
	[[nodiscard]] virtual std::filesystem::path ConfigDir() = 0;

	/// Папка для скачиваемого (spawn.mpq, ru.mpq), доступная для записи.
	/// Она же — путь поиска MPQ для самой игры.
	[[nodiscard]] virtual std::filesystem::path DownloadsDir() = 0;

	/// Папки, в которых ищутся файлы игры, по убыванию приоритета:
	/// сначала выбранная пользователем, затем пути платформы.
	[[nodiscard]] virtual std::vector<std::filesystem::path> CandidateDataDirs(const LauncherConfig &config) = 0;
};

} // namespace launcher
