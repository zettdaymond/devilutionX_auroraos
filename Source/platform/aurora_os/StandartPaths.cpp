#include "StandartPaths.hpp"

#include <array>

#include <QStandardPaths>
#include <QtGui/QGuiApplication>

#include <SDL2/SDL.h>

namespace devilution {

struct AppContextImpl : public AuroraOsStandartPaths::AppContext
{
    ~AppContextImpl() override
    {

    }

    std::unique_ptr<QGuiApplication> app;
};

std::string AuroraOsStandartPaths::GetWritableDataPath()
{
    auto str = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).toStdString() + "/";
    return str;
}

std::string AuroraOsStandartPaths::GetConfigPath()
{
    return GetWritableDataPath();
}

std::string AuroraOsStandartPaths::GetBundledAssetsPath()
{
    return std::string("/usr/share/org.diasurgical.devilutionx/assets/");
}

std::string AuroraOsStandartPaths::GetAdditionalMPQSearchPath()
{
    auto str = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toStdString() + "/devilutionx/";
    return str;
}

std::unique_ptr<AuroraOsStandartPaths::AppContext> AuroraOsStandartPaths::MakeAppContext(int argc, char **argv)
{
    auto context = std::make_unique<AppContextImpl>();

    QGuiApplication::setOrganizationName(QStringLiteral("org.diasurgical"));
    QGuiApplication::setApplicationName(QStringLiteral("devilutionx"));

    context->app = std::unique_ptr<QGuiApplication>(nullptr);

    return context;
}


AuroraOsStandartPaths::AppContext::~AppContext()
{

}

}
