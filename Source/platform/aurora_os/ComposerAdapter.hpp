#pragma once

#include <optional>
#include <SDL2/SDL.h>

#include "Utilities.hpp"

namespace devilution
{
class WaylandComposerAdapter
{
public:
    static void SetWindowOrientation( SDL_Window* window, SDL_DisplayOrientation orientation );
    static std::optional<vec2> GetScreenDpi( SDL_Window* window );
};
} // namespace devilution
