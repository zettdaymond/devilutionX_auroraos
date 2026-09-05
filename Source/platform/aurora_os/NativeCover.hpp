#pragma once

#ifdef AURORA_OS

struct SDL_Window;

namespace devilution {

/// Нативная обложка плитки Lipstick: отдельное окно с категорией cover.
///
/// Qt-приложения Авроры/Sailfish показывают в плитке не последний буфер
/// главного окна, а специальное окно обложки: композитору через
/// qt_surface_extension (протокол QtWayland, update_generic_property)
/// сообщаются свойства окна. Связка:
///   окно обложки: WINID=<id>, CATEGORY="cover", TRANSPARENT=false;
///   главное окно: SAILFISH_HAVE_COVER=true,
///                 SAILFISH_COVER_WINDOW="__winref:<id>".
/// Дальше Lipstick сам показывает обложку при уходе приложения в фон —
/// без детекта «мы свёрнуты» и переключений buffer transform. Значения
/// свойств сериализуются как QVariant (см. реализацию).
///
/// Метод рефенса: auroraos-rs/aurora-gui (aurora_app/src/window.rs).
class NativeCover {
public:
	/// Создать скрытое окно обложки width x height, залить в него кадр
	/// rgb24 (stride — байт на строку), выставить свойства и связать с
	/// главным окном. false — qt_surface_extension недоступен или окна
	/// не создались: вызывающий живёт по-старому (запечённый кадр в
	/// буфере главного окна). Повторные вызовы игнорируются.
	[[nodiscard]] static bool CreateAndLink(
	    SDL_Window *mainWindow, int width, int height,
	    const unsigned char *rgb24, int strideBytes);

	/// Обновить кадр в окне обложки (прогресс загрузки в плитке).
	/// true — нативная обложка активна.
	[[nodiscard]] static bool UpdateFrame(const unsigned char *rgb24, int strideBytes);
};

} // namespace devilution

#endif
