#pragma once

#include "Intent.hpp"
#include "LauncherState.hpp"

#include "services/IConfigService.hpp"
#include "services/IDownloadService.hpp"
#include "services/IEngineOptionsService.hpp"
#include "services/IGameFilesService.hpp"
#include "services/IPathProvider.hpp"

#include <deque>
#include <functional>
#include <mutex>
#include <vector>
#include <thread>

namespace launcher {

/// «Редьюсер» из схемы MVI: владеет LauncherState и превращает Intents
/// в изменения состояния и побочные эффекты через переданные сервисы.
///
/// Правила работы с потоками:
/// - Dispatch() можно звать из любого потока (он только кладёт в очередь);
/// - Poll() вызывается раз в кадр на главном потоке;
/// - State() читается только на главном потоке.
class Store {
public:
	Store(IConfigService &configService,
	      IGameFilesService &filesService,
	      IDownloadService &downloadService,
	      IPathProvider &pathProvider,
	      IEngineOptionsService &engineOptionsService,
	      std::vector<int> displayHeights = {});

	/// Читает настройки и делает первичный поиск файлов.
	void Init();

	/// Колбэк «в очередь пришёл интент из фонового потока». Главный цикл
	/// в свёрнутом состоянии спит в SDL_WaitEvent и сам о новых интентах
	/// (прогресс загрузок из потоков zoe) не узнает — Application вешает
	/// сюда пинок в очередь событий SDL. Ставится один раз, до Init().
	void SetWakeCallback(std::function<void()> callback);

	/// Ставит интент в очередь. Потокобезопасно; применяется при
	/// следующем Poll() на главном потоке.
	void Dispatch(Intent intent);

	/// Применяет все накопленные интенты. Вызывается раз в кадр перед отрисовкой.
	void Poll();

	[[nodiscard]] const LauncherState &State() const { return m_state; }

private:
	void Reduce(const Intent &intent);

	// По одному обработчику на тип интента; специализации определены
	// в Store.cpp для каждого варианта из Intent.
	template <typename T>
	void ReduceIntent(const T &intent);

	void RescanAndDerive();
	void DeriveGameCards();
	void BeginDownload(KnownFile file);

	IConfigService &m_configService;
	IGameFilesService &m_filesService;
	IDownloadService &m_downloadService;
	IPathProvider &m_pathProvider;
	IEngineOptionsService &m_engineOptionsService;

	/// Ландшафтные высоты дисплейных режимов (собирает Application через
	/// SDL) — исходные данные зеркала игрового списка разрешений.
	std::vector<int> m_displayHeights;

	LauncherConfig m_config;
	LauncherState m_state;

	/// Поток, создавший Store (по договорённости — главный). Dispatch с
	/// любого другого потока дёргает wake-колбэк.
	std::thread::id m_mainThreadId;
	std::function<void()> m_wakeCallback;

	std::deque<Intent> m_queue;
	std::mutex m_queueMutex;
};

} // namespace launcher
