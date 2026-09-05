/**
 * @file movie.cpp
 *
 * Implementation of video playback.
 */

#include <cstdint>

#ifdef AURORA_OS
#include <chrono>
#endif

#include "controls/plrctrls.h"
#include "diablo.h"
#include "effects.h"
#include "engine/backbuffer_state.hpp"
#include "engine/demomode.h"
#include "engine/events.hpp"
#include "engine/sound.h"
#include "hwcursor.hpp"
#include "storm/storm_svid.h"
#include "utils/display.h"

#ifdef AURORA_OS
#include "GameCover.hpp"
#include "engine/dx.h"
#endif

namespace devilution {

/** Should the movie continue playing. */
bool movie_playing;
/** Should the movie play in a loop. */
bool loop_movie;

void play_movie(const char *pszMovie, bool userCanClose)
{
	if (demo::IsRunning())
		return;

	movie_playing = true;

	sound_disable_music(true);
	stream_stop();

	if (IsHardwareCursorEnabled() && ControlDevice == ControlTypes::KeyboardAndMouse) {
		SetHardwareCursorVisible(false);
	}

	if (SVidPlayBegin(pszMovie, loop_movie ? 0x100C0808 : 0x10280808)) {
		SDL_Event event;
		uint16_t modState;
#ifdef AURORA_OS
		bool videoPaused = false;
		std::chrono::steady_clock::time_point videoPauseStartedAt {};
#endif
		while (movie_playing) {
			while (movie_playing && FetchMessage(&event, &modState)) {
				if (userCanClose) {
					for (ControllerButtonEvent ctrlEvent : ToControllerButtonEvents(event)) {
						if (!SkipsMovie(ctrlEvent))
							continue;
						movie_playing = false;
						break;
					}
				}
				switch (event.type) {
				case SDL_KEYDOWN:
				case SDL_MOUSEBUTTONUP:
					if (userCanClose || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
						movie_playing = false;
					break;
#ifndef USE_SDL1
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
						diablo_focus_pause();
					else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
						diablo_focus_unpause();
					break;
#else
				case SDL_ACTIVEEVENT:
					if ((event.active.state & SDL_APPINPUTFOCUS) != 0) {
						if (event.active.gain == 0)
							diablo_focus_pause();
						else
							diablo_focus_unpause();
					}
					break;
#endif
				case SDL_QUIT:
					SVidPlayEnd();
					diablo_quit(0);
				}
			}
#ifdef AURORA_OS
			// Плитка/гашение экрана: честная пауза видео — декод стоит
			// (звук глушится diablo_focus_pause по потере фокуса).
			// Pacing видео — от настенных часов (SDL_GetTicks): при снятии
			// с паузы сдвигаем базу часов на её длительность, иначе декод
			// молниеносно догоняет реальное время и видео «не стояло на
			// паузе». Спим до события (смена фокуса/дисплея всегда
			// приходит событием); проснутое событие возвращаем в очередь —
			// его обработает насос наверху цикла.
			if (launcher::aurora::GameCover::IsHidden()) {
				if (!videoPaused) {
					videoPaused = true;
					videoPauseStartedAt = std::chrono::steady_clock::now();
					SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "aurora: видео на паузе");
				}
				SDL_Event wait {};
				if (SDL_WaitEvent(&wait) == 1) {
					SDL_PushEvent(&wait);
				} else {
					SDL_Delay(100);
				}
				continue;
			}
			if (videoPaused) {
				videoPaused = false;
				const auto pausedMicros = std::chrono::duration_cast<std::chrono::microseconds>(
				    std::chrono::steady_clock::now() - videoPauseStartedAt);
				SVidShiftFrameClock(static_cast<double>(pausedMicros.count()));
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "aurora: видео снято с паузы, часы сдвинуты на %.0f мс",
				    static_cast<double>(pausedMicros.count()) / 1000.0);
			}
#endif
			if (!SVidPlayContinue())
				break;
		}
		SVidPlayEnd();
	}

	sound_disable_music(false);

	movie_playing = false;

	SDL_GetMouseState(&MousePosition.x, &MousePosition.y);
	OutputToLogical(&MousePosition.x, &MousePosition.y);
	InitBackbufferState();
}

void PlayInGameMovie(const char *pszMovie)
{
	PaletteFadeOut(8);
	play_movie(pszMovie, false);
	ClearScreenBuffer();
	RedrawEverything();
	scrollrt_draw_game_screen();
	PaletteFadeIn(8);
	RedrawEverything();
}

} // namespace devilution
