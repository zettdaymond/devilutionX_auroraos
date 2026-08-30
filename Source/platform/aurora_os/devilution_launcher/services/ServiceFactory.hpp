#pragma once

#include "IConfigService.hpp"
#include "IDownloadService.hpp"
#include "IGameFilesService.hpp"
#include "IPathProvider.hpp"

#include <filesystem>
#include <memory>

namespace launcher {

/// A complete set of service implementations, ready to be wired into
/// the Store. Built either from real platform services or from mocks.
struct ServiceBundle {
	std::unique_ptr<IConfigService> config;
	std::unique_ptr<IGameFilesService> files;
	std::unique_ptr<IDownloadService> downloads;
	std::unique_ptr<IPathProvider> paths;
};

/// Real implementations. `baseDir` is the SDL preference path obtained
/// by the caller (Application / desktop main). On Aurora OS the path
/// provider additionally exposes the platform MPQ locations.
[[nodiscard]] ServiceBundle MakeRealServices(std::filesystem::path baseDir);

} // namespace launcher
