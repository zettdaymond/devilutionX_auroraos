#include "core/EngineLaunch.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace launcher {
namespace {

TEST(EngineLaunchTest, DiabloForcesDiabloModeAndPassesDataDir)
{
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchDiablo, std::filesystem::path("/data/games"));
	ASSERT_EQ(args.size(), 3U);
	EXPECT_EQ(args[0], "--diablo");
	EXPECT_EQ(args[1], "--data-dir");
	EXPECT_EQ(args[2], "/data/games");
}

TEST(EngineLaunchTest, HellfireForcesHellfireModeAndPassesDataDir)
{
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchHellfire, std::filesystem::path("C:/games"));
	ASSERT_EQ(args.size(), 3U);
	EXPECT_EQ(args[0], "--hellfire");
	EXPECT_EQ(args[1], "--data-dir");
	EXPECT_EQ(args[2], "C:/games");
}

TEST(EngineLaunchTest, DataDirOmittedWhenNoFolderSelected)
{
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchDiablo, {});
	ASSERT_EQ(args.size(), 1U);
	EXPECT_EQ(args[0], "--diablo");
}

TEST(EngineLaunchTest, DemoForcesSharewareWithoutDataDir)
{
	// spawn.mpq lives in the engine's own search path; --data-dir must not
	// be passed so the engine cannot accidentally prefer a user folder
	// over the downloaded spawn.mpq.
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchDemo, std::filesystem::path("/data/games"));
	ASSERT_EQ(args.size(), 1U);
	EXPECT_EQ(args[0], "--spawn");
}

} // namespace
} // namespace launcher
