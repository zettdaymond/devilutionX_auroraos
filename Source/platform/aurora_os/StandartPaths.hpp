#pragma once

#include <optional>
#include <string>

namespace devilution {

/// Пути порта на Aurora OS. Без Qt: каталог данных даёт SDL_GetPrefPath
/// (~/.local/share/org.diasurgical/devilutionx/), вшитые ассеты лежат в
/// /usr/share, папка с MPQ выбирается в лаунчере и хранится в его
/// launcher.ini.
class AuroraOsStandartPaths
{
public:
    static std::string GetWritableDataPath();

    static std::string GetConfigPath();

    static std::string GetBundledAssetsPath();

    /// Каталог данных приложения — сюда лаунчер складывает скачанные MPQ.
    static std::string GetAdditionalMPQSearchPath();

    /// Папка с файлами игры, выбранная в лаунчере (launcher.ini,
    /// секция Storage/DataFolder), если выбрана.
    static std::optional<std::string> GetUserDefinedMPQSearchPath();
};

}
