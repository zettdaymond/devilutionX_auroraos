#pragma once

#include <filesystem>

namespace launcher {

/// Какую игру лаунчер должен запустить, когда его цикл завершился.
enum class ExitAction {
	LaunchDiablo,
	LaunchHellfire,
	LaunchDemo,
};

/// Итоговый ответ лаунчера главной программе
/// (см. Source/main.cpp — он превращает действие в аргументы командной строки движка).
struct AppResult {
	bool success = false;
	ExitAction action = ExitAction::LaunchDiablo;
	std::filesystem::path dataPath; // folder containing the user's MPQ files
};

} // namespace launcher
