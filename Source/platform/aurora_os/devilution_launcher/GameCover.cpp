// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "GameCover.hpp"

#include "AuroraStateWatch.hpp"
#include "core/CoverMachine.hpp"

#include "../ComposerAdapter.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <vector>

namespace launcher::aurora {

namespace {

struct CoverState {
	std::unique_ptr<StateWatch> watch; ///< второй экземпляр на время движка
	CoverMachine machine;              ///< событийная стейтмашина кадра
	std::vector<unsigned char> pixels; ///< запечённый кадр лаунчера, RGB24
	int width = 0;
	int height = 0;
	SDL_Texture *texture = nullptr; ///< ленивая; живёт, пока жив рендерер движка
	SDL_Window *window = nullptr;   ///< окно движка — для buffer transform
	bool portraitRotated = false;   ///< режим порта: transform 270 + ротатор
	bool coverActive = false;       ///< обложка сейчас в буфере (transform NORMAL)
	bool inputFocused = true;       ///< SDL-фокус окна (антидребезг ниже)
	bool focusLostDispatched = true; ///< FocusLost уже отправлен машине
	bool wasDisplayOn = true;       ///< детект смены состояния дисплея
	std::chrono::steady_clock::time_point focusLostAt {};
};

CoverState &Cover()
{
	static CoverState state;
	return state;
}

/// В плитке буфер окна показывается «как есть» — портретная карточка,
/// как у лаунчера; в игре — ландшафтный режим порта (transform 270,
/// контент предращает ротатор). NORMAL соответствует всем ориентациям
/// кроме LANDSCAPE/LANDSCAPE_FLIPPED (см. WaylandComposerAdapter).
void SetTileOrientation(SDL_Window *window, bool tile)
{
	devilution::WaylandComposerAdapter::SetWindowOrientation(
	    window, tile ? SDL_ORIENTATION_PORTRAIT : SDL_ORIENTATION_LANDSCAPE_FLIPPED);
}

/// Границы фокуса → события машины. Антидребезг 100 мс — единственная
/// «временная» штука, и это фильтр шума (шторки/диалоги мигают фокусом),
/// а не решение: машина получает по одному событию на смену.
void FeedFocus(CoverState &s)
{
	if (s.window == nullptr) {
		return;
	}
	const Uint32 flags = SDL_GetWindowFlags(s.window);
	const bool focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0
	    && (flags & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN)) == 0;
	const auto now = std::chrono::steady_clock::now();
	if (focused) {
		if (!s.inputFocused) {
			s.inputFocused = true;
			s.watch->RefreshDisplay();
			s.machine.Handle(CoverEvent::FocusGained);
		}
		return;
	}
	if (s.inputFocused) {
		s.inputFocused = false;
		s.focusLostAt = now;
		s.focusLostDispatched = false;
		s.watch->RefreshDisplay();
		return;
	}
	if (!s.focusLostDispatched && now - s.focusLostAt >= std::chrono::milliseconds(100)) {
		s.focusLostDispatched = true;
		s.machine.Handle(CoverEvent::FocusLost);
	}
}

/// Границы состояния дисплея → события машины.
void FeedDisplay(CoverState &s)
{
	const bool on = s.watch->DisplayOn();
	if (on == s.wasDisplayOn) {
		return;
	}
	s.wasDisplayOn = on;
	s.machine.Handle(on ? CoverEvent::DisplayOn : CoverEvent::DisplayOff);
}

/// (Пере)создать текстуру под текущий рендерер; false — обложки больше нет.
bool EnsureTexture(SDL_Renderer *renderer)
{
	CoverState &s = Cover();
	if (s.texture != nullptr) {
		return true;
	}
	if (s.pixels.empty()) {
		return false;
	}
	s.texture = SDL_CreateTexture(
	    renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, s.width, s.height);
	if (s.texture == nullptr) {
		// Не ретраим каждый кадр: гасим обложку до следующего запуска.
		spdlog::warn("aurora: SDL_CreateTexture(обложка) не удалась: {}", SDL_GetError());
		s.pixels.clear();
		return false;
	}
	SDL_UpdateTexture(s.texture, nullptr, s.pixels.data(), s.width * 3);
	SDL_SetTextureScaleMode(s.texture, SDL_ScaleModeLinear);
	return true;
}

/// Полный кадр обложки: один блит на весь вывод. В таком кадре нет сырого
/// GL (ротатор не запускался) — только вызовы SDL_Renderer.
void DrawCover(SDL_Renderer *renderer)
{
	CoverState &s = Cover();
	SDL_SetRenderDrawColor(renderer, 10, 7, 5, 255);
	SDL_RenderClear(renderer);
	if (SDL_RenderCopy(renderer, s.texture, nullptr, nullptr) < 0) {
		spdlog::warn("aurora: SDL_RenderCopy(обложка) не удался: {}", SDL_GetError());
	}
	SDL_RenderPresent(renderer);
}

} // namespace

