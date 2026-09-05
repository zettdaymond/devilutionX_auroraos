#include <SDL.h>
#include <SDL_main.h>
#include <array>

#ifdef __SWITCH__
#include "platform/switch/network.h"
#include "platform/switch/random.hpp"
#include "platform/switch/romfs.hpp"
#endif
#ifdef __3DS__
#include "platform/ctr/system.h"
#endif
#ifdef __vita__
#include "platform/vita/network.h"
#include "platform/vita/random.hpp"
#endif
#ifdef NXDK
#include <nxdk/mount.h>
#endif
#ifdef GPERF_HEAP_MAIN
#include <gperftools/heap-profiler.h>
#endif

#ifdef AURORA_OS
#   include "Utilities.hpp"
#   include "StandartPaths.hpp"
#   include "DisplayBlankerController.hpp"
#   include "Application.hpp"
#   include "core/EngineLaunch.hpp"
#   include "appfat.h"
#   include <string>
#   include <vector>
#   define FUNC_EXPORT extern "C" __attribute__((visibility("default"))) int main

namespace devilution {
extern SDL_Window *ghMainWnd;
}

#else
#   define FUNC_EXPORT extern "C" int main
#endif

#include "diablo.h"


#if !defined(__APPLE__)
extern "C" const char *__asan_default_options() // NOLINT(bugprone-reserved-identifier, readability-identifier-naming)
{
	return "halt_on_error=0";
}
#endif

FUNC_EXPORT(int argc, char **argv)
{
#ifdef __SWITCH__
	switch_romfs_init();
	switch_enable_network();
	randombytes_switchrandom_init();
#endif
#ifdef __3DS__
	ctr_sys_init();
#endif
#ifdef __vita__
	vita_enable_network();
	randombytes_vitarandom_init();
#endif
#ifdef NXDK
	nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
#endif
#ifdef GPERF_HEAP_MAIN
	HeapProfilerStart("main");
#endif

#ifdef AURORA_OS
    devilution::DisplayBlankerController::Init();
    devilution::DisplayBlankerController::SetPreventDisplayBlanking(true);

    // Аргументы движка, собранные из выбора пользователя в лаунчере.
    // Должны жить до конца main — argv передаётся в DiabloMain.
    std::vector<std::string> engineArgs;
    std::vector<char *> engineArgv;

    if (SDL_Init(SDL_INIT_VIDEO) <= -1) {
        devilution::ErrSdl();
    }

    devilution::ghMainWnd = SDL_CreateWindow("Diablo launcher", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1, 1, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
    launcher::AppResult launcherResult;
    {
        // Скоуп ради раннего ~Application: рендерер, ImGui и текстуры
        // лаунчера умирают сразу после Run(), пока окно ещё живо — движок
        // создаст свой рендерер на этом же окне.
        auto app = App::Application(devilution::ghMainWnd, "org.diasurgical", "devilutionx");
        launcherResult = app.Run();
    }

    if (!launcherResult.success) {
        // Пользователь закрыл лаунчер, не выбрав игру — выходим из
        // приложения вместо запуска игры в режиме по умолчанию.
        SDL_DestroyWindow(devilution::ghMainWnd);
        devilution::ghMainWnd = nullptr;
        SDL_Quit();
        devilution::DisplayBlankerController::Shutdown();
        return 0;
    }

    // Handover: окно переезжает движку без пересоздания (SpawnWindow
    // обнаружит живой ghMainWnd) — без close/open анимаций липстика.
    // Хвост событий лаунчера (последний тап, фокус-шум) движку не нужен.
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

    // Папку с MPQ движок получает аргументом --data-dir и, независимо
    // от запуска, читает её же из launcher.ini (StandartPaths).
    engineArgs = launcher::EngineArgsFor(launcherResult.action, launcherResult.dataPath);
    if (!engineArgs.empty()) {
        engineArgv.push_back(argv[0]);
        for (const std::string &arg : engineArgs) {
            engineArgv.push_back(const_cast<char *>(arg.c_str()));
        }
        argc = static_cast<int>(engineArgv.size());
        argv = engineArgv.data();
    }
#endif
	const int result = devilution::DiabloMain(argc, argv);
#ifdef GPERF_HEAP_MAIN
	HeapProfilerStop();
#endif

#ifdef AURORA_OS
    devilution::DisplayBlankerController::Shutdown();
#endif
	return result;
}
