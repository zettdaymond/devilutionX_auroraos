#include "ConfigService.hpp"

#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

#include <fstream>
#include <string_view>

namespace launcher {

namespace {
constexpr std::string_view kDataFolderKey = "data_folder";
} // namespace

ConfigService::ConfigService(std::filesystem::path configFilePath)
    : m_filePath(std::move(configFilePath))
{
}

LauncherConfig ConfigService::load()
{
	LauncherConfig config;

	if (!std::filesystem::exists(m_filePath)) {
		return config;
	}

	try {
		auto table = toml::parse_file(m_filePath.string());
		if (auto folder = table[kDataFolderKey].value<std::string>()) {
			config.dataFolder = std::filesystem::path(*folder);
		}
	} catch (const toml::parse_error &err) {
		spdlog::error("Failed to parse {}: {}. Using defaults.", m_filePath.string(), err.description());
	}

	return config;
}

void ConfigService::save(const LauncherConfig &config)
{
	toml::table table;
	if (config.dataFolder) {
		table.insert(kDataFolderKey, config.dataFolder->string());
	}

	std::error_code ec;
	const std::filesystem::path tmp = m_filePath;
	auto stream = std::ofstream(tmp.string() + ".tmp", std::ios::trunc);
	if (!stream) {
		spdlog::error("Failed to open {} for writing", tmp.string());
		return;
	}

	stream << table << std::endl;

	if (!stream.good()) {
		spdlog::error("Failed to write {}", tmp.string());
		return;
	}

	stream.close();
	std::filesystem::rename(tmp.string() + ".tmp", m_filePath, ec);
	if (ec) {
		// Fall back to a plain overwrite when rename is not possible.
		std::ofstream direct(m_filePath, std::ios::trunc);
		direct << table << std::endl;
	}
}

} // namespace launcher