void GameCover::CaptureFromBackbuffer(SDL_Renderer *renderer)
{
	CoverState &s = Cover();
	int width = 0;
	int height = 0;
	if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0 || width <= 0 || height <= 0) {
		spdlog::warn("aurora: SDL_GetRendererOutputSize не удался: {}", SDL_GetError());
		return;
	}
	std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
	const SDL_Rect full { 0, 0, width, height };
	if (SDL_RenderReadPixels(renderer, &full, SDL_PIXELFORMAT_RGB24, pixels.data(), width * 3) != 0) {
		spdlog::warn("aurora: SDL_RenderReadPixels(обложка) не удался: {}", SDL_GetError());
		return;
	}
	s.pixels = std::move(pixels);
	s.width = width;
	s.height = height;
	spdlog::info("aurora: обложка для фазы движка запечена ({}x{})", width, height);
}

void GameCover::Init(SDL_Window *window, bool portraitRotated)
{
	CoverState &s = Cover();
	s.window = window;
	s.portraitRotated = portraitRotated;
	if (s.watch == nullptr) {
		s.watch = std::make_unique<StateWatch>();
		s.machine.Reset();
		s.coverActive = false;
		s.inputFocused = true;
		s.focusLostDispatched = true;
		s.wasDisplayOn = true;
		spdlog::info("aurora: GameCover Init (фаза движка, portraitRotated={})", portraitRotated);
	}
}

void GameCover::Shutdown()
{
	CoverState &s = Cover();
	if (s.texture != nullptr) {
		SDL_DestroyTexture(s.texture);
		s.texture = nullptr;
	}
	s.watch.reset();
	spdlog::info("aurora: GameCover Shutdown");
}

void GameCover::ResetTexture()
{
	if (Cover().texture != nullptr) {
		SDL_DestroyTexture(Cover().texture);
		Cover().texture = nullptr;
	}
}

bool GameCover::BeginCoverFrame(SDL_Renderer *renderer)
{
	CoverState &s = Cover();
	if (renderer == nullptr || s.watch == nullptr || s.pixels.empty()) {
		return false;
	}

	// Входы машины — только то, что работает под песочницей иконочного
	// запуска: SDL-фокус окна и состояние дисплея. Topmost/tklock
	// композитора dbus-прокси не пропускает и в решениях не участвуют.
	// NextAction() отдаёт одноразовые действия переходов (кадр-замена
	// карточки при гашении) ровно одному кадру.
	FeedFocus(s);
	FeedDisplay(s);

	switch (s.machine.NextAction()) {
	case CoverAction::RenderNothing:
		// Экран погашен: кадр не нужен вовсе.
		return true;
	case CoverAction::RenderCover:
		if (!EnsureTexture(renderer)) {
			return false;
		}
		if (!s.coverActive) {
			s.coverActive = true;
			if (s.portraitRotated && s.window != nullptr) {
				SetTileOrientation(s.window, true);
			}
		}
		DrawCover(renderer);
		return true;
	case CoverAction::RenderGame:
		if (s.coverActive) {
			s.coverActive = false;
			if (s.portraitRotated && s.window != nullptr) {
				SetTileOrientation(s.window, false);
			}
		}
		return false;
	}
	return false;
}

bool GameCover::IsHidden()
{
	return Cover().machine.Action() != CoverAction::RenderGame;
}

} // namespace launcher::aurora

#endif
