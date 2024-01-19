#pragma once

#include <string>
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

    static std::unique_ptr<AppContext> MakeAppContext(int argc, char** argv);
};

}
