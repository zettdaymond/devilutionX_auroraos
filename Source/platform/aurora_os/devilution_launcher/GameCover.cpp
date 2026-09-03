// Компилируется только в сборке под Aurora OS (см. CMakeLists).

#ifdef AURORA_OS

#include "GameCover.hpp"

#include "AuroraStateWatch.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

namespace launcher::aurora {

namespace {

/// Кросс-фейд на входе в плитку — той же длины, что у лаунчера.
constexpr auto kCoverFadeDuration = std::chrono::milliseconds(300);

struct CoverState {
	std::unique_ptr<StateWatch> watch; ///< второй экземпляр на время движка
	std::vector<unsigned char> pixels; ///< запечённый кадр лаунчера, RGB24
	int width = 0;
	int height = 0;
	SDL_Texture *texture = nullptr; ///< ленивая; живёт, пока жив рендерер движка
	std::optional<std::chrono::steady_clock::time_point> fadeStartedAt;
	bool wasTiled = false;
};

CoverState &Cover()
{
	static CoverState state;
	return state;
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

/// Полный кадр обложки: один блит на весь вывод.
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

void GameCover::Init()
{
	CoverState &s = Cover();
	if (s.watch == nullptr) {
		s.watch = std::make_unique<StateWatch>();
		s.wasTiled = false;
		s.fadeStartedAt.reset();
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
	const bool screenVisible = s.watch->DisplayOn() && !s.watch->TkLocked();
	if (tiled && !s.wasTiled && screenVisible) {
		// Фейд живёт только на переходе «видимое → плитка»: будить его
		// повторно (просыпание уже в плитке) нельзя — иначе под обложкой
		// снова мелькал бы интерфейс.
		s.fadeStartedAt = std::chrono::steady_clock::now();
	}
	s.wasTiled = tiled;

	if (s.fadeStartedAt.has_value()
	    && std::chrono::steady_clock::now() - *s.fadeStartedAt >= kCoverFadeDuration) {
		s.fadeStartedAt.reset();
	}

	if (!tiled) {
		s.fadeStartedAt.reset();
		return false;
	}
	if (!screenVisible) {
		// Плитка на погашенном/заблокированном экране: рисовать не для
		// кого, но и игровой кадр в тёмную матрицу гнать незачем.
		return true;
	}
	if (s.fadeStartedAt.has_value()) {
		// Идёт кросс-фейд: игровой кадр рисуется как обычно, обложка
		// подмешается поверх в OverlayCoverFade.
		return false;
	}
	if (!EnsureTexture(renderer)) {
		return false;
	}
	DrawCover(renderer);
	return true;
}

void GameCover::OverlayCoverFade(SDL_Renderer *renderer)
{
	CoverState &s = Cover();
	if (renderer == nullptr || !s.fadeStartedAt.has_value()) {
		return;
	}
	const auto elapsed = std::chrono::steady_clock::now() - *s.fadeStartedAt;
	if (elapsed >= kCoverFadeDuration) {
		s.fadeStartedAt.reset();
		return;
	}
	if (!EnsureTexture(renderer)) {
		s.fadeStartedAt.reset();
		return;
	}
	const float progress = std::clamp(
	    std::chrono::duration<float>(elapsed).count()
	        / std::chrono::duration<float>(kCoverFadeDuration).count(),
	    0.0F, 1.0F);
	// smoothstep: линейный фейд воспринимается резким вначале.
	const float alpha = progress * progress * (3.0F - 2.0F * progress);
	SDL_SetTextureBlendMode(s.texture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(s.texture, static_cast<unsigned char>(alpha * 255.0F));
	if (SDL_RenderCopy(renderer, s.texture, nullptr, nullptr) < 0) {
		spdlog::warn("aurora: SDL_RenderCopy(фейд обложки) не удался: {}", SDL_GetError());
	}
}

} // namespace launcher::aurora

#endif
