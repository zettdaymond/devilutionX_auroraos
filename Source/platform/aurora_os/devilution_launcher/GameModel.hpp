#pragma once

#include <string>
#include <filesystem>

#include <toml++/toml.hpp>

class GameModel {
public:
    GameModel(std::filesystem::path modelFilePath);

    void SetDiabloResourcesPath(const std::optional<std::string>& path);
    auto GetDiabloResourcesPath() -> std::optional<std::filesystem::path>;

    void SetDemoResourcesPath(const std::optional<std::string>& path);
    auto GetDemoResourcesPath() -> std::optional<std::filesystem::path>;

private:
    std::filesystem::path m_modelFilePath;
    toml::v3::table m_configTable;
};
