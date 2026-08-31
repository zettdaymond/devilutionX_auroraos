#pragma once

#include "core/GameFiles.hpp"

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace launcher {

/// Общая поддельная среда для сервисов-имитаций: какие файлы «существуют»,
/// их размеры и поддельное свободное место. MockGameFilesService
/// читает её; MockDownloadService обновляет при успехе и отмене.
/// Потокобезопасна: имитация загрузки работает в отдельном потоке.
class MockWorld {
public:
	MockWorld();

	void SetPresent(KnownFile file, int64_t sizeBytes);
	void SetAbsent(KnownFile file);
	[[nodiscard]] bool IsPresent(KnownFile file) const;

	void SetFreeSpace(int64_t bytes);

	struct Snapshot {
		std::array<int64_t, kKnownFileCount> files {};
		int64_t freeSpaceBytes = 0;
	};

	/// Непротиворечивая копия всего состояния «мира».
	[[nodiscard]] Snapshot Take() const;

	void ApplyDownloadFinished(KnownFile file, bool success);

	/// Применяет именованный набор ("empty", "diablo-found", "hellfire-partial",
	/// "full"). Для неизвестного имени возвращает false.
	bool ApplyPreset(const std::string &name);

	/// Все допустимые имена наборов — для вывода в --help.
	static const std::vector<std::string> &PresetNames();

private:
	mutable std::mutex m_mutex;
	std::array<int64_t, kKnownFileCount> m_files {};
	int64_t m_freeSpaceBytes = 0;
};

} // namespace launcher
