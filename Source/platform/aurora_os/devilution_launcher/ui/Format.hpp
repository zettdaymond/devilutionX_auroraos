#pragma once

#include <cstdint>
#include <string>

namespace launcher::ui {

/// "52 МБ", "1.2 ГБ" style human-readable byte sizes.
[[nodiscard]] std::string FormatBytes(int64_t bytes);

/// "осталось 1 мин 20 с" style remaining-time text; empty when unknown.
[[nodiscard]] std::string FormatEta(int64_t seconds);

} // namespace launcher::ui
