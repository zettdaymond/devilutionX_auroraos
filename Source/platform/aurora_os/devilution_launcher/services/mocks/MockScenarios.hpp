#pragma once

#include "../ServiceFactory.hpp"

#include "MockDownloadService.hpp"
#include "MockWorld.hpp"

#include <memory>
#include <string>

namespace launcher {

/// Named scenario for desktop user-flow walkthroughs (no device, no
/// network, no real files). Combines a MockWorld file preset with a
/// download behavior:
///
///   empty          no files, downloads finish instantly
///   slow-download  no files, downloads take ~12 s (progress/cancel)
///   fail-download  no files, downloads fail at 50 %
///   diablo-found   DIABDAT.MPQ present
///   hellfire-partial DIABDAT + 2 of 4 hellfire files
///   full           everything present (data management flows)
class MockScenario {
public:
	/// Parses a scenario name; returns a scenario using MockWorld
	/// services. Throws std::invalid_argument for unknown names.
	[[nodiscard]] static MockScenario byName(const std::string &name);

	[[nodiscard]] ServiceBundle makeBundle() const;

	[[nodiscard]] const std::string &name() const { return m_name; }

	/// Access to the world, e.g. for tests to mutate state mid-flow.
	[[nodiscard]] MockWorld &world() const { return *m_world; }

	static const std::vector<std::string> &names();

private:
	MockScenario(std::string name, MockDownloadBehavior behavior);

	std::string m_name;
	MockDownloadBehavior m_behavior;
	std::shared_ptr<MockWorld> m_world;
};

} // namespace launcher
