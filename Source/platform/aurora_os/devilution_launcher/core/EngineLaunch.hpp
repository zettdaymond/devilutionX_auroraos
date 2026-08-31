#pragma once

#include "AppResult.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace launcher {

/// DevilutionX command-line arguments for the mode the user picked in
/// the launcher (the program name is NOT included).
///
/// - Diablo:    --diablo  (skips the engine's "which game?" dialog when
///                        both Diablo and Hellfire files are installed)
/// - Hellfire:  --hellfire
/// - Shareware: --spawn   (the engine finds spawn.mpq through its MPQ
///                        search paths, no extra argument needed)
///
/// For the full games the picked folder is also passed as
/// `--data-dir <path>`: the engine puts it first in its MPQ search
/// order, which guarantees DIABDAT.MPQ is found without relying on the
/// QSettings round-trip (and prevents a silent fallback to the
/// downloaded shareware spawn.mpq).
[[nodiscard]] std::vector<std::string> EngineArgsFor(ExitAction action, const std::filesystem::path &dataPath = {});

/// Whether the launcher result should ALSO persist `dataPath` as the
/// engine's user-defined MPQ search path (QSettings on Aurora OS) so
/// the folder is remembered on the next run.
[[nodiscard]] bool WantsUserMpqPath(ExitAction action);

} // namespace launcher
