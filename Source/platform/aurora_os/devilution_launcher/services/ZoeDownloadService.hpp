#pragma once

#include "IDownloadService.hpp"

#include <zoe/zoe.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>

namespace launcher {

/// IDownloadService implementation backed by the zoe downloader.
///
/// - One download at a time; a second Start() while active is ignored.
/// - Progress reports are throttled to ~4 per second; speed is sampled
///   from zoe's realtime-speed callback and sent along with progress.
/// - Cancel() aborts; the listener then receives onFinished(false,
///   "cancelled") and the partial file is removed.
class ZoeDownloadService final : public IDownloadService {
public:
	ZoeDownloadService();
	~ZoeDownloadService() override;

	void Start(const std::string &url, const std::filesystem::path &destination, Listener listener) override;
	void Cancel() override;
	[[nodiscard]] bool IsActive() override;

private:
	/// Guards m_active, m_listener and throttle state (zoe callbacks
	/// arrive on background threads).
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
