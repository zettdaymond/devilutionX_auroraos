#pragma once

#include "Intent.hpp"
#include "LauncherState.hpp"

#include "services/IConfigService.hpp"
#include "services/IDownloadService.hpp"
#include "services/IGameFilesService.hpp"
#include "services/IPathProvider.hpp"

#include <deque>
#include <mutex>

namespace launcher {

/// The MVI "reducer": owns LauncherState and translates Intents into
/// state changes plus side effects through the injected services.
///
/// Threading contract:
/// - Dispatch() may be called from any thread (it only enqueues);
/// - Poll() must be called once per frame on the main thread;
/// - State() must only be read from the main thread.
class Store {
public:
	Store(IConfigService &configService,
	      IGameFilesService &filesService,
	      IDownloadService &downloadService,
	      IPathProvider &pathProvider);

	/// Load the config and perform the initial file scan.
	void Init();

	/// Enqueue an intent. Thread-safe; the intent is applied on the
	/// next Poll() on the main thread.
	void Dispatch(Intent intent);

	/// Apply all queued intents. Call once per frame before rendering.
	void Poll();

	[[nodiscard]] const LauncherState &State() const { return m_state; }

private:
	void Reduce(const Intent &intent);

	// One handler per intent type; specialized in Store.cpp for every
	// intent listed in the Intent variant.
	template <typename T>
	void ReduceIntent(const T &intent);

	void RescanAndDerive();
	void DeriveGameCards();
	void BeginDownload(KnownFile file);

	IConfigService &m_configService;
	IGameFilesService &m_filesService;
	IDownloadService &m_downloadService;
	IPathProvider &m_pathProvider;

	LauncherConfig m_config;
	LauncherState m_state;

	std::deque<Intent> m_queue;
	std::mutex m_queueMutex;
};

} // namespace launcher
