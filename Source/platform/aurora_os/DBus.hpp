#pragma once

#include <string_view>

namespace devilution {

/// Тонкий клиент системной шины D-Bus: libdbus линкуется напрямую,
/// dlopen-биндинги не нужны. Достаточно асинхронной отправки void-методов
/// (mce-запреты гашения экрана из DisplayBlankerController).
class DBus
{
public:
    static void Init();
    static void Shutdown();

    static bool CallVoidMethod(std::string_view node, std::string_view path,
        std::string_view interface, std::string_view method);
};

}
