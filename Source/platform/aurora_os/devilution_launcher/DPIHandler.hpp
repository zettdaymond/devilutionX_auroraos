#pragma once

namespace App {

/**
 * @brief Масштаб экрана для HiDPI-устройств.
 */
class DPIHandler {
public:
	/// Коэффициент масштаба дисплея (96 dpi = 1.0).
	[[nodiscard]] static auto GetScale() -> float;
};

} // namespace App
