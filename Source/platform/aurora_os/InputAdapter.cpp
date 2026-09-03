#include "InputAdapter.hpp"

#include <functional>
#include <memory>

#include "Utilities.hpp"

#include "ComposerAdapter.hpp"
#include "ScreenOrientation.hpp"
#include "DisplayBlankerController.hpp"
#include "AuroraOSAudio.hpp"
#include "GameCover.hpp"

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
        auto newMousePos = ApplyMouseFixes(ivec2( event->motion.x, event->motion.y ), ivec2(sourceW, sourceH), ivec2(window->GetScreenWidth(), window->GetScreenHeight()), degreesToRadians(AuroraMouseAngleDegrees()));
        event->motion.x = newMousePos.x;
        event->motion.y = newMousePos.y;
    }

    if(fbNativePortrait && (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) ) {
        auto newMousePos = ApplyMouseFixes(ivec2(event->button.x, event->button.y ), ivec2(sourceW, sourceH), ivec2(window->GetScreenWidth(), window->GetScreenHeight()), degreesToRadians(AuroraMouseAngleDegrees()));
        event->button.x = newMousePos.x;
        event->button.y = newMousePos.y;
    }

    if(fbNativePortrait && (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION)) {
        vec2 rotatedFingerUV = rotateUV(vec2(event->tfinger.x, event->tfinger.y), degreesToRadians(AuroraTouchAngleDegrees()));
        event->tfinger.x = rotatedFingerUV.x;
        event->tfinger.y = rotatedFingerUV.y;
    }

    if (event->type == SDL_WINDOWEVENT) {

        // Minimizing on Aurora os
        if(event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "input filter: FOCUS_LOST");
            DisplayBlankerController::SetPreventDisplayBlanking(false);
            ReleaseAudioResource();
        }

        //Maximizing
        if(event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "input filter: FOCUS_GAINED");
            DisplayBlankerController::SetPreventDisplayBlanking(true);

            // Запрос аудиоресурса неблокирующийся: фильтр работает внутри
            // SDL_PollEvent, и зависание в libdbus здесь замораживало
            // первый кадр движка (чёрный экран, сентябрь 2026).
            AcquireAudioResourceAsync();
        }
    }

    // Переворот телефона на 180°: липстик сообщает смену ориентации
    // выхода, SDL доставляет её display-событием — и повторяет текущее
    // значение помногу раз, поэтому реагируем только на смену (edge).
    // Портретные положения игнорируем: игра ландшафтная, окно остаётся
    // в текущем ландшафте, композитор показывает его с полями.
    if (event->type == SDL_DISPLAYEVENT && event->display.event == SDL_DISPLAYEVENT_ORIENTATION) {
        const auto orientation = static_cast<SDL_DisplayOrientation>(event->display.data1);
        if (AuroraApplyOrientation(orientation)) {
            // В плитке transform не трогаем: карточка обложки портретная,
            // GameCover выставит новый transform при возврате к игре.
            if (!launcher::aurora::GameCover::IsHidden()) {
                AuroraApplyWindowTransform(window->sdl_window);
            }
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "aurora-orient: ландшафт сменился (data1=%d)",
                event->display.data1);
        }
    }

    // Липстик повторяет текущую ориентацию выхода помногу раз в секунду
    // (в движковой фазе ~20/с). Ориентация полностью обработана выше,
    // другим потребителям display-события не нужны — в очередь их не
    // пускаем: иначе они будят WaitEvent-паузы свёрнутого приложения
    // (~10% CPU в плитке из меню).
    if (event->type == SDL_DISPLAYEVENT) {
        return 0;
    }

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
