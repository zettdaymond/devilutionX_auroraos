#pragma once

#include "IConfigService.hpp"
#include "IDownloadService.hpp"
#include "IGameFilesService.hpp"
#include "IPathProvider.hpp"

#include <filesystem>
#include <memory>

namespace launcher {

/// Полный набор сервисов, готовый для передачи в Store.
/// Собирается либо из настоящих сервисов платформы, либо из имитаций.
struct ServiceBundle {
	std::unique_ptr<IConfigService> config;
	std::unique_ptr<IGameFilesService> files;
	std::unique_ptr<IDownloadService> downloads;
	std::unique_ptr<IPathProvider> paths;
};

/// Настоящие реализации. `baseDir` — путь настроек SDL, который
/// получает вызывающий код (Application или десктопный main).
/// На Aurora OS поставщик путей дополнительно отдаёт папки MPQ платформы.
[[nodiscard]] ServiceBundle MakeRealServices(std::filesystem::path baseDir);

} // namespace launcher
