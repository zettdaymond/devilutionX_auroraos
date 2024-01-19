#include "StandartPaths.hpp"

#include <array>

#include <QStandardPaths>
#include <QtGui/QGuiApplication>

#include <auroraapp.h>

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
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).toStdString() + "/";
}

std::string AuroraOsStandartPaths::GetConfigPath()
{
    return GetWritableDataPath();
}

std::string AuroraOsStandartPaths::GetBundledAssetsPath()
{
    return Aurora::Application::getPath(Aurora::Application::PathType::PackageFilesLocation).toStdString() + "/assets/";
}

std::string AuroraOsStandartPaths::GetAdditionalMPQSearchPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toStdString() + "/devilutionx/";
}

std::unique_ptr<AuroraOsStandartPaths::AppContext> AuroraOsStandartPaths::MakeAppContext(int argc, char **argv)
{
    auto context = std::make_unique<AppContextImpl>();

    auto application = Aurora::Application::application(argc, argv);
    application->setOrganizationName(QStringLiteral("org.diasurgical"));
    application->setApplicationName(QStringLiteral("devilutionx"));

    context->app = std::unique_ptr<QGuiApplication>(application);

    return context;
}


AuroraOsStandartPaths::AppContext::~AppContext()
{

}

}
