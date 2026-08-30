#include "services/GameFilesService.hpp"

#include "core/GameFiles.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace launcher {
namespace {

class GameFilesServiceTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		m_dir = std::filesystem::temp_directory_path() / ("devilutionx-gfs-test-" + std::to_string(++s_counter));
		std::filesystem::create_directories(m_dir);
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_dir, ec);
	}

	void makeFile(const std::string &name, size_t size)
	{
		std::ofstream out(m_dir / name, std::ios::binary);
		std::string payload(size, 'x');
		out << payload;
	}

	std::filesystem::path m_dir;
	static int s_counter;
};

int GameFilesServiceTest::s_counter = 0;

TEST_F(GameFilesServiceTest, ScanIsCaseInsensitive)
{
	makeFile("DIABDAT.MPQ", 128);
	GameFilesService service;

	const FileScanResult result = service.scan({ m_dir });

	EXPECT_EQ(result.sizes[static_cast<size_t>(KnownFile::Diabdat)], 128);
	EXPECT_EQ(result.folders[static_cast<size_t>(KnownFile::Diabdat)], m_dir);
}

TEST_F(GameFilesServiceTest, MissingFilesAreNegative)
{
	GameFilesService service;

	const FileScanResult result = service.scan({ m_dir });

	for (size_t i = 0; i < kKnownFileCount; ++i) {
		EXPECT_EQ(result.sizes[i], -1) << "index " << i;
	}
}

TEST_F(GameFilesServiceTest, EarlierFolderWins)
{
	makeFile("spawn.mpq", 64);
	GameFilesService service;

	const FileScanResult result = service.scan({ m_dir, m_dir });

	EXPECT_EQ(result.sizes[static_cast<size_t>(KnownFile::Spawn)], 64);
}

TEST_F(GameFilesServiceTest, FreeSpaceIsPositive)
{
	GameFilesService service;
	EXPECT_GT(service.freeSpace(m_dir), 0);
}

TEST_F(GameFilesServiceTest, RemoveFileDeletesAndReports)
{
	makeFile("spawn.mpq", 32);
	GameFilesService service;

	EXPECT_TRUE(service.removeFile(m_dir, KnownFile::Spawn));
	EXPECT_FALSE(std::filesystem::exists(m_dir / "spawn.mpq"));
	EXPECT_FALSE(service.removeFile(m_dir, KnownFile::Spawn));
}

} // namespace
} // namespace launcher
