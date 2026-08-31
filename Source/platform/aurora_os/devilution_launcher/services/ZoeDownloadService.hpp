#pragma once

#include "IDownloadService.hpp"

#include <zoe/zoe.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>

namespace launcher {

/// Реализация IDownloadService поверх загрузчика zoe.
///
/// - Одна загрузка за раз; повторный Start() при активной игнорируется.
/// - Прогресс присылается не чаще ~4 раз в секунду; скорость берётся
///   из колбэка мгновенной скорости zoe и идёт вместе с прогрессом.
/// - Cancel() прерывает загрузку; слушатель получает onFinished(false,
///   "cancelled"), недокачанный файл удаляется.
class ZoeDownloadService final : public IDownloadService {
public:
	ZoeDownloadService();
	~ZoeDownloadService() override;

	void Start(const std::string &url, const std::filesystem::path &destination, Listener listener) override;
	void Cancel() override;
	[[nodiscard]] bool IsActive() override;

private:
/// Охраняет m_active, m_listener и состояние троттлинга (колбэки zoe
/// приходят из фоновых потоков).
	std::mutex m_mutex;
	bool m_active = false;
	Listener m_listener;
	std::filesystem::path m_destination;
	std::chrono::steady_clock::time_point m_lastProgressEmit {};
	int64_t m_lastSpeedBytesPerSec = 0;

	std::unique_ptr<zoe::Zoe> m_zoe;
	std::shared_future<zoe::ZoeResult> m_future;
};

} // namespace launcher
