#include "InputAdapter.hpp"

#include <functional>
#include <memory>

#include "Utilities.hpp"

#include "ComposerAdapter.hpp"
#include "DisplayBlankerController.hpp"
#include "AuroraOSAudio.hpp"

#include <SDL2/SDL_video.h>

namespace devilution
{

namespace  {

struct WindowSizesProvider
{
    std::function<Uint16()> get_logic_screen_width_func;
    std::function<Uint16()> get_logic_screen_height_func;
    SDL_Window *sdl_window = nullptr;

    auto GetScreenWidth() {
        return std::invoke(get_logic_screen_width_func);
    }

    auto GetScreenHeight() {
        return std::invoke(get_logic_screen_height_func);
    }
};

static SDL_EventFilter rotate_and_fit_filter = [](void *userdata, SDL_Event * event) -> int {

    auto window = static_cast<WindowSizesProvider*>(userdata);
    if(!window) {
        return 1;
    }

    int sourceW, sourceH;
    SDL_GL_GetDrawableSize(window->sdl_window, &sourceW, &sourceH);

    const bool fbNativePortrait = (sourceW < sourceH);

    if(fbNativePortrait && event->type == SDL_MOUSEMOTION) {
        auto newMousePos = ApplyMouseFixes(ivec2( event->motion.x, event->motion.y ), ivec2(sourceW, sourceH), ivec2(window->GetScreenWidth(), window->GetScreenHeight()));
        event->motion.x = newMousePos.x;
        event->motion.y = newMousePos.y;
    }

    if(fbNativePortrait && (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) ) {
        auto newMousePos = ApplyMouseFixes(ivec2(event->button.x, event->button.y ), ivec2(sourceW, sourceH), ivec2(window->GetScreenWidth(), window->GetScreenHeight()));
        event->button.x = newMousePos.x;
        event->button.y = newMousePos.y;
    }

    if(fbNativePortrait && (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION)) {
        vec2 rotatedFingerUV = rotateUV(vec2(event->tfinger.x, event->tfinger.y), degreesToRadians(90.0));
        event->tfinger.x = rotatedFingerUV.x;
        event->tfinger.y = rotatedFingerUV.y;
    }

    if (event->type == SDL_WINDOWEVENT) {

        // Minimizing on Aurora os
        if(event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            DisplayBlankerController::SetPreventDisplayBlanking(false);

            //release audio lock
            if(AudioresourceHolder::audio_resource) {
                AudioresourceHolder::audio_resource = nullptr;
            }
        }

        //Maximizing
        if(event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            DisplayBlankerController::SetPreventDisplayBlanking(true);

            //aquire audio lock
            if(!AudioresourceHolder::audio_resource) {
                AudioresourceHolder::audio_resource = AudioResource::Aquire();
            }
        }
    }

//    if (event->type == SDL_DISPLAYEVENT) {
//        switch (event->display.event) {
//            case SDL_DISPLAYEVENT_ORIENTATION: {
//                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Display orientation changed for display : %i", event->display.display);

//                auto orientation = SDL_GetDisplayOrientation(event->display.display);
//                WaylandComposerAdapter::SetWindowOrientation(window->sdl_window, orientation);
//                break;
//            }
//        }
//    }

    return 1;
};

std::unique_ptr<WindowSizesProvider> window_sizes_provider;

}

void AuroraOSInputAdapter::InstallInputEventsFilter(SDL_Window *window, std::function<Uint16()> get_logic_screen_width_func, std::function<Uint16()> get_logic_screen_height_func)
{
    if(!window) {
        return;
    }

    SDL_SetEventFilter(nullptr, window);

    window_sizes_provider = std::make_unique<WindowSizesProvider>();
    window_sizes_provider->get_logic_screen_width_func = get_logic_screen_width_func;
    window_sizes_provider->get_logic_screen_height_func = get_logic_screen_height_func;
    window_sizes_provider->sdl_window = window;

    SDL_SetEventFilter(rotate_and_fit_filter, window_sizes_provider.get());
}

void AuroraOSInputAdapter::RemoveInputEventsFilter(SDL_Window *window)
{
    SDL_SetEventFilter(nullptr, window);
}

}
