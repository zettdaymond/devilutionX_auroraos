#include "EngineLaunch.hpp"

namespace launcher {

std::vector<std::string> EngineArgsFor(ExitAction action, const std::filesystem::path &dataPath)
{
	std::vector<std::string> args;

	switch (action) {
	case ExitAction::LaunchDiablo:
		args.push_back("--diablo");
		break;
	case ExitAction::LaunchHellfire:
		args.push_back("--hellfire");
		break;
	case ExitAction::LaunchDemo:
		args.push_back("--spawn");
		return args;
	}

	// The shareware demo relies on the engine's own search paths
	// (spawn.mpq is downloaded into the additional MPQ directory).
	if (!dataPath.empty()) {
		args.push_back("--data-dir");
		args.push_back(dataPath.string());
	}
	return args;
}

bool WantsUserMpqPath(ExitAction action)
{
	return action == ExitAction::LaunchDiablo || action == ExitAction::LaunchHellfire;
}

} // namespace launcher
