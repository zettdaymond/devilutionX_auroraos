#pragma once

#ifdef AURORA_OS

#include <SDL2/SDL_stdinc.h>

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
///   shell-роль поверхности обложки — transient к ГЛАВНОМУ окну (как у
///   Qt/Silica), НЕ toplevel: иначе окно поднимается в стек обычных окон.
/// Свойства доставляются через qt_surface_extension (протокол QtWayland,
/// update_generic_property), значения сериализуются как QVariant.
///
/// Референсы: auroraos-rs/aurora-gui (свойства/сериализация) и исходники
/// lipstick (windowIdForLink — линковка по WINID своего процесса).
class NativeCover {
public:
	/// Создать поверхность обложки width x height с кадром rgb24 (stride —
	/// байт на строку), выставить свойства и связать с главным окном.
	/// false — qt_surface_extension недоступен или протокол не собрался:
	/// плитка остаётся на последнем буфере главного окна, карточки нет.
	/// Повторные вызовы игнорируются.
	[[nodiscard]] static bool CreateAndLink(
	    SDL_Window *mainWindow, int width, int height,
	    const unsigned char *rgb24, int strideBytes);

	/// Обновить кадр обложки (прогресс загрузки в плитке); источник
	/// масштабируется под текущий размер окна обложки. true — активна.
	[[nodiscard]] static bool UpdateFrame(
	    const unsigned char *rgb24, int strideBytes, int srcWidth, int srcHeight);

	/// Текущий размер окна обложки (приходит configure'ом от свитчера);
	/// 0x0 — обложка не связана. Кадры стоит рисовать в этом размере:
	/// кроп в UpdateFrame вырождается в тождество, контент не смещается.
	static void Size(int &outWidth, int &outHeight);

	/// Диагностика (DEVILUTIONX_COVER_POKE=1): «пнуть» композитор —
	/// переиздать роль transient и закоммитить кадр другого размера.
	/// На 5.1 configure от свитчера не приходит вовсе; опыт проверяет,
	/// отвечает ли композитор configure'ом хоть на какое-то действие
	/// клиента.
	static void Poke();

	/// Тип пользовательского SDL-события «окно обложки сменило размер»:
	/// свитчер прислал configure. Пушится из слушателя wayland (диспетчер
	/// — сам SDL внутри Poll/WaitEvent, т.е. главный поток); циклу
	/// обложки пора перерисовать карточку под новый аспект плитки.
	[[nodiscard]] static Uint32 ConfigureEventType();
};

} // namespace devilution

#endif
