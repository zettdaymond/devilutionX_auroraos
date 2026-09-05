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
	    # Экспорт SDL_WaylandSetWindowGenericProperty: lipstick читает
	    # generic-свойства окна только с ПЕРВОГО qt_extended_surface,
	    # который SDL создаёт сам. Применяется после свежего клона —
	    # при смене патча снести _deps/sdl2-src.
	    PATCH_COMMAND sh -c "git apply ${CMAKE_CURRENT_LIST_DIR}/sdl-wayland-generic-property.patch 2>/dev/null || echo 'SDL patch: already applied'"
	)
else()
	FetchContent_Declare(SDL2
	    URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.0/SDL2-2.32.0.tar.gz
	    URL_HASH SHA256=f5c2b52498785858f3de1e2996eba3c1b805d08fe168a47ea527c7fc339072d0
	)
endif()

FetchContent_MakeAvailableExcludeFromAll(SDL2)
