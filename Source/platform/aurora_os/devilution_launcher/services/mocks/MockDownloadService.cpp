#include "MockDownloadService.hpp"

#include "core/GameFiles.hpp"

#include <chrono>
#include <utility>

namespace launcher {

namespace {

using namespace std::chrono_literals;

/// Figure out which known file a download destination refers to,
/// so the mock can update the right MockWorld entry.
KnownFile KnownFileFromDestination(const std::filesystem::path &destination)
{
	const std::string name = destination.filename().string();
	for (size_t i = 0; i < kKnownFileCount; ++i) {
		const KnownFile id = static_cast<KnownFile>(i);
		if (IsKnownFileName(name, id)) {
			return id;
		}
	}
	return KnownFile::Spawn;
}

/// Sleep in small slices, waking up early on cancel.
bool SleepCancelAware(std::atomic<bool> &cancelRequested, std::chrono::milliseconds total)
{
	const auto slice = 25ms;
	for (auto remaining = total; remaining.count() > 0; remaining -= slice) {
		if (cancelRequested.load()) {
			return true;
		}
		std::this_thread::sleep_for(std::min(slice, remaining));
	}
	return cancelRequested.load();
}

} // namespace

MockDownloadService::MockDownloadService(std::shared_ptr<MockWorld> world, MockDownloadBehavior behavior)
    : m_world(std::move(world))
    , m_behavior(behavior)
{
}

MockDownloadService::~MockDownloadService()
{
	m_cancelRequested.store(true);
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void MockDownloadService::Start(const std::string &, const std::filesystem::path &destination, Listener listener)
{
	if (m_active.load()) {
		return;
	}

	const KnownFile file = KnownFileFromDestination(destination);
	const int64_t totalBytes = std::max<int64_t>(FileSpecOf(file).expectedSizeBytes, 1024);

	m_cancelRequested.store(false);
	m_active.store(true);
	if (m_thread.joinable()) {
		m_thread.join();
	}
	m_thread = std::thread([this, listener = std::move(listener), totalBytes, file]() mutable {
		Run(std::move(listener), totalBytes, file);
	});
}

void MockDownloadService::Cancel()
{
	m_cancelRequested.store(true);
}

bool MockDownloadService::IsActive()
{
	return m_active.load();
}

void MockDownloadService::Run(Listener listener, int64_t totalBytes, KnownFile file)
{
	const int stepCount = m_behavior == MockDownloadBehavior::InstantSuccess ? 10 : 60;
	const auto stepDelay = m_behavior == MockDownloadBehavior::InstantSuccess ? 30ms : 200ms;
	const double simulatedSpeed = static_cast<double>(totalBytes) / (stepCount * 0.2); // bytes/sec at 200 ms steps

	for (int step = 1; step <= stepCount; ++step) {
		if (SleepCancelAware(m_cancelRequested, stepDelay)) {
			break;
		}

		const int64_t downloaded = totalBytes * step / stepCount;
		if (listener.onProgress) {
			listener.onProgress(totalBytes, downloaded, static_cast<int64_t>(simulatedSpeed));
		}

		if (m_behavior == MockDownloadBehavior::FailAtHalf && step == stepCount / 2) {
			m_active.store(false);
			m_world->ApplyDownloadFinished(file, false);
			if (listener.onFinished) {
				listener.onFinished(false, "Ошибка сети: соединение прервано");
			}
			return;
		}
	}

	m_active.store(false);
	if (m_cancelRequested.load()) {
		m_world->ApplyDownloadFinished(file, false);
		if (listener.onFinished) {
			listener.onFinished(false, "cancelled");
		}
		return;
	}

	m_world->ApplyDownloadFinished(file, true);
	if (listener.onFinished) {
		listener.onFinished(true, {});
	}
}

} // namespace launcher
