#include "Format.hpp"

#include <cstdio>

namespace launcher::ui {

std::string FormatBytes(int64_t bytes)
{
	char buffer[32];
	if (bytes < 0) {
		return "?";
	}
	constexpr int64_t kMb = 1024 * 1024;
	constexpr int64_t kGb = 1024 * kMb;
	if (bytes >= kGb) {
		std::snprintf(buffer, sizeof(buffer), "%.1f ГБ", static_cast<double>(bytes) / kGb);
	} else if (bytes >= kMb) {
		std::snprintf(buffer, sizeof(buffer), "%.1f МБ", static_cast<double>(bytes) / kMb);
	} else if (bytes >= 1024) {
		std::snprintf(buffer, sizeof(buffer), "%.0f КБ", static_cast<double>(bytes) / 1024);
	} else {
		std::snprintf(buffer, sizeof(buffer), "%lld Б", static_cast<long long>(bytes));
	}
	return buffer;
}

std::string FormatEta(int64_t seconds)
{
	if (seconds < 0) {
		return {};
	}
	char buffer[48];
	if (seconds >= 3600) {
		std::snprintf(buffer, sizeof(buffer), "%d ч %d мин", static_cast<int>(seconds / 3600),
		    static_cast<int>((seconds % 3600) / 60));
	} else if (seconds >= 60) {
		std::snprintf(buffer, sizeof(buffer), "%d мин %d с", static_cast<int>(seconds / 60),
		    static_cast<int>(seconds % 60));
	} else {
		std::snprintf(buffer, sizeof(buffer), "%d с", static_cast<int>(seconds));
	}
	return std::string("осталось ") + buffer;
}

} // namespace launcher::ui
