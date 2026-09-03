#include "DBus.hpp"

#include <dbus/dbus.h>

#include <SDL2/SDL.h>

#include <mutex>
#include <string>

namespace devilution {

namespace {

/// Приватное соединение системной шины: таймер DisplayBlankerController
/// стреляет из потока SDL, а Init/Shutdown зовёт основной поток.
std::mutex g_connectionMutex;
DBusConnection *g_systemConnection = nullptr;

} // namespace

void DBus::Init()
{
    std::lock_guard lock(g_connectionMutex);
    if (g_systemConnection != nullptr) {
        return;
    }
    dbus_threads_init_default();

    DBusError error;
    dbus_error_init(&error);
    g_systemConnection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
    if (g_systemConnection == nullptr || dbus_error_is_set(&error)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "DBus: system bus unavailable (%s)",
            error.message != nullptr ? error.message : "?");
        dbus_error_free(&error);
        g_systemConnection = nullptr;
        return;
    }
    dbus_connection_set_exit_on_disconnect(g_systemConnection, false);
}

void DBus::Shutdown()
{
    std::lock_guard lock(g_connectionMutex);
    if (g_systemConnection != nullptr) {
        dbus_connection_close(g_systemConnection);
        dbus_connection_unref(g_systemConnection);
        g_systemConnection = nullptr;
    }
}

bool DBus::CallVoidMethod(std::string_view node, std::string_view path,
    std::string_view interface, std::string_view method)
{
    std::lock_guard lock(g_connectionMutex);
    if (g_systemConnection == nullptr) {
        return false;
    }

    // string_view не обязан быть NUL-терминирован, а D-Bus принимает C-строки.
    const std::string nodeStr { node };
    const std::string pathStr { path };
    const std::string interfaceStr { interface };
    const std::string methodStr { method };

    DBusMessage *message = dbus_message_new_method_call(
        nodeStr.c_str(), pathStr.c_str(), interfaceStr.c_str(), methodStr.c_str());
    if (message == nullptr) {
        return false;
    }
    const dbus_bool_t sent = dbus_connection_send(g_systemConnection, message, nullptr);
    dbus_message_unref(message);
    if (sent != 0) {
        dbus_connection_flush(g_systemConnection);
    }
    return sent != 0;
}

}
