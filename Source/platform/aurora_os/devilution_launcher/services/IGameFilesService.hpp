#pragma once

#include "core/GameFiles.hpp"

#include <filesystem>
#include <vector>

namespace launcher {

/// Результат поиска известных файлов игры в одной или нескольких папках:
/// sizes[i] — размер файла в байтах или -1, если файл не найден.
struct FileScanResult {
	std::array<int64_t, kKnownFileCount> sizes {};
	std::array<std::filesystem::path, kKnownFileCount> folders {}; // folder the file was found in
};

/// Доступ к файловой системе для данных игры. Все методы синхронные,
/// их можно звать на главном потоке (папки маленькие).
class IGameFilesService {
public:
	virtual ~IGameFilesService() = default;

	/// Сканирует папки по порядку; побеждает первое найденное место.
	/// Имена файлов сравниваются без учёта регистра.
	[[nodiscard]] virtual FileScanResult Scan(const std::vector<std::filesystem::path> &folders) = 0;

	/// Свободное место на разделе, где лежит путь, в байтах.
	[[nodiscard]] virtual int64_t FreeSpace(const std::filesystem::path &dir) = 0;

	/// Удаляет известный файл из папки. true, если файл существовал
/// и файл удалён; false, если файла не было.
	virtual bool RemoveFile(const std::filesystem::path &dir, KnownFile file) = 0;
};

} // namespace launcher
