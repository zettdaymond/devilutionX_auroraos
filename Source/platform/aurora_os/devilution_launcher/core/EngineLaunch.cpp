#include "EngineLaunch.hpp"

namespace launcher {

std::vector<std::string> EngineArgsFor(ExitAction action)
{
	switch (action) {
	case ExitAction::LaunchDiablo:
		return { "--diablo" };
	case ExitAction::LaunchHellfire:
		return { "--hellfire" };
	case ExitAction::LaunchDemo:
		return { "--spawn" };
	}
	return {};
}

bool WantsUserMpqPath(ExitAction action)
{
	// The shareware demo relies on the engine's own search paths
	// (spawn.mpq is downloaded into the additional MPQ directory).
	return action == ExitAction::LaunchDiablo || action == ExitAction::LaunchHellfire;
}

} // namespace launcher
