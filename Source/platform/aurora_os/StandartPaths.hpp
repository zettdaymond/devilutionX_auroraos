#pragma once

#include <string>
#include <optional>
#include <memory>

class QGuiApplication;

namespace devilution {

class AuroraOsStandartPaths
{
public:
    struct AppContext
    {
        virtual ~AppContext();
    };

    static std::string GetWritableDataPath();

    static std::string GetConfigPath();

    static std::string GetBundledAssetsPath();

    static std::string GetAdditionalMPQSearchPath();

    static std::optional<std::string> GetUserDefinedMPQSearchPath();

    static std::unique_ptr<AppContext> MakeAppContext(int argc, char** argv);
};

}
