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
#   include "appfat.h"
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
    auto context = devilution::AuroraOsStandartPaths::MakeAppContext(argc, argv);

    devilution::DisplayBlankerController::Init();
    devilution::DisplayBlankerController::SetPreventDisplayBlanking(true);

    std::array<char*, 2> new_argv;

    if (SDL_Init(SDL_INIT_VIDEO) <= -1) {
        devilution::ErrSdl();
    }

    devilution::ghMainWnd = SDL_CreateWindow("Diablo launcher", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1, 1, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
    auto app = App::Application(devilution::ghMainWnd, "org.diasurgical", "devilutionx");
    const auto launcherResult = app.run();

    SDL_DestroyWindow(devilution::ghMainWnd);
    SDL_Quit();

    if(launcherResult.success) {
        if(launcherResult.action == App::ExitAction::LaunchDiablo
           || launcherResult.action == App::ExitAction::LaunchHellfire)
        {
            if(!launcherResult.dataPath.empty()) {
                devilution::AuroraOsStandartPaths::SetUserDefinedMPQSearchPath(launcherResult.dataPath.string());
            }
        }

        if(launcherResult.action == App::ExitAction::LaunchDemo) {
            new_argv = { argv[0], (char*)"--spawn" };
            argc = new_argv.size();
            argv = new_argv.data();
        }
        else if(launcherResult.action == App::ExitAction::LaunchHellfire) {
            new_argv = { argv[0], (char*)"--hellfire" };
            argc = new_argv.size();
            argv = new_argv.data();
        }
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
