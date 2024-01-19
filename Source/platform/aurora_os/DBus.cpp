#include "DBus.hpp"


#include <dbus/dbus.h>
#include "SDL_loadso.h"

namespace devilution {

namespace impl {

typedef struct DBusContext {
    DBusConnection *session_conn;
    DBusConnection *system_conn;

    DBusConnection *(*bus_get_private)(DBusBusType, DBusError *);
    dbus_bool_t (*bus_register)(DBusConnection *, DBusError *);
    void (*bus_add_match)(DBusConnection *, const char *, DBusError *);
    DBusConnection * (*connection_open_private)(const char *, DBusError *);
    void (*connection_set_exit_on_disconnect)(DBusConnection *, dbus_bool_t);
    dbus_bool_t (*connection_get_is_connected)(DBusConnection *);
    dbus_bool_t (*connection_add_filter)(DBusConnection *, DBusHandleMessageFunction, void *, DBusFreeFunction);
    dbus_bool_t (*connection_try_register_object_path)(DBusConnection *, const char *,
        const DBusObjectPathVTable *, void *, DBusError *);
    dbus_bool_t (*connection_send)(DBusConnection *, DBusMessage *, dbus_uint32_t *);
    DBusMessage *(*connection_send_with_reply_and_block)(DBusConnection *, DBusMessage *, int, DBusError *);
    void (*connection_close)(DBusConnection *);
    void (*connection_unref)(DBusConnection *);
    void (*connection_flush)(DBusConnection *);
    dbus_bool_t (*connection_read_write)(DBusConnection *, int);
    DBusDispatchStatus (*connection_dispatch)(DBusConnection *);
    dbus_bool_t (*message_is_signal)(DBusMessage *, const char *, const char *);
    DBusMessage *(*message_new_method_call)(const char *, const char *, const char *, const char *);
    dbus_bool_t (*message_append_args)(DBusMessage *, int, ...);
    dbus_bool_t (*message_append_args_valist)(DBusMessage *, int, va_list);
    dbus_bool_t (*message_get_args)(DBusMessage *, DBusError *, int, ...);
    dbus_bool_t (*message_get_args_valist)(DBusMessage *, DBusError *, int, va_list);
    dbus_bool_t (*message_iter_init)(DBusMessage *, DBusMessageIter *);
    dbus_bool_t (*message_iter_next)(DBusMessageIter *);
    void (*message_iter_get_basic)(DBusMessageIter *, void *);
    int (*message_iter_get_arg_type)(DBusMessageIter *);
    void (*message_iter_recurse)(DBusMessageIter *, DBusMessageIter *);
    void (*message_unref)(DBusMessage *);
    void (*error_init)(DBusError *);
    dbus_bool_t (*error_is_set)(const DBusError *);
    void (*error_free)(DBusError *);
    char *(*get_local_machine_id)(void);
    void (*free)(void *);
    void (*free_string_array)(char **);
    void (*shutdown)(void);

} DBusContext;

static const char *dbus_library = "libdbus-1.so.3";
static void *dbus_handle = NULL;
static DBusContext dbus;

static int LoadDBUSSyms(void)
{
    #define SDL_DBUS_SYM2(x, y) \
        if (!(dbus.x = (decltype(dbus.x))SDL_LoadFunction(dbus_handle, #y))) return -1

    #define SDL_DBUS_SYM(x) \
        SDL_DBUS_SYM2(x, dbus_##x)

    SDL_DBUS_SYM(bus_get_private);
    SDL_DBUS_SYM(bus_register);
    SDL_DBUS_SYM(bus_add_match);
    SDL_DBUS_SYM(connection_open_private);
    SDL_DBUS_SYM(connection_set_exit_on_disconnect);
    SDL_DBUS_SYM(connection_get_is_connected);
    SDL_DBUS_SYM(connection_add_filter);
    SDL_DBUS_SYM(connection_try_register_object_path);
    SDL_DBUS_SYM(connection_send);
    SDL_DBUS_SYM(connection_send_with_reply_and_block);
    SDL_DBUS_SYM(connection_close);
    SDL_DBUS_SYM(connection_unref);
    SDL_DBUS_SYM(connection_flush);
    SDL_DBUS_SYM(connection_read_write);
    SDL_DBUS_SYM(connection_dispatch);
    SDL_DBUS_SYM(message_is_signal);
    SDL_DBUS_SYM(message_new_method_call);
    SDL_DBUS_SYM(message_append_args);
    SDL_DBUS_SYM(message_append_args_valist);
    SDL_DBUS_SYM(message_get_args);
    SDL_DBUS_SYM(message_get_args_valist);
    SDL_DBUS_SYM(message_iter_init);
    SDL_DBUS_SYM(message_iter_next);
    SDL_DBUS_SYM(message_iter_get_basic);
    SDL_DBUS_SYM(message_iter_get_arg_type);
    SDL_DBUS_SYM(message_iter_recurse);
    SDL_DBUS_SYM(message_unref);
    SDL_DBUS_SYM(error_init);
    SDL_DBUS_SYM(error_is_set);
    SDL_DBUS_SYM(error_free);
    SDL_DBUS_SYM(get_local_machine_id);
    SDL_DBUS_SYM(free);
    SDL_DBUS_SYM(free_string_array);
    SDL_DBUS_SYM(shutdown);

    #undef SDL_DBUS_SYM
    #undef SDL_DBUS_SYM2

    return 0;
}

static void UnloadDBUSLibrary(void)
{
    if (dbus_handle != NULL) {
        SDL_UnloadObject(dbus_handle);
        dbus_handle = NULL;
    }
}

static int LoadDBUSLibrary(void)
{
    int retval = 0;
    if (dbus_handle == NULL) {
        dbus_handle = SDL_LoadObject(dbus_library);
        if (dbus_handle == NULL) {
            retval = -1;
            /* Don't call SDL_SetError(): SDL_LoadObject already did. */
        } else {
            retval = LoadDBUSSyms();
            if (retval < 0) {
                UnloadDBUSLibrary();
            }
        }
    }

    return retval;
}

void DBus_Quit(void)
{
    if (dbus.system_conn) {
        dbus.connection_close(dbus.system_conn);
        dbus.connection_unref(dbus.system_conn);
    }
    if (dbus.session_conn) {
        dbus.connection_close(dbus.session_conn);
        dbus.connection_unref(dbus.session_conn);
    }
/* Don't do this - bug 3950
   dbus_shutdown() is a debug feature which closes all global resources in the dbus library. Calling this should be done by the app, not a library, because if there are multiple users of dbus in the process then SDL could shut it down even though another part is using it.
*/
#if 0
    if (dbus.shutdown) {
        dbus.shutdown();
    }
#endif
    SDL_zero(dbus);
    UnloadDBUSLibrary();
}

void DBus_Init(void)
{
    if (!dbus.session_conn && LoadDBUSLibrary() != -1) {
        DBusError err;
        dbus.error_init(&err);
        dbus.session_conn = dbus.bus_get_private(DBUS_BUS_SESSION, &err);
        if (!dbus.error_is_set(&err)) {
            dbus.system_conn = dbus.bus_get_private(DBUS_BUS_SYSTEM, &err);
        }
        if (dbus.error_is_set(&err)) {
            dbus.error_free(&err);
            DBus_Quit();
            return;  /* oh well */
        }
        dbus.connection_set_exit_on_disconnect(dbus.system_conn, 0);
        dbus.connection_set_exit_on_disconnect(dbus.session_conn, 0);
    }
}

DBusContext * DBus_GetContext(void)
{
    if(!dbus_handle || !dbus.session_conn){
        DBus_Init();
    }

    if(dbus_handle && dbus.session_conn){
        return &dbus;
    } else {
        return NULL;
    }
}

static SDL_bool DBus_CallVoidMethodInternal(DBusConnection *conn, const char *node, const char *path, const char *interface, const char *method, va_list ap)
{
    SDL_bool retval = SDL_FALSE;

    if (conn) {
        DBusMessage *msg = dbus.message_new_method_call(node, path, interface, method);
        if (msg) {
            int firstarg = va_arg(ap, int);
            if ((firstarg == DBUS_TYPE_INVALID) || dbus.message_append_args_valist(msg, firstarg, ap)) {
                if (dbus.connection_send(conn, msg, NULL)) {
                    dbus.connection_flush(conn);
                    retval = SDL_TRUE;
                }
            }

            dbus.message_unref(msg);
        }
    }

    return retval;
}

}

void DBus::Init()
{
    impl::DBus_Init();
}

void DBus::Shutdown()
{
    impl::DBus_Quit();
}

bool DBus::CallVoidMethod(const std::string_view node, const std::string_view path, const std::string_view interface, const std::string_view method, ...)
{
    SDL_bool retval;

    va_list ap;
    va_start(ap, method);

    retval = impl::DBus_CallVoidMethodInternal(impl::dbus.system_conn, node.data(), path.data(), interface.data(), method.data(), ap);

    va_end(ap);

    return retval;
}


}
