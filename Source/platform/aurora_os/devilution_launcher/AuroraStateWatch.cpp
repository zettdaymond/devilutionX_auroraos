// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "AuroraStateWatch.hpp"

#include <dbus/dbus.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

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
	case StateEvent::CoverActive: return "cover";
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
	DBusConnection *system = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
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

	// Сессионная шина: coverstatus жеста сворачивания. Значения: 1 и 2
	// приходят парой в начале жеста (прогресса драга композитор не
	// вещает), 3 и 0 — парой при возврате из плитки.
	DBusConnection *session = nullptr;
	if (mceOk || compositorOk) {
		session = dbus_bus_get(DBUS_BUS_SESSION, &error);
		if (session == nullptr) {
			spdlog::warn("aurora: сессионная шина недоступна ({}), жести не увидим",
			    error.message != nullptr ? error.message : "?");
			dbus_error_free(&error);
		} else {
			dbus_bus_add_match(session, "type='signal',interface='com.jolla.lipstick',member='coverstatus'", &error);
			if (dbus_error_is_set(&error)) {
				spdlog::warn("aurora: подписка на coverstatus не удалась ({})", error.message);
				dbus_error_free(&error);
				dbus_connection_unref(session);
				session = nullptr;
			}
		}
	}
	if (!mceOk && !compositorOk) {
		dbus_connection_unref(system);
		if (session != nullptr) {
			dbus_connection_unref(session);
		}
		return;
	}

	// Начальные состояния — синхронными запросами, чтобы не ждать первых
	// переключений (лаунчер могут запустить уже заблокированным).
	if (mceOk) {
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
		const std::string display = queryMceString("get_display_status");
		if (!display.empty()) {
			Push(StateEvent::DisplayOn, display != "off");
		}
		const std::string lock = queryMceString("get_tklock_mode");
		if (!lock.empty()) {
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
			} else if (dbus_message_is_signal(message, "com.jolla.lipstick", "coverstatus")) {
				int32_t cover = 0;
				if (dbus_message_get_args(message, &parse, DBUS_TYPE_INT32, &cover, DBUS_TYPE_INVALID)) {
					Push(StateEvent::CoverActive, cover == 2);
				}
			}
			dbus_error_free(&parse);
			dbus_message_unref(message);
		}
	};

	// Блокирующее чтение с таймаутом по каждой из шин по очереди:
	// просыпаемся ~7 раз в секунду только чтобы проверить флаг
	// завершения — дешевле интеграции шин в цикл событий.
	while (!m_stop.load()) {
		if (!dbus_connection_read_write(system, 70)) {
			spdlog::warn("aurora: системная шина потеряна");
			break;
		}
		drain(system);
		if (session != nullptr) {
			if (!dbus_connection_read_write(session, 70)) {
				spdlog::warn("aurora: сессионная шина потеряна");
				dbus_connection_unref(session);
				session = nullptr;
				continue;
			}
			drain(session);
		}
	}
	dbus_connection_unref(system);
	if (session != nullptr) {
		dbus_connection_unref(session);
	}
}

} // namespace launcher::aurora

#endif
