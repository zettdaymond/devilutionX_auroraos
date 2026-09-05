#pragma once

#ifdef AURORA_OS

#include <SDL2/SDL.h>

namespace launcher::aurora {

/// Видимость окна на время фазы движка. После «Играть» окно приложения
/// переезжает движку без пересоздания; плитку в это время ведёт
/// нативное окно обложки Lipstick (NativeCover), а GameCover отвечает
/// за честные паузы свёрнутого состояния: IsHidden() — синхронный
/// предикат «окно без фокуса/скрыто или дисплей погашен» (второй
/// экземпляр aurora::StateWatch, Init/Shutdown рядом с
/// dx_init/dx_cleanup); topmost/tklock композитора dbus-прокси
/// песочницы не пропускает и в решении не участвуют.
class GameCover final {
public:
	GameCover() = delete;

	/// Запустить/остановить наблюдателя состояния на фазу движка.
	static void Init(SDL_Window *window);
	static void Shutdown();

	/// Правда, если окно сейчас никому не видно (плитка, гашение экрана,
	/// локскрин поверх): меню, видео и игровой рендер честно спят в
	/// SDL_WaitEvent, а не молотят кадры в свёрнутое окно. До Init —
	/// всегда false. Край перехода (скрылось/показалось) переключает
	/// паузу аудиоустройства SDL_audiolib: колбэк продолжал микшировать
	/// заглушённую музыку и в плитке (~10% CPU).
	[[nodiscard]] static bool IsHidden();
};

} // namespace launcher::aurora

#endif
