#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace launcher {

/// Перечисляет все MPQ-файлы, о которых знает лаунчер.
/// Порядок важен: по нему индексируются массивы в LauncherState.
enum class KnownFile : uint8_t {
	Diabdat,
	Hellfire,
	HfMonk,
	HfMusic,
	HfVoice,
	Spawn,
	RuVoice,

	Count
};

constexpr size_t kKnownFileCount = static_cast<size_t>(KnownFile::Count);

/// Неизменное описание известного файла: каноничное имя в нижнем регистре,
/// встречающиеся в жизни варианты написания и ожидаемый размер загрузки
/// (имеет смысл только для скачиваемых файлов, иначе 0).
struct FileSpec {
	KnownFile id;
	std::string_view canonical;
	std::string_view displayName; // Russian name for the data screen
	int64_t expectedSizeBytes;
	bool downloadable;
};

/// Каталог всех известных файлов. Держать синхронно с KnownFile.
inline constexpr std::array<FileSpec, kKnownFileCount> kFileCatalog { {
	{ KnownFile::Diabdat, "diabdat.mpq", "DIABDAT.MPQ (оригинальный Diablo)", 0, false },
	{ KnownFile::Hellfire, "hellfire.mpq", "hellfire.mpq (дополнение)", 0, false },
	{ KnownFile::HfMonk, "hfmonk.mpq", "hfmonk.mpq (монах Hellfire)", 0, false },
	{ KnownFile::HfMusic, "hfmusic.mpq", "hfmusic.mpq (музыка Hellfire)", 0, false },
	{ KnownFile::HfVoice, "hfvoice.mpq", "hfvoice.mpq (озвучка Hellfire)", 0, false },
	{ KnownFile::Spawn, "spawn.mpq", "spawn.mpq (демо-версия)", 52 * 1024 * 1024, true },
	{ KnownFile::RuVoice, "ru.mpq", "ru.mpq (русская озвучка)", 150 * 1024 * 1024, true },
} };

constexpr const FileSpec &FileSpecOf(KnownFile file)
{
	return kFileCatalog[static_cast<size_t>(file)];
}

/// Сравнение имени файла с известными вариантами без учёта регистра.
/// Принимается только каноничное имя — на практике MPQ встречаются
/// в нижнем и верхнем регистрах, оба покрыты.
bool IsKnownFileName(std::string_view fileName, KnownFile file);

/// Откуда скачивается файл (пусто, если файл не скачивается).
std::string_view DownloadUrl(KnownFile file);

} // namespace launcher
