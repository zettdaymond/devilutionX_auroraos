#pragma once

#include "services/IDownloadService.hpp"

#include "MockWorld.hpp"

#include <atomic>
#include <thread>

namespace launcher {

/// How a mock download behaves.
enum class MockDownloadBehavior {
	InstantSuccess, ///< finishes in ~0.3 s (UI smoke tests)
	SlowSuccess,    ///< ~12 s of visible progress, cancelable
	FailAtHalf,     ///< fails at 50 % with an error message
};

/// Simulated downloader. Runs a background thread that emits progress
/// events, updates the shared MockWorld on success and supports
/// cancellation — mirrors the threading contract of ZoeDownloadService.
class MockDownloadService final : public IDownloadService {
public:
	/// Owns a share of the world so the service bundle stays valid
	/// regardless of the MockScenario's lifetime.
	MockDownloadService(std::shared_ptr<MockWorld> world, MockDownloadBehavior behavior);
	~MockDownloadService() override;

	void Start(const std::string &url, const std::filesystem::path &destination, Listener listener) override;
	void Cancel() override;
	[[nodiscard]] bool IsActive() override;

private:
	void Run(Listener listener, int64_t totalBytes, KnownFile file);

	std::shared_ptr<MockWorld> m_world;
	MockDownloadBehavior m_behavior;

	std::atomic<bool> m_active { false };
	std::atomic<bool> m_cancelRequested { false };
	std::thread m_thread;
};

} // namespace launcher
