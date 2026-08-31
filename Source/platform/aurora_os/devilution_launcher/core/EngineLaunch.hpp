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
/// поэтому DIABDAT.MPQ находится гарантированно, без обхода настроек
/// QSettings (и без риска незаметно подменить игру
/// скачанной демо-версией).
[[nodiscard]] std::vector<std::string> EngineArgsFor(ExitAction action, const std::filesystem::path &dataPath = {});

/// Должен ли результат лаунчера ещё и запомнить `dataPath` как
/// пользовательский путь поиска MPQ (QSettings на Aurora OS), чтобы
/// папка сохранилась и на следующий запуск.
[[nodiscard]] bool WantsUserMpqPath(ExitAction action);

} // namespace launcher
