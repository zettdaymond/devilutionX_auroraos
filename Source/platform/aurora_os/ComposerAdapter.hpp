#pragma once

#include <SDL2/SDL.h>

namespace devilution
{
class WaylandComposerAdapter
{
public:
    static void SetWindowOrientation( SDL_Window* window, SDL_DisplayOrientation orientation );
};
} // namespace devilution
