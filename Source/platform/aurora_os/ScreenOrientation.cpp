#include "ScreenOrientation.hpp"

#include <SDL2/SDL.h>

#include "ComposerAdapter.hpp"

namespace devilution
{

namespace
{

// Все обращения — из главного потока: событие приходит через SDL-насос,
// ротатор рисует в RenderPresent, обложка — тоже. Атомики не нужны.
AuroraLandscape g_landscape = AuroraLandscape::Primary;
bool g_nativePortrait = true;

} // namespace

void AuroraSetNativePortrait( bool nativePortrait )
{
    g_nativePortrait = nativePortrait;
}

float AuroraRotatorAngleDegrees()
{
    return g_landscape == AuroraLandscape::Primary ? 90.0f : 270.0f;
}

float AuroraTouchAngleDegrees()
{
    // Палец и ротатор поворачиваются в одну сторону.
    return AuroraRotatorAngleDegrees();
}

float AuroraMouseAngleDegrees()
{
    // Мышь считается в пикселях с осью Y вниз — знак обратный ротатору.
    return g_landscape == AuroraLandscape::Primary ? -90.0f : 90.0f;
}

bool AuroraApplyOrientation( SDL_DisplayOrientation orientation )
{
    // Семантика события ЗАВИСИТ от панели (см. маппинг wl_output в
    // SDL_waylandvideo.c): на портретной панели телефона LANDSCAPE —
    // transform 90, и выверенная на устройстве пара контринтуитивна
    // (LANDSCAPE→Flipped). На ландшафтной панели планшета LANDSCAPE —
    // натуральная ориентация: LANDSCAPE→Primary, LANDSCAPE_FLIPPED→
    // Flipped(180°). Одна пара для обеих панелей ставила окно планшета
    // вверх ногами: композитор считал приложение инвертированным —
    // жест сворачивания срабатывал с верхнего края, а шторка Авроры
    // показывалась перевёрнутой.
    AuroraLandscape next = g_landscape;
    if( g_nativePortrait ) {
        if( orientation == SDL_ORIENTATION_LANDSCAPE )
            next = AuroraLandscape::Flipped;
        else if( orientation == SDL_ORIENTATION_LANDSCAPE_FLIPPED )
            next = AuroraLandscape::Primary;
    } else {
        if( orientation == SDL_ORIENTATION_LANDSCAPE )
            next = AuroraLandscape::Primary;
        else if( orientation == SDL_ORIENTATION_LANDSCAPE_FLIPPED )
            next = AuroraLandscape::Flipped;
    }

    if( next == g_landscape )
        return false;

    g_landscape = next;
    return true;
}

void AuroraApplyWindowTransform( SDL_Window* window )
{
    if( window == nullptr )
        return;

    if( g_nativePortrait )
    {
        // Телефон: Primary = transform 270 (исторический режим порта),
        // Flipped = 90. См. WaylandComposerAdapter за маппингом enum → wl.
        WaylandComposerAdapter::SetWindowOrientation(
            window,
            g_landscape == AuroraLandscape::Primary ? SDL_ORIENTATION_LANDSCAPE_FLIPPED
                                                    : SDL_ORIENTATION_LANDSCAPE );
    }
    else
    {
        // Планшет: панель ландшафтная, переворот — на 180°. Пара
        // PORTRAIT/PORTRAIT_FLIPPED выбрана ради маппинга NORMAL/180.
        WaylandComposerAdapter::SetWindowOrientation(
            window,
            g_landscape == AuroraLandscape::Primary ? SDL_ORIENTATION_PORTRAIT
                                                    : SDL_ORIENTATION_PORTRAIT_FLIPPED );
    }
}

} // namespace devilution
