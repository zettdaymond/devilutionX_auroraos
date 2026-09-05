// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "GameCover.hpp"

#include "AuroraStateWatch.hpp"

#include <spdlog/spdlog.h>

#include <SDL2/SDL.h>

#include <memory>

// Пауза аудиоустройства SDL_audiolib в свёрнутом состоянии: колбэк
// устройства продолжал микшировать заглушённую музыку (~10% CPU в
// плитке из геймплея). Публичного API у библиотеки нет, id устройства —
// приватный статик (src/stream_p.h); частичное объявление класса ради
// статика резолвится в тот же символ pinned-версии зависимости.
// ОБЯЗАТЕЛЬНО на файловом уровне: внутри анонимного namespace символ
// получал внутреннюю линковку и не находился линкером.
namespace Aulib {
struct Stream_priv {
	static SDL_AudioDeviceID fDeviceId;
};
}

namespace launcher::aurora {

namespace {

void PauseAudioDevice(bool pause)
{
	if (Aulib::Stream_priv::fDeviceId != 0) {
		SDL_PauseAudioDevice(Aulib::Stream_priv::fDeviceId, pause ? SDL_TRUE : SDL_FALSE);
		spdlog::info("aurora: аудиоустройство {}", pause ? "на паузе (плитка)" : "возобновлено");
	}
}

struct CoverState {
	std::unique_ptr<StateWatch> watch; ///< второй экземпляр на время движка
	SDL_Window *window = nullptr;      ///< окно движка — флаги видимости
	bool audioPaused = false;          ///< край скрытости: аудио на паузе
};

CoverState &Cover()
{
	static CoverState state;
	return state;
}

} // namespace

void GameCover::Init(SDL_Window *window)
{
	CoverState &s = Cover();
	s.window = window;
	s.audioPaused = false;
	if (s.watch == nullptr) {
		s.watch = std::make_unique<StateWatch>();
		spdlog::info("aurora: GameCover Init (фаза движка)");
	}
}

void GameCover::Shutdown()
{
	CoverState &s = Cover();
	s.watch.reset();
	spdlog::info("aurora: GameCover Shutdown");
}

bool GameCover::IsHidden()
{
	CoverState &s = Cover();
	if (s.watch == nullptr) {
		return false;
	}
	bool hidden = !s.watch->DisplayOn();
	if (s.window != nullptr) {
		const Uint32 flags = SDL_GetWindowFlags(s.window);
		hidden = hidden
		    || (flags & SDL_WINDOW_INPUT_FOCUS) == 0
		    || (flags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) != 0;
	}
	if (hidden != s.audioPaused) {
		s.audioPaused = hidden;
		PauseAudioDevice(hidden);
	}
	return hidden;
}

} // namespace launcher::aurora

#endif
