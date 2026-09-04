#pragma once

#include "AppResult.hpp"
#include "EngineOptions.hpp"
#include "GameFiles.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace launcher {

/// Верхний уровень: какой экран показывает лаунчер. Навигация
/// взаимоисключающая — один экран, плюс не более одного диалога.
enum class Screen : uint8_t {
	Home,
	Data,
	Settings,
	About,
};

/// Диалог поверх экрана.
enum class Dialog : uint8_t {
	None,
	ConfirmDownloadDemo,
	ConfirmDownloadRu,
	DownloadProgress,
	HellfireMissingFiles,
	ConfirmResetSettings,
	Error,
};

/// Доступность одного запускаемого режима игры.
struct GameCardState {
	bool available = false;                     // every required file is present
	std::vector<KnownFile> missingFiles = {};   // what is missing (for the Hellfire warning)
};

/// Состояние одной загрузки (демо или русская озвучка).
struct DownloadState {
	KnownFile file = KnownFile::Spawn;
	bool active = false;
	float fraction = 0.0F;          // 0..1
	int64_t totalBytes = 0;
	int64_t downloadedBytes = 0;
	int64_t bytesPerSec = 0;
	std::string error;              // non-empty => failed panel in the overlay
};

/// Единственный источник истины для всего интерфейса лаунчера.
/// Интерфейс — чистая функция от этой структуры; меняет её
/// только Store, и только на главном потоке.
struct LauncherState {
// Навигация
	Screen screen = Screen::Home;
	Dialog dialog = Dialog::None;

// Найденные файлы игры
	std::filesystem::path dataFolder;                                // user-selected folder (may be empty)
	std::array<int64_t, kKnownFileCount> fileSizes {};               // -1 = not found
	std::array<std::filesystem::path, kKnownFileCount> fileFolders {};// where the file was found
	int64_t freeDiskBytes = 0;

// Вычисленная доступность режимов
	GameCardState diablo;
	GameCardState hellfire;
	GameCardState demo;
	bool russianVoiceInstalled = false;

// Текущая загрузка
	std::optional<DownloadState> download;

// Настройки движка (diablo.ini); значения — в единицах каталога
// (bool 0/1, проценты для громкостей), индекс — SettingId.
	std::array<int, kSettingCount> settingValues = DefaultSettingValues();
	bool settingsLoaded = false;

// Данные для диалога
	std::vector<KnownFile> hellfireMissing; // for Dialog::HellfireMissingFiles
	std::string errorText;                  // for Dialog::Error

// Краткоживущее состояние интерфейса
	bool fileBrowserOpen = false;
	bool exitRequested = false;             // window close / SDL_QUIT
	std::optional<ExitAction> pendingLaunch;// set => the UI loop ends and the game starts

// Короткое уведомление внизу экрана
	std::optional<std::string> toast;

	[[nodiscard]] bool HasAnyFiles() const
	{
		for (int64_t size : fileSizes) {
			if (size >= 0)
				return true;
		}
		return false;
	}

	/// Запускается ли хоть один режим. Только ru.mpq (или частичные
	/// файлы Hellfire) — НЕ повод уводить главный экран с first-run:
	/// там пользователь видит предложение скачать демо или указать
	/// папку, а не замок «Hellfire: требуются файлы».
	[[nodiscard]] bool HasPlayableMode() const
	{
		return diablo.available || hellfire.available || demo.available;
	}

	[[nodiscard]] bool DownloadInProgress() const { return download.has_value() && download->active; }
};

} // namespace launcher
