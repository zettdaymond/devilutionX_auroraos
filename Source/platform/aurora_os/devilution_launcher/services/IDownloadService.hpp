#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace launcher {

/// Качает по одному файлу за раз. Обратные вызовы могут приходить
/// из фонового потока — реализация гарантирует их вызов,
/// но получатель обязан сам переносить данные в свой поток
/// (Store делает это через Dispatch()).
class IDownloadService {
public:
	struct Listener {
/// Отчёт о прогрессе; bytesPerSec — сглаженная мгновенная скорость.
		std::function<void(int64_t totalBytes, int64_t downloadedBytes, int64_t bytesPerSec)> onProgress;
/// Завершающее событие; вызывается ровно один раз на каждый Start().
/// При отмене или ошибке success = false, а error объясняет причину.
		std::function<void(bool success, std::string error)> onFinished;
	};

	virtual ~IDownloadService() = default;

	/// Начинает загрузку `url` в файл `destination`.
	/// Параллельный старт второй загрузки — ошибка программирования;
	/// реализации записывают это в лог и игнорируют вызов.
	virtual void Start(const std::string &url, const std::filesystem::path &destination, Listener listener) = 0;

	/// Прерывает текущую загрузку: вызывает onFinished(false, "cancelled"),
	/// и удаляет недокачанный файл.
	virtual void Cancel() = 0;

	[[nodiscard]] virtual bool IsActive() = 0;
};

} // namespace launcher
