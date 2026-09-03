#pragma once

#ifdef AURORA_OS

#include <SDL2/SDL.h>

#include <atomic>
#include <thread>

namespace launcher::aurora {

/// Какое состояние прибывал наблюдатель. Код кладётся в
/// SDL_Event::user::code — значения фиксированы, порядок не менять.
enum class StateEvent : int {
	DisplayOn = 0,   ///< Дисплей включён (mce display_status_ind).
	TkLocked = 1,    ///< Экран заблокирован (mce tklock_mode_ind).
	TopmostOurs = 2, ///< Наш процесс — верхнее окно (композитор Lipstick).
};

/// Наблюдатель состояния Авроры: поток слушает D-Bus и переправляет
/// изменения в очередь SDL пользовательским событием.
///
/// Источники (снимались на устройстве, см. память проекта): системная
/// шина — демон mce (дисплей и блокировка экрана) и композитор
/// Lipstick (верхнее окно по pid; сигнал широковещательный).
///
/// Событий SDL, различающих блокировку и сворачивание, не существует:
/// в обоих случаях приходит только FOCUS_LOST — поэтому вся логика
/// состояния живёт здесь, а Application лишь применяет события.
/// Широковещательный coverstatus с сессионной шины НЕ используется:
/// сигнал не адресован окну и корректно атрибутировать чужие жесты
/// нельзя — вход в плитку делает topmost, выход тоже.
class StateWatch final {
public:
	StateWatch();
	~StateWatch();

	StateWatch(const StateWatch &) = delete;
	StateWatch &operator=(const StateWatch &) = delete;

	/// Тип пользовательского SDL-события, по которому приходят состояния.
	[[nodiscard]] Uint32 EventType() const { return m_eventType; }

	/// Текущие состояния без ожидания событий. Нужны фазе движка:
	/// его цикл не слушает наши события (выбрасывает их как чужие),
	/// а опрашивает перед каждым Present'ом. До первых сигналов
	/// считаем себя передним планом на включённом экране.
	[[nodiscard]] bool TopmostOurs() const { return m_topmostOurs.load(std::memory_order_relaxed); }
	[[nodiscard]] bool DisplayOn() const { return m_displayOn.load(std::memory_order_relaxed); }
	[[nodiscard]] bool TkLocked() const { return m_tkLocked.load(std::memory_order_relaxed); }

private:
	/// Тело потока: подключение к шинам и диспетчеризация до остановки.
	void Run();

	/// Отправляет состояние в очередь SDL (потокобезопасно).
	void Push(StateEvent what, bool value);

	const Uint32 m_eventType;
	std::thread m_thread;
	std::atomic<bool> m_stop { false };

	std::atomic<bool> m_displayOn { true };
	std::atomic<bool> m_tkLocked { false };
	std::atomic<bool> m_topmostOurs { true };
};

} // namespace launcher::aurora

#endif
