#include "GameFiles.hpp"

#include <algorithm>
#include <cctype>

namespace launcher {

namespace {

bool IEquals(std::string_view a, std::string_view b)
{
	return a.size() == b.size()
	    && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
		       return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
	       });
}

} // namespace

bool IsKnownFileName(std::string_view fileName, KnownFile file)
{
	const FileSpec &spec = FileSpecOf(file);
	return IEquals(fileName, spec.canonical);
}

std::string_view DownloadUrl(KnownFile file)
{
	switch (file) {
	case KnownFile::Spawn:
		return "https://github.com/diasurgical/devilutionx-assets/releases/download/v5/spawn.mpq";
	case KnownFile::RuVoice:
		return "https://github.com/diasurgical/devilutionx-assets/releases/download/v5/ru.mpq";
	default:
		return {};
	}
}

} // namespace launcher
