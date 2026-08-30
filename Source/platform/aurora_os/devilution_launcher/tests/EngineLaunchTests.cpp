#include "core/EngineLaunch.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace launcher {
namespace {

TEST(EngineLaunchTest, DiabloForcesDiabloMode)
{
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchDiablo);
	ASSERT_EQ(args.size(), 1U);
	EXPECT_EQ(args[0], "--diablo");
}

TEST(EngineLaunchTest, HellfireForcesHellfireMode)
{
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchHellfire);
	ASSERT_EQ(args.size(), 1U);
	EXPECT_EQ(args[0], "--hellfire");
}

TEST(EngineLaunchTest, DemoForcesSharewareMode)
{
	const std::vector<std::string> args = EngineArgsFor(ExitAction::LaunchDemo);
	ASSERT_EQ(args.size(), 1U);
	EXPECT_EQ(args[0], "--spawn");
}

TEST(EngineLaunchTest, UserMpqPathOnlyForFullGames)
{
	EXPECT_TRUE(WantsUserMpqPath(ExitAction::LaunchDiablo));
	EXPECT_TRUE(WantsUserMpqPath(ExitAction::LaunchHellfire));
	EXPECT_FALSE(WantsUserMpqPath(ExitAction::LaunchDemo));
}

} // namespace
} // namespace launcher
