if(DEVILUTIONX_STATIC_SDL2)
  set(BUILD_SHARED_LIBS OFF)
  set(SDL_SHARED OFF)
  set(SDL_STATIC ON)
  if(PIE)
    set(SDL_STATIC_PIC ON)
  endif()
else()
  set(BUILD_SHARED_LIBS ON)
  set(SDL_SHARED ON)
  set(SDL_STATIC OFF)
endif()
set(SDL_TEST_ENABLED_BY_DEFAULT OFF)

include(functions/FetchContent_MakeAvailableExcludeFromAll)
include(FetchContent)

if(DEVILUTIONX_AURORA_OS)
	FetchContent_Declare(SDL2
	    GIT_REPOSITORY https://github.com/savegame/SDL.git
	    GIT_TAG        aurora-sdl-2.30.0
	    GIT_SHALLOW    1
	)
else()
	FetchContent_Declare(SDL2
	    URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.5/SDL2-2.30.5.tar.gz
	    URL_HASH SHA256=f374f3fa29c37dfcc20822d4a7d7dc57e58924d1a5f2ad511bfab4c8193de63b
	)
endif()

FetchContent_MakeAvailableExcludeFromAll(SDL2)
