#include "DPIHandler.hpp"

#include <SDL2/SDL.h>

namespace App {

auto DPIHandler::GetScale() -> float
{
	constexpr int kDisplayIndex = 0;
	constexpr float kReferenceDpi = 96.0F;
	float dpi = kReferenceDpi;
	SDL_GetDisplayDPI(kDisplayIndex, nullptr, &dpi, nullptr);
#ifndef AURORA_OS
	return dpi / kReferenceDpi;
#else
	// На Aurora плотность считается от базовой 160 dpi (как на Android):
	// MDPI = 1.0, HDPI = 1.5 и т.д.
	return dpi / 160.0F;
#endif
}

} // namespace App
