#pragma once

// Все отладочные ручки проекта в одном месте. Режим задаёт CMake-флаг
// DEVILUTIONX_DEBUG_ENV_GATES (compile-Definition DEVILUTIONX_DEBUG_ENV_GATES):
//   ON  (Debug-сборки, или явный флаг даже для релиза) — значения читаются
//       из переменных окружения один раз и кэшируются;
//   OFF (релиз без флага) — constexpr-константы: проверки выпиливаются
//       компилятором целиком, в бинарник ни getenv, ни веток не попадает.
// Ручка пользователя или поведения продукта здесь НЕ живёт: ей место
// в конфиг-сервисе (launcher.ini). Это только диагностика разработчика.

#include <SDL2/SDL_stdinc.h>

namespace launcher {

#if DEVILUTIONX_DEBUG_ENV_GATES

class DebugConfig {
public:
	/// DEVILUTIONX_NATIVE_COVER_DEBUG=1 — кардиограмма обложки: перезалив
	/// кадра каждые 500 мс сплошным красным (живы ли коммиты).
	[[nodiscard]] static bool NativeCoverDebug()
	{
		static const bool value = EnvOn("DEVILUTIONX_NATIVE_COVER_DEBUG");
		return value;
	}

	/// DEVILUTIONX_NATIVE_COVER_PROBE=1 — STATUSBAR_VISIBLE=true на главное
	/// окно: зонд живости канала свойств.
	[[nodiscard]] static bool StatusBarProbe()
	{
		static const bool value = EnvOn("DEVILUTIONX_NATIVE_COVER_PROBE");
		return value;
	}

	/// DEVILUTIONX_COVER_POKE=1 — «пнуть» композитор из цикла обложки
	/// (re-transient + кадр другого размера): на 5.1 ответ — тишина.
	[[nodiscard]] static bool CoverPoke()
	{
		static const bool value = EnvOn("DEVILUTIONX_COVER_POKE");
		return value;
	}

	/// DEVILUTIONX_COVER_DUMP=1 — BMP-дампы карточек рядом с настройками.
	[[nodiscard]] static bool CoverDump()
	{
		static const bool value = EnvOn("DEVILUTIONX_COVER_DUMP");
		return value;
	}

	/// DEVILUTIONX_NATIVE_COVER=0 — выключить нативную обложку: плитка
	/// показывает последний буфер главного окна (A/B-сравнение).
	[[nodiscard]] static bool NativeCoverEnabled()
	{
		static const bool value = EnvNotOff("DEVILUTIONX_NATIVE_COVER");
		return value;
	}

	/// DEVILUTIONX_COVER_WINDOWED=0 — вернуть fullscreen-роль лаунчера
	/// (оконный режим — рабочая норма, см. main.cpp).
	[[nodiscard]] static bool CoverWindowed()
	{
		static const bool value = EnvNotOff("DEVILUTIONX_COVER_WINDOWED");
		return value;
	}

	/// DEVILUTIONX_COVER_SEQ=a|b|c — порядок установки роли/свойств/мапа
	/// при линковке обложки.
	[[nodiscard]] static char CoverSequence()
	{
		static const char value = [] {
			const char *env = SDL_getenv("DEVILUTIONX_COVER_SEQ");
			return (env != nullptr && env[0] >= 'a' && env[0] <= 'c') ? env[0] : 'a';
		}();
		return value;
	}

	/// DEVILUTIONX_DUMP_FRAME — номера кадров десктопного дампа через
	/// запятую ("30,150,270"); nullptr — дамп выключен.
	[[nodiscard]] static const char *DumpFrameSpec()
	{
		return SDL_getenv("DEVILUTIONX_DUMP_FRAME");
	}

	/// DEVILUTIONX_DUMP_PATH — куда писать BMP десктопного дампа
	/// («{}» подставляет номер кадра); nullptr — путь по умолчанию.
	[[nodiscard]] static const char *DumpFramePath()
	{
		return SDL_getenv("DEVILUTIONX_DUMP_PATH");
	}

private:
	static bool EnvOn(const char *name)
	{
		const char *value = SDL_getenv(name);
		return value != nullptr && value[0] == '1';
	}

	static bool EnvNotOff(const char *name)
	{
		const char *value = SDL_getenv(name);
		return value == nullptr || value[0] != '0';
	}
};

#else

class DebugConfig {
public:
	[[nodiscard]] static constexpr bool NativeCoverDebug() { return false; }
	[[nodiscard]] static constexpr bool StatusBarProbe() { return false; }
	[[nodiscard]] static constexpr bool CoverPoke() { return false; }
	[[nodiscard]] static constexpr bool CoverDump() { return false; }
	[[nodiscard]] static constexpr bool NativeCoverEnabled() { return true; }
	[[nodiscard]] static constexpr bool CoverWindowed() { return true; }
	[[nodiscard]] static constexpr char CoverSequence() { return 'a'; }
	[[nodiscard]] static constexpr const char *DumpFrameSpec() { return nullptr; }
	[[nodiscard]] static constexpr const char *DumpFramePath() { return nullptr; }
};

#endif

} // namespace launcher
