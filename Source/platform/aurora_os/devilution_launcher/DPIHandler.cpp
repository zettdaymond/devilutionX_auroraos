#include "DPIHandler.hpp"

#include <SDL2/SDL.h>
#include <imgui.h>

namespace App {

float DPIHandler::get_scale()
{
    constexpr int display_index{0};
    const float default_dpi{96.0F};
    float dpi{default_dpi};

    SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

    return dpi / default_dpi;
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
