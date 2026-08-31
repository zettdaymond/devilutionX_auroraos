#pragma once

#include "services/IDownloadService.hpp"

#include "MockWorld.hpp"

#include <atomic>
#include <thread>

namespace launcher {

/// Как ведёт себя имитируемая загрузка.
enum class MockDownloadBehavior {
	InstantSuccess, ///< finishes in ~0.3 s (UI smoke tests)
	SlowSuccess,    ///< ~12 s of visible progress, cancelable
	FailAtHalf,     ///< fails at 50 % with an error message
};

/// Имитация загрузчика. Фоновый поток присылает события прогресса,
/// при успехе обновляет общий MockWorld и поддерживает отмену —
/// повторяет правила работы с потоками настоящей ZoeDownloadService.
class MockDownloadService final : public IDownloadService {
public:
/// Держит долю владения «миром», чтобы набор сервисов оставался жив
/// независимо от времени жизни сценария-имитации.
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
