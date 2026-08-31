#pragma once

#include <algorithm>

namespace launcher::ui {

/// Простые easing-функции для чисто презентационных анимаций.
/// Вся динамика живёт в view-слое и не пересекает границу MVI:
/// LauncherState остаётся единственным источником истины.

/// Плавное замедление в конце — стандарт для появления элементов UI.
[[nodiscard]] inline float EaseOutCubic(float t)
{
	t = std::clamp(t, 0.0F, 1.0F);
	const float inv = 1.0F - t;
	return 1.0F - inv * inv * inv;
}

/// Ускоряющееся течение — закрытие «ставней»: медленный старт, решительный
/// финал (iris-out перед уходом в движок).
[[nodiscard]] inline float EaseInQuad(float t)
{
	t = std::clamp(t, 0.0F, 1.0F);
	return t * t;
}

/// Линейное значение из прошедшего времени (секунды).
[[nodiscard]] inline float ElapsedFraction(double t0, double now, float duration)
{
	return duration <= 0.0F ? 1.0F : static_cast<float>((now - t0) / duration);
}

} // namespace launcher::ui
