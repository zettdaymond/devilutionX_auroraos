#pragma once

#ifdef AURORA_OS

struct SDL_Window;

namespace devilution {

/// Нативная обложка плитки Lipstick: отдельная wayland-поверхность с
/// категорией cover, связанная с главным окном приложения.
///
/// Qt-приложения Авроры/Sailfish показывают в плитке не последний буфер
/// главного окна, а специальное окно обложки. Механизм (реализован в
/// Silica-плагине домашнего экрана, композитор только ретранслирует):
///   поверхность обложки: WINID=<id>, CATEGORY="cover", TRANSPARENT=false;
///   главное окно: SAILFISH_HAVE_COVER=true,
///                 SAILFISH_COVER_WINDOW="__winref:<id>";
///   shell-роль поверхности обложки — transient (к самой себе), НЕ
///   toplevel: иначе окно поднимается в стек обычных окон.
/// Свойства доставляются через qt_surface_extension (протокол QtWayland,
/// update_generic_property), значения сериализуются как QVariant.
///
/// Метод рефенса: auroraos-rs/aurora-gui + форк lmaxyz/winit (rm_maliit):
/// рабочая связка на Aurora 5.2 — set_transient(own_surface, 0, 0, 0).
class NativeCover {
public:
	/// Создать поверхность обложки width x height с кадром rgb24 (stride —
	/// байт на строку), выставить свойства и связать с главным окном.
	/// false — qt_surface_extension недоступен или протокол не собрался:
	/// вызывающий живёт по-старой схеме (запечённый кадр в буфере окна).
	/// Повторные вызовы игнорируются.
	[[nodiscard]] static bool CreateAndLink(
	    SDL_Window *mainWindow, int width, int height,
	    const unsigned char *rgb24, int strideBytes);

	/// Обновить кадр обложки (прогресс загрузки в плитке); источник
	/// масштабируется под текущий размер окна обложки. true — активна.
	[[nodiscard]] static bool UpdateFrame(
	    const unsigned char *rgb24, int strideBytes, int srcWidth, int srcHeight);
};

} // namespace devilution

#endif
