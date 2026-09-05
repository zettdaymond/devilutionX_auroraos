#pragma once

#ifdef AURORA_OS

#include <SDL2/SDL.h>

#include <vector>

namespace launcher::aurora {

/// Обложка на время фазы движка. Пока работал лаунчер, плитку вело его
/// нативное окно обложки (NativeCover); после «Играть» окно приложения
/// переезжает движку без пересоздания, и от GameCover остаются две
/// задачи:
///
/// - хранить запечённый кадр обложки: лаунчер снимает пиксели с
///   офскрин-таргета своего рендерера (CaptureFromBackbuffer) — буфер
///   обычная память, он переживает смерть рендерера лаунчера и уходит в
///   NativeCover первым кадром;
/// - честно замораживать движок в свёрнутом состоянии: IsHidden() —
///   синхронный предикат «окно без фокуса/скрыто или дисплей погашен»
///   (второй экземпляр aurora::StateWatch, Init/Shutdown рядом с
///   dx_init/dx_cleanup); topmost/tklock композитора dbus-прокси
///   песочницы не пропускает и в решении не участвуют.
class GameCover final {
public:
	GameCover() = delete;

	/// Снять пиксели обложки с текущего таргета рендерера лаунчера
	/// (RGB24). Звать после отрисовки кадра обложки в офскрин-таргет.
	static void CaptureFromBackbuffer(SDL_Renderer *renderer);

	/// Копия запечённых пикселей обложки (RGB24, stride = width*3).
	/// false — обложка ещё не запечена или запеклась пустой.
	[[nodiscard]] static bool CopyBakedPixels(
	    std::vector<unsigned char> &out, int &width, int &height);

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
