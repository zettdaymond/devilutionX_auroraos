#include "GameModel.hpp"

#include <fstream>

#include <toml++/toml.hpp>

// #include <fmt/format.h>
#include <spdlog/spdlog.h>

using namespace std::literals;

namespace {
auto CreateDefaultConfigFile(std::filesystem::path modelFilePath)
{
    auto tbl = toml::table{};

    std::ofstream fs(modelFilePath);
    fs << tbl << std::endl;
}

} // namespace

GameModel::GameModel(std::filesystem::path modelFilePath)
   : m_modelFilePath(modelFilePath)
{
    if (!std::filesystem::exists(modelFilePath)) {
        // Create config file
        CreateDefaultConfigFile(modelFilePath);
    }

    try {
        auto tbl = toml::parse_file(modelFilePath.string());
    } catch (const toml::parse_error& err) {
        spdlog::error("Could not deserialize toml configuration file: {} couse of {}. Recreate file from scratch... ",
                      modelFilePath.string(),
                      err.description());
        std::filesystem::remove(modelFilePath);
        CreateDefaultConfigFile(modelFilePath);
    }

    m_configTable = std::move(toml::parse_file(modelFilePath.string()));
}

void GameModel::SetDiabloResourcesPath(const std::optional<std::string>& path)
{
    auto value = m_configTable["mpq_path"];

    if(value && path) {
        //update
        value.ref<std::string&>() = path.value_or("");
    }
    else if(value && !path){
        //remove
        m_configTable.erase("mpq_path");
    }
    else if(!value && path) {
        //add
        m_configTable.insert("mpq_path", path.value());
    }

    std::ofstream fs(m_modelFilePath);
    fs << m_configTable << std::endl;
    fs.close();
}

std::optional<std::filesystem::path> GameModel::GetDiabloResourcesPath()
{
    auto value = m_configTable["mpq_path"].value<std::string>();
    return value;
}

void GameModel::SetDemoResourcesPath(const std::optional<std::string>& path)
{
    auto value = m_configTable["demo_path"];

    if(value && path) {
        //update
        value.ref<std::string&>() = path.value_or("");
    }
    else if(value && !path){
        //remove
        m_configTable.erase("demo_path");
    }
    else if(!value && path) {
        //add
        m_configTable.insert("demo_path", path.value());
    }

    std::ofstream fs(m_modelFilePath);
    fs << m_configTable << std::endl;
    fs.close();
}

std::optional<std::filesystem::path> GameModel::GetDemoResourcesPath()
{
    auto value = m_configTable["demo_path"].value<std::string>();
    return value;
}
