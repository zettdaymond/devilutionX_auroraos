#include <SDL.h>
#include <SDL_main.h>

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
#   define FUNC_EXPORT extern "C" __attribute__((visibility("default"))) int main
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
