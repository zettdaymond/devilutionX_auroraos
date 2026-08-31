#include "MockScenarios.hpp"

#include "../DesktopPathProvider.hpp"
#include "MockDownloadService.hpp"
#include "MockServices.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace launcher {

namespace {

struct ScenarioDef {
	std::string name;
	MockDownloadBehavior behavior;
	const char *worldPreset; // nullptr = start empty
};

const std::vector<ScenarioDef> &scenarioDefs()
{
	static const std::vector<ScenarioDef> defs {
		{ "empty", MockDownloadBehavior::InstantSuccess, nullptr },
		{ "slow-download", MockDownloadBehavior::SlowSuccess, nullptr },
		{ "fail-download", MockDownloadBehavior::FailAtHalf, nullptr },
		{ "diablo-found", MockDownloadBehavior::InstantSuccess, "diablo-found" },
		{ "hellfire-partial", MockDownloadBehavior::InstantSuccess, "hellfire-partial" },
		{ "hellfire-files-only", MockDownloadBehavior::InstantSuccess, "hellfire-files-only" },
		{ "demo-installed", MockDownloadBehavior::InstantSuccess, "demo-installed" },
		{ "full", MockDownloadBehavior::InstantSuccess, "full" },
	};
	return defs;
}

} // namespace

MockScenario MockScenario::ByName(const std::string &name)
{
	const auto &defs = scenarioDefs();
	const auto it = std::find_if(defs.begin(), defs.end(), [&](const ScenarioDef &d) { return d.name == name; });
	if (it == defs.end()) {
		throw std::invalid_argument("Unknown mock scenario: " + name);
	}
	return MockScenario(it->name, it->behavior);
}

const std::vector<std::string> &MockScenario::Names()
{
	static const std::vector<std::string> names = [] {
		std::vector<std::string> result;
		for (const ScenarioDef &def : scenarioDefs()) {
			result.push_back(def.name);
		}
		return result;
	}();
	return names;
}

MockScenario::MockScenario(std::string name, MockDownloadBehavior behavior)
    : m_name(std::move(name))
    , m_behavior(behavior)
    , m_world(std::make_shared<MockWorld>())
{
	for (const ScenarioDef &def : scenarioDefs()) {
		if (def.name == m_name && def.worldPreset != nullptr) {
			m_world->ApplyPreset(def.worldPreset);
		}
	}
}

ServiceBundle MockScenario::MakeBundle() const
{
	ServiceBundle bundle;
	bundle.config = std::make_unique<MockConfigService>();
	bundle.files = std::make_unique<MockGameFilesService>(m_world);
	bundle.downloads = std::make_unique<MockDownloadService>(m_world, m_behavior);
	bundle.paths = std::make_unique<DesktopPathProvider>(std::filesystem::temp_directory_path() / "devilutionx-mock");
	return bundle;
}

} // namespace launcher
