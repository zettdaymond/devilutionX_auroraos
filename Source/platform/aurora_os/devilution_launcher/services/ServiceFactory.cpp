#include "ServiceFactory.hpp"

#include "ConfigService.hpp"
#include "EngineOptionsService.hpp"
#include "GameFilesService.hpp"
#include "ZoeDownloadService.hpp"

#ifdef AURORA_OS
#   include "AuroraPathProvider.hpp"
#else
#   include "DesktopPathProvider.hpp"
#endif

#include <utility>

namespace launcher {

ServiceBundle MakeRealServices(std::filesystem::path baseDir, std::filesystem::path engineIniPath)
{
	ServiceBundle bundle;
	bundle.config = std::make_unique<ConfigService>(baseDir / "launcher.conf");
	bundle.files = std::make_unique<GameFilesService>();
	bundle.downloads = std::make_unique<ZoeDownloadService>();
	bundle.engineOptions = std::make_unique<EngineOptionsService>(std::move(engineIniPath));

#ifdef AURORA_OS
	bundle.paths = std::make_unique<AuroraPathProvider>(std::move(baseDir));
#else
	bundle.paths = std::make_unique<DesktopPathProvider>(std::move(baseDir));
#endif

	return bundle;
}

} // namespace launcher
