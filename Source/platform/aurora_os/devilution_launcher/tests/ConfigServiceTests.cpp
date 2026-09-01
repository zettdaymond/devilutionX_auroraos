#include "services/ConfigService.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace launcher {
namespace {

class ConfigServiceTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		m_dir = std::filesystem::temp_directory_path()
		    / ("devilutionx-config-service-test-" + std::to_string(++s_counter));
		std::filesystem::create_directories(m_dir);
		m_filePath = m_dir / "launcher.ini";
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_dir, ec);
	}

	std::filesystem::path m_dir;
	std::filesystem::path m_filePath;

	static int s_counter;
};

int ConfigServiceTest::s_counter = 0;

TEST_F(ConfigServiceTest, MissingFileGivesEmptyConfig)
{
	ConfigService service(m_filePath);
	const LauncherConfig config = service.Load();
	EXPECT_FALSE(config.dataFolder.has_value());
}

TEST_F(ConfigServiceTest, RoundTripThroughFile)
{
	LauncherConfig config;
	config.dataFolder = std::filesystem::path("C:/Games/Diablo");

	ConfigService writer(m_filePath);
	writer.Save(config);

	ConfigService reader(m_filePath);
	const LauncherConfig loaded = reader.Load();
	ASSERT_TRUE(loaded.dataFolder.has_value());
	EXPECT_EQ(*loaded.dataFolder, *config.dataFolder);
}

TEST_F(ConfigServiceTest, WrittenInIniFormatWithoutSpaces)
{
	LauncherConfig config;
	config.dataFolder = std::filesystem::path("/home/user/mpq");

	ConfigService(m_filePath).Save(config);

	std::ifstream stream(m_filePath);
	const std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	EXPECT_NE(content.find("[Storage]"), std::string::npos);
	EXPECT_NE(content.find("DataFolder=/home/user/mpq"), std::string::npos);
	EXPECT_EQ(content.find(" = "), std::string::npos);
}

TEST_F(ConfigServiceTest, ClearingFolderRemovesKey)
{
	LauncherConfig config;
	config.dataFolder = std::filesystem::path("/some/path");
	ConfigService(m_filePath).Save(config);

	config.dataFolder.reset();
	ConfigService(m_filePath).Save(config);

	const LauncherConfig loaded = ConfigService(m_filePath).Load();
	EXPECT_FALSE(loaded.dataFolder.has_value());
}

TEST_F(ConfigServiceTest, CorruptFileFallsBackToDefaults)
{
	{
		std::ofstream stream(m_filePath, std::ios::trunc);
		stream << "\xFF\xFE not an ini [[[";
	}

	const LauncherConfig config = ConfigService(m_filePath).Load();
	EXPECT_FALSE(config.dataFolder.has_value());
}

TEST_F(ConfigServiceTest, NoTmpFileLeftAfterSave)
{
	LauncherConfig config;
	config.dataFolder = std::filesystem::path("/x");
	ConfigService(m_filePath).Save(config);

	EXPECT_TRUE(std::filesystem::exists(m_filePath));
	EXPECT_FALSE(std::filesystem::exists(m_dir / "launcher.ini.tmp"));
}

TEST_F(ConfigServiceTest, SavePreservesForeignKeys)
{
	{
		std::ofstream stream(m_filePath, std::ios::trunc);
		stream << "[Storage]\nDataFolder=/old\nWindow=Maximized\n";
	}

	LauncherConfig config;
	config.dataFolder = std::filesystem::path("/new");
	ConfigService(m_filePath).Save(config);

	std::ifstream stream(m_filePath);
	const std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	EXPECT_NE(content.find("DataFolder=/new"), std::string::npos);
	EXPECT_NE(content.find("Window=Maximized"), std::string::npos);
}

} // namespace
} // namespace launcher
