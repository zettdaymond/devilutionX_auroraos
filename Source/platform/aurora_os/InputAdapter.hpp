#pragma once

#include <functional>

#include <SDL2/SDL.h>

namespace devilution {

class AuroraOSInputAdapter
{
public:
    static void InstallInputEventsFilter(SDL_Window* window, std::function<Uint16 ()> get_logic_screen_width_func, std::function<Uint16 ()> get_logic_screen_height_func);
    static void RemoveInputEventsFilter(SDL_Window* window);
};

}
