#include "ZoeDownloadService.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <system_error>
#include <utility>

namespace launcher {

namespace {

constexpr auto kProgressInterval = std::chrono::milliseconds(250);
constexpr std::string_view kCancelledError = "cancelled";

/// zoe::GlobalInit/GlobalUnInit are process-wide; refcount so multiple
/// instances (and tests) stay balanced.
int &GlobalInitRefCount()
{
	static int count = 0;
	return count;
}

void AcquireZoeGlobal()
{
	if (GlobalInitRefCount()++ == 0) {
		zoe::Zoe::GlobalInit();
	}
}

void ReleaseZoeGlobal()
{
	if (--GlobalInitRefCount() == 0) {
		zoe::Zoe::GlobalUnInit();
	}
}

} // namespace

ZoeDownloadService::ZoeDownloadService()
{
	AcquireZoeGlobal();

	m_zoe = std::make_unique<zoe::Zoe>();
	m_zoe->setThreadNum(1);
	m_zoe->setUncompletedSliceSavePolicy(zoe::UncompletedSliceSavePolicy::AlwaysDiscard);
}

ZoeDownloadService::~ZoeDownloadService()
{
	if (m_zoe) {
		m_zoe->stop();
	}
	if (m_future.valid()) {
		m_future.wait();
	}
	ReleaseZoeGlobal();
}

void ZoeDownloadService::start(const std::string &url, const std::filesystem::path &destination, Listener listener)
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_active) {
			spdlog::warn("ZoeDownloadService::start ignored, a download is already active");
			return;
		}
		m_active = true;
		m_listener = std::move(listener);
		m_destination = destination;
		m_lastProgressEmit = {};
		m_lastSpeedBytesPerSec = 0;
	}

	std::error_code ec;
	std::filesystem::create_directories(destination.parent_path(), ec);

	m_future = m_zoe->start(
	    url,
	    destination.string(),
	    [this](zoe::ZoeResult result) {
		    IDownloadService::Listener listener;
		    bool cancelled = false;
		    std::filesystem::path destination;
		    {
			    std::lock_guard<std::mutex> lock(m_mutex);
			    listener = m_listener;
			    destination = m_destination;
			    m_active = false;
			    cancelled = (result == zoe::ZoeResult::CANCELED);
		    }

		    if (result == zoe::ZoeResult::SUCCESSED) {
			    if (listener.onFinished) {
				    listener.onFinished(true, {});
			    }
			    return;
		    }

		    const std::string error = cancelled
		        ? std::string(kCancelledError)
		        : std::string("Ошибка загрузки: ") + zoe::Zoe::GetResultString(result);
		    if (!cancelled) {
			    std::error_code removeEc;
			    std::filesystem::remove(destination, removeEc);
		    }
		    if (listener.onFinished) {
			    listener.onFinished(false, error);
		    }
	    },
	    [this](int64_t total, int64_t downloaded) {
		    std::lock_guard<std::mutex> lock(m_mutex);
		    if (!m_active || !m_listener.onProgress) {
			    return;
		    }
		    const auto now = std::chrono::steady_clock::now();
		    if (m_lastProgressEmit.time_since_epoch().count() != 0 && now - m_lastProgressEmit < kProgressInterval) {
			    return;
		    }
		    m_lastProgressEmit = now;
		    m_listener.onProgress(total, downloaded, m_lastSpeedBytesPerSec);
	    },
	    [this](int64_t bytesPerSec) {
		    std::lock_guard<std::mutex> lock(m_mutex);
		    m_lastSpeedBytesPerSec = bytesPerSec;
	    });
}

void ZoeDownloadService::cancel()
{
	m_zoe->stop();
}

bool ZoeDownloadService::isActive()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_active;
}

} // namespace launcher
