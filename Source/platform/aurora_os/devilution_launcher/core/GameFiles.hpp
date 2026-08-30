#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace launcher {

/// Identifies every MPQ file the launcher knows about.
/// Order matters: it is used to index arrays in LauncherState.
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

/// Static description of a known game file: canonical lower-case name,
/// alternative spellings found in the wild and the expected download size
/// (only meaningful for downloadable files, 0 otherwise).
struct FileSpec {
	KnownFile id;
	std::string_view canonical;
	std::string_view displayName; // Russian name for the data screen
	int64_t expectedSizeBytes;
	bool downloadable;
};

/// Catalog of all known files. Keep in sync with KnownFile.
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

/// Case-insensitive comparison of a file name against the known aliases.
/// Only the plain canonical name is accepted — in practice MPQs ship in
/// lower or upper case, both covered here.
bool IsKnownFileName(std::string_view fileName, KnownFile file);

/// URL a downloadable file is fetched from (empty when not downloadable).
std::string_view DownloadUrl(KnownFile file);

} // namespace launcher
