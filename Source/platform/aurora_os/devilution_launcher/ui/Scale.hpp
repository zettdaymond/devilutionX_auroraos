#pragma once

namespace launcher::ui {

/// Максимальная ширина контентного столбца в ландшафте: на широких
/// экранах строки и кнопки не растягиваются, столбец центрируется.
inline constexpr float kLandscapeContentMaxRem = 30.0F;

/**
 * @brief Жидкое масштабирование интерфейса по размеру вьюпорта.
 *
 * Все размеры UI выражены в «rem» — как в современных веб-тулкитах, —
 * поэтому интерфейс плавно масштабируется между телефоном, планшетом и
 * окном на десктопе: фиксированные брейкпоинты есть только у структуры
 * layout, но не у размеров.
 *
 * rem = clamp(диагональ вьюпорта / 44, 12·dpi, 26·dpi) пикселей.
 */
class Scale {
public:
	/// Пересчитывает значения под текущий кадр. Вызывается раз в кадр до рендера.
	static void BeginFrame(float dpiScale);

	/// Текущий rem в пикселях.
	[[nodiscard]] static auto Rem() -> float;

	/// Переводит rem-ы в пиксели.
	[[nodiscard]] static auto Px(float remUnits) -> float { return remUnits * Rem(); }

	/// Портретная (телефон) или альбомная (планшет/десктоп) компоновка.
	[[nodiscard]] static auto Portrait() -> bool;

	/// Меньшая сторона вьюпорта в пикселях.
	[[nodiscard]] static auto MinSide() -> float;

	/// Размер ФИЗИЧЕСКОГО экрана (SDL_GetDesktopDisplayMode, ландшафт).
	/// Вьюпорт окна на десктопе меньше экрана, а решающим для лестницы
	/// разрешений и подобных «экранных» вещей является именно экран —
	/// как SDL_GetDesktopDisplayMode у движка.
	static void SetScreenSize(int landscapeWidth, int landscapeHeight);

	/// Меньшая сторона экрана; до SetScreenSize — вьюпорта.
	[[nodiscard]] static auto ScreenMinSide() -> float;
};

} // namespace launcher::ui
