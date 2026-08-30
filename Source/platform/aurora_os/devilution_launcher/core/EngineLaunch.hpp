#pragma once

#include "AppResult.hpp"

#include <vector>

namespace launcher {

/// DevilutionX command-line arguments for the mode the user picked in
/// the launcher (empty program name is NOT included).
///
/// - Diablo:    --diablo  (skips the engine's "which game?" dialog when
///                        both Diablo and Hellfire files are installed)
/// - Hellfire:  --hellfire
/// - Shareware: --spawn   (the engine finds spawn.mpq through its MPQ
///                        search paths, no extra argument needed)
[[nodiscard]] std::vector<std::string> EngineArgsFor(ExitAction action);

/// Whether the launcher result should install `dataPath` as the engine's
/// user-defined MPQ search path (QSettings on Aurora OS).
[[nodiscard]] bool WantsUserMpqPath(ExitAction action);

} // namespace launcher
