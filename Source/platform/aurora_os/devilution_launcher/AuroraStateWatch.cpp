// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "AuroraStateWatch.hpp"

#include <dbus/dbus.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <string>

namespace launcher::aurora {

namespace {

/// Имя для диагностического лога.
const char *NameOf(StateEvent event)
{
	switch (event) {
	case StateEvent::DisplayOn: return "display";
	case StateEvent::TkLocked: return "tklock";
	case StateEvent::TopmostOurs: return "topmost";
	}
	return "?";
}

} // namespace

StateWatch::StateWatch()
    : m_eventType(SDL_RegisterEvents(1))
    , m_thread([this]() { Run(); })
{
}

StateWatch::~StateWatch()
{
	m_stop.store(true);
	if (m_thread.joinable()) {
		m_thread.join();
	}
}

void StateWatch::Push(StateEvent what, bool value)
{
	switch (what) {
	case StateEvent::DisplayOn:
		m_displayOn.store(value, std::memory_order_relaxed);
		break;
	case StateEvent::TkLocked:
		m_tkLocked.store(value, std::memory_order_relaxed);
		break;
	case StateEvent::TopmostOurs:
		m_topmostOurs.store(value, std::memory_order_relaxed);
		break;
	}
	spdlog::info("aurora: {} = {}", NameOf(what), value ? 1 : 0);
	SDL_Event event {};
	event.type = m_eventType;
	event.user.code = static_cast<int>(what);
	event.user.data1 = value ? reinterpret_cast<void *>(1) : nullptr;
	SDL_PushEvent(&event);
}

void StateWatch::Run()
{
	dbus_threads_init_default();

	DBusError error;
	dbus_error_init(&error);
	// Приватная (не разделяемая) коннекция: цикл ниже выгребает ВСЕ
	// сообщения шины, и на общей коннекции он съедал бы чужие ответы
	// (Hello/методы других клиентов процесса — так зависал запрос
	// аудиоресурса в фазе движка).
	DBusConnection *system = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
	if (system == nullptr) {
		spdlog::warn("aurora: системная шина недоступна ({})",
		    error.message != nullptr ? error.message : "?");
		dbus_error_free(&error);
		return;
	}

	// Системная шина: mce (дисплей, блокировка) и верхнее окно композитора.
	dbus_bus_add_match(system, "type='signal',sender='com.nokia.mce',interface='com.nokia.mce.signal'", &error);
	const bool mceOk = !dbus_error_is_set(&error);
	if (!mceOk) {
		spdlog::warn("aurora: подписка на mce не удалась ({})", error.message);
		dbus_error_free(&error);
	}
	dbus_bus_add_match(system,
	    "type='signal',interface='org.nemomobile.compositor',member='privateTopmostWindowProcessIdChanged'",
	    &error);
	const bool compositorOk = !dbus_error_is_set(&error);
	if (!compositorOk) {
		spdlog::warn("aurora: подписка на композитор не удалась ({})", error.message);
		dbus_error_free(&error);
	}

	if (!mceOk && !compositorOk) {
		dbus_connection_unref(system);
		return;
	}

	// Начальные состояния — синхронными запросами, чтобы не ждать первых
	// переключений (лаунчер могут запустить уже заблокированным).
	const auto queryMceString = [&system](const char *method) -> std::string {
		DBusMessage *call = dbus_message_new_method_call(
		    "com.nokia.mce", "/com/nokia/mce/request", "com.nokia.mce.request", method);
		if (call == nullptr) {
			return {};
		}
		DBusError queryError;
		dbus_error_init(&queryError);
		DBusMessage *reply = dbus_connection_send_with_reply_and_block(system, call, 1000, &queryError);
		dbus_message_unref(call);
		std::string status;
		if (reply != nullptr) {
			const char *value = nullptr;
			if (dbus_message_get_args(reply, &queryError, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID)
			    && value != nullptr) {
				status = value;
			}
			dbus_message_unref(reply);
		}
		dbus_error_free(&queryError);
		return status;
	};
	if (mceOk) {
		const std::string display = queryMceString("get_display_status");
		if (display.empty()) {
			// Пустой ответ = таймаут/отказ: под песочницей Авроры (иконочный
			// запуск) dbus-прокси молча режет часть методов mce.
			spdlog::warn("aurora: get_display_status без ответа (песочница?)");
		} else {
			Push(StateEvent::DisplayOn, display != "off");
		}
		const std::string lock = queryMceString("get_tklock_mode");
		if (lock.empty()) {
			spdlog::warn("aurora: get_tklock_mode без ответа (песочница?)");
		} else {
			Push(StateEvent::TkLocked, lock == "locked");
		}
	}
	const int32_t ourPid = static_cast<int32_t>(::getpid());

	// Диспетчеризация одной шины: интерфейс определяет источник.
	const auto drain = [this, ourPid](DBusConnection *connection) {
		while (DBusMessage *message = dbus_connection_pop_message(connection)) {
			DBusError parse;
			dbus_error_init(&parse);
			const char *status = nullptr;
			if (dbus_message_is_signal(message, "com.nokia.mce.signal", "display_status_ind")
			    && dbus_message_get_args(message, &parse, DBUS_TYPE_STRING, &status, DBUS_TYPE_INVALID)
			    && status != nullptr) {
				// «dimmed» и прочие промежуточные состояния считаем
				// включённым экраном: рисовать ещё есть для кого.
				Push(StateEvent::DisplayOn, std::strcmp(status, "off") != 0);
			} else if (dbus_message_is_signal(message, "com.nokia.mce.signal", "tklock_mode_ind")
			    && dbus_message_get_args(message, &parse, DBUS_TYPE_STRING, &status, DBUS_TYPE_INVALID)
			    && status != nullptr) {
				Push(StateEvent::TkLocked, std::strcmp(status, "locked") == 0);
			} else if (dbus_message_is_signal(message, "org.nemomobile.compositor",
			               "privateTopmostWindowProcessIdChanged")) {
				int32_t pid = 0;
				if (dbus_message_get_args(message, &parse, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID)) {
					Push(StateEvent::TopmostOurs, pid == ourPid);
				}
			}
			dbus_error_free(&parse);
			dbus_message_unref(message);
		}
	};

	// Блокирующее чтение с таймаутом: просыпаемся четыре раза в секунду
	// только чтобы проверить флаг завершения — дешевле интеграции шины
	// в цикл событий.
	//
	// Песочница Авроры не форвардит ИНДИКАТОРЫ mce (display_status_ind и
	// прочие сигналы), но разовые ЗАПРОСЫ пропускает (частично) — поэтому
	// состояние дисплея дополнительно опрашиваем сами, редко и с
	// дедупликацией: иначе под иконкой «экран погашен» не узнать, и
	// движок рисовал бы обложку в тёмную матрицу.
	auto lastDisplayPoll = std::chrono::steady_clock::now();
	constexpr auto kDisplayPollInterval = std::chrono::seconds(2);
	while (!m_stop.load()) {
		if (!dbus_connection_read_write(system, 250)) {
			spdlog::warn("aurora: системная шина потеряна");
			break;
		}
		drain(system);
		if (mceOk && std::chrono::steady_clock::now() - lastDisplayPoll >= kDisplayPollInterval) {
			lastDisplayPoll = std::chrono::steady_clock::now();
			const std::string display = queryMceString("get_display_status");
			if (!display.empty()) {
				const bool on = display != "off";
				if (on != m_displayOn.load(std::memory_order_relaxed)) {
					Push(StateEvent::DisplayOn, on);
				}
			}
		}
	}
	// Приватную коннекцию нужно явно закрыть (разделяемая закрывается
	// сама при обнулении ссылок).
	dbus_connection_close(system);
	dbus_connection_unref(system);
}

} // namespace launcher::aurora

#endif
