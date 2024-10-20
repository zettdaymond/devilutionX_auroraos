#include "DPIHandler.hpp"

#include <SDL2/SDL.h>
#include <imgui.h>

namespace App {

float DPIHandler::get_scale()
{
    // Для SDL
    constexpr int display_index{0};
    const float default_dpi{96.0F};
    float dpi{default_dpi};

    SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

#ifndef AURORA_OS
    return dpi / default_dpi;

// Для Android
#else
    // Преобразование плотности Android в масштабный коэффициент
    // MDPI (160 dpi) = 1.0, HDPI (240 dpi) = 1.5, и т.д.
    return static_cast<float>(dpi) / 160.0f;
#endif
}

WindowSize DPIHandler::get_dpi_aware_window_size(const WindowSize& size)
{
    const float scale{DPIHandler::get_scale()};
    const int width{static_cast<int>(static_cast<float>(size.width) * scale)};
    const int height{static_cast<int>(static_cast<float>(size.height) * scale)};
    return {width, height};
}

void DPIHandler::set_global_font_scaling([[maybe_unused]] ImGuiIO* io)
{
    // do nothing
}

} // namespace App
