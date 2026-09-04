#include "StandartPaths.hpp"

#include <SimpleIni.h>
#include <SDL2/SDL_filesystem.h>

namespace devilution {

namespace {

// SDL_GetPrefPath создаёт каталог и аллоцирует строку — результат
// кэшируется на первый вызов (путь за жизнь процесса не меняется).
// Совпадает с прежним QStandardPaths::AppLocalDataLocation:
// ~/.local/share/org.diasurgical/devilutionx/
std::string PrefPathOnce()
{
    static const std::string path = [] {
        char *pref = SDL_GetPrefPath("org.diasurgical", "devilutionx");
        std::string result = pref != nullptr ? pref : "";
        SDL_free(pref);
        return result;
    }();
    return path;
}

constexpr const char *kBundledAssetsPath = "/usr/share/org.diasurgical.devilutionx/assets/";

} // namespace

std::string AuroraOsStandartPaths::GetWritableDataPath()
{
    return PrefPathOnce();
}

std::string AuroraOsStandartPaths::GetConfigPath()
{
    return PrefPathOnce();
}

std::string AuroraOsStandartPaths::GetBundledAssetsPath()
{
    return kBundledAssetsPath;
}

std::string AuroraOsStandartPaths::GetAdditionalMPQSearchPath()
{
    return PrefPathOnce();
}

std::optional<std::string> AuroraOsStandartPaths::GetUserDefinedMPQSearchPath()
{
    // Папку выбирает пользователь на экране «Данные»; лаунчер хранит её
    // в своём launcher.ini — движок читает тот же файл. Флаги формата
    // как у ConfigService лаунчера (SetSpaces(false) влияет только на
    // запись и для чтения не важна).
    const std::string iniPath = PrefPathOnce() + "launcher.ini";

    CSimpleIniA ini;
    ini.SetMultiKey();
    if (ini.LoadFile(iniPath.c_str()) < SI_OK) {
        return {};
    }

    const char *folder = ini.GetValue("Storage", "DataFolder");
    if (folder == nullptr || folder[0] == '\0') {
        return {};
    }
    return std::string(folder);
}

} // namespace devilution
