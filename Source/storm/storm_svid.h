#pragma once

namespace devilution {

bool SVidPlayBegin(const char *filename, int flags);
bool SVidPlayContinue();
void SVidPlayEnd();
void SVidMute();
void SVidUnmute();

#ifdef AURORA_OS
/// Сдвинуть внутренние часы кадров на deltaMicros: pacing видео идёт от
/// настенных часов (SDL_GetTicks), и после паузы свёрнутого состояния
/// без сдвига декод молниеносно догоняет реальное время — видео «не
/// стояло на паузе». Сдвиг на длительность паузы замораживает и время.
void SVidShiftFrameClock(double deltaMicros);
#endif

} // namespace devilution
