#include "StandartPaths.hpp"

#include <array>

#include <QSettings>
#include <QStandardPaths>
#include <QtGui/QGuiApplication>

#include <SDL2/SDL.h>

namespace devilution {

struct AppContextImpl : public AuroraOsStandartPaths::AppContext {
    ~AppContextImpl() override
    {}

    std::unique_ptr<QGuiApplication> app;
    std::unique_ptr<QSettings> settings;
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
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).toStdString() + "/";
}

std::optional<std::string> AuroraOsStandartPaths::GetUserDefinedMPQSearchPath()
{
    QSettings s(QStringLiteral("org.diasurgical"), QStringLiteral("devilutionx"));

    auto dir = s.value("userDataDirectory");

    if (dir.isNull()) {
        return {};
    }

    return dir.toString().toStdString();
}

std::unique_ptr<AuroraOsStandartPaths::AppContext> AuroraOsStandartPaths::MakeAppContext(int argc, char** argv)
{
    auto context = std::make_unique<AppContextImpl>();

    QGuiApplication::setOrganizationName(QStringLiteral("org.diasurgical"));
    QGuiApplication::setApplicationName(QStringLiteral("devilutionx"));

    context->app = std::unique_ptr<QGuiApplication>(nullptr);

    return context;
}

AuroraOsStandartPaths::AppContext::~AppContext()
{}

} // namespace devilution
