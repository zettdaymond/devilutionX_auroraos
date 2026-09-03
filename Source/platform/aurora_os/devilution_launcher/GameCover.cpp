// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "GameCover.hpp"

#include "AuroraStateWatch.hpp"

#include "../ComposerAdapter.hpp"

#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

namespace launcher::aurora {

namespace {

struct CoverState {
	std::unique_ptr<StateWatch> watch; ///< второй экземпляр на время движка
	std::vector<unsigned char> pixels; ///< запечённый кадр лаунчера, RGB24
	int width = 0;
	int height = 0;
	SDL_Texture *texture = nullptr; ///< ленивая; живёт, пока жив рендерер движка
	SDL_Window *window = nullptr;   ///< окно движка — для buffer transform
	bool portraitRotated = false;   ///< режим порта: transform 270 + ротатор
	bool seenTopmost = false;        ///< был ли хоть один видимый кадр движка
	bool coverActive = false;        ///< для диагностического лога переходов
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
		s.seenTopmost = false;
		s.coverActive = false;
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
	const bool tiled = !s.watch->TopmostOurs();
	if (!tiled) {
		s.seenTopmost = true;
		if (s.coverActive) {
			s.coverActive = false;
			// Вернуть ландшафтный transform ДО первого игрового кадра:
			// ротатор рисует контент предращённым, и без 270 он показался
			// бы повёрнутым.
			if (s.portraitRotated && s.window != nullptr) {
				SetTileOrientation(s.window, false);
			}
			spdlog::info("aurora: обложка движка выключена (вернулись в передний план)");
		}
		return false;
	}
	// На старте окно движка полсекунды не является верхним, пока композитор
	// его поднимает: в этом зале обложкой кадр не закрываем — пользователь
	// ждёт первое меню, а не брендинг.
	if (!s.seenTopmost) {
		return false;
	}
	// Плитка на погашенном/заблокированном экране: рисовать не для кого,
	// но и игровой кадр в тёмную матрицу гнать незачем.
	if (!s.watch->DisplayOn() || s.watch->TkLocked()) {
		return true;
	}
	if (!EnsureTexture(renderer)) {
		return false;
	}
	if (!s.coverActive) {
		s.coverActive = true;
		// Плитка показывает буфер «как есть»: портретная обложка без
		// поворота требует NORMAL (до этого на окне висит 270 режима
		// порта — обложка показывалась бы повёрнутой на 90°).
		if (s.portraitRotated && s.window != nullptr) {
			SetTileOrientation(s.window, true);
		}
		spdlog::info("aurora: обложка движка включена (свернулись в плитку)");
	}
	DrawCover(renderer);
	return true;
}

} // namespace launcher::aurora

#endif
