#pragma once

#include "AppResult.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace launcher {

/// Аргументы командной строки DevilutionX для режима, выбранного
/// в лаунчере (имя самого исполняемого файла не входит).
///
/// - Diablo:    --diablo  (пропускает диалог движка «какую игру запустить?»
///                        при установленных файлах и Diablo, и Hellfire)
/// - Hellfire:  --hellfire
/// - Shareware: --spawn   (файл spawn.mpq движок находит сам через
///                        свои пути поиска, лишние аргументы не нужны)
///
/// Для полных версий выбранная папка передаётся ещё и как
/// `--data-dir <путь>`: движок ставит её первой в порядок поиска MPQ,
/// поэтому DIABDAT.MPQ находится гарантированно (и без риска незаметно
/// подменить игру скачанной демо-версией). Папка переживает перезапуск —
/// движок читает её из launcher.ini (см. AuroraOsStandartPaths).
[[nodiscard]] std::vector<std::string> EngineArgsFor(ExitAction action, const std::filesystem::path &dataPath = {});

} // namespace launcher
