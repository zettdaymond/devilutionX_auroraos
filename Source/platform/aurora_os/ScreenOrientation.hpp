#pragma once

#include <cstdint>
#include <SDL2/SDL.h>

namespace devilution
{

/// Сторона ландшафта: каким боком игра лежит на панели телефона.
/// Primary — историческое состояние порта (buffer transform 270),
/// Flipped — телефон перевёрнут на 180° (transform 90).
enum class AuroraLandscape : std::uint8_t
{
    Primary = 0,
    Flipped = 1
};

/// Режим панели: портретная (телефон — ротатор + transform 270/90) или
/// ландшафтная (планшет — transform NORMAL/180 без ротатора).
/// Вызывается при создании окна движка; панель не меняется на лету.
void AuroraSetNativePortrait( bool nativePortrait );

/// Угол (градусы), на который шейдер ротатора поворачивает кадр,
/// укладывая ландшафт в портретный фреймбуфер (90/270).
float AuroraRotatorAngleDegrees();

/// Угол поворота UV пальца: тач-координаты нормализованы (0..1).
float AuroraTouchAngleDegrees();

/// Угол поворота UV мыши: пиксели, ось Y вниз, направление обратное
/// ротатору.
float AuroraMouseAngleDegrees();

/// Задать ландшафт по SDL-ориентации (событие SDL_DISPLAYEVENT_ORIENTATION
/// или начальный запрос). Портретные положения игнорируются — игра
/// ландшафтная. Возвращает true, если ландшафт сменился.
bool AuroraApplyOrientation( SDL_DisplayOrientation orientation );

/// Выставить окну wl buffer transform текущего ландшафта.
void AuroraApplyWindowTransform( SDL_Window* window );

}
