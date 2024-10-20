cmake_minimum_required(VERSION 3.19)

include(FetchContent)
include(CMakeRC.cmake)

if(LAUNCHER_DESKTOP)

  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  # FetchContent_Declare(
  #           fmt
  #           URL https://github.com/fmtlib/fmt/releases/download/11.2.0/fmt-11.2.0.zip
  #           DOWNLOAD_EXTRACT_TIMESTAMP true
  #           OVERRIDE_FIND_PACKAGE
  # )

  # FetchContent_MakeAvailable(fmt)

  # FetchContent_Declare(
  #     zlib
  #     GIT_REPOSITORY "https://github.com/madler/zlib.git"
  #     FIND_PACKAGE_ARGS NAMES ZLIB
  # )
  # FetchContent_MakeAvailable(zlib)
  # add_library(ZLIB::ZLIB ALIAS zlibstatic)


  # FetchContent_Declare(
  #         SDL2
  #         GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  #         GIT_TAG release-2.26.3
  #         GIT_SHALLOW TRUE
  #         GIT_PROGRESS TRUE
  # )
  # FetchContent_MakeAvailable(SDL2)

  find_package(SDL2)

  # FetchContent_Declare(
  #   ssl_content
  #   URL      https://github.com/janbar/openssl-cmake/archive/refs/tags/1.1.1k-20210430.tar.gz
  #   URL_HASH MD5=f4294e5de7ebb20f9a695155660f1afd
  #   DOWNLOAD_EXTRACT_TIMESTAMP true
  # )
  # FetchContent_MakeAvailable(ssl_content)
  # add_library(OpenSSL::SSL ALIAS ssl)
  # add_library(OpenSSL::Crypto ALIAS crypto)


  # FetchContent_Declare(curl
  #     URL https://curl.se/download/curl-8.5.0.tar.gz
  #     DOWNLOAD_EXTRACT_TIMESTAMP true
  #     OVERRIDE_FIND_PACKAGE # this one
  # )

  # FetchContent_MakeAvailable(curl)

  find_package(CURL REQUIRED)

  FetchContent_Declare(SDL2_image
      URL https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-2.8.8.zip
      DOWNLOAD_EXTRACT_TIMESTAMP true
      OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(SDL2_image)

  find_package(SDL2_image REQUIRED)

  add_library(SDL2::SDL2_image ALIAS SDL2::image)

else()
  include(functions/FetchContent_MakeAvailableExcludeFromAll)
endif()

cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(ZOE_BUILD_SHARED_LIBS OFF)
set(ZOE_BUILD_TESTS OFF)
set(SPDLOG_NO_EXCEPTIONS ON)
# set(BUILD_SHARED_LIBS OFF)
# set(SPDLOG_FMT_EXTERNAL ON)
set(SPDLOG_BUILD_SHARED OFF)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG 44aa9a4b3a6f27d09a4eb5770d095cbd376dfc4b # Branch: docking, date: 10.09.2024, 04:29 GMT+2
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  spdlog
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  zoe
  GIT_REPOSITORY "https://github.com/winsoft666/zoe"
  GIT_TAG v3.2
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(zoe)

FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY "https://github.com/marzer/tomlplusplus.git"
  GIT_TAG v3.4.0
  GIT_SHALLOW
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(tomlplusplus)


add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui.h
  ${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_internal.h
  ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imstb_rectpack.h ${imgui_SOURCE_DIR}/imstb_textedit.h
  ${imgui_SOURCE_DIR}/imstb_truetype.h

  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  # ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp

  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.h ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_compile_features(imgui PRIVATE cxx_std_20)
target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(imgui PUBLIC SDL2::SDL2)


cmrc_add_resource_library(launcher_assets
    ALIAS
        launcher::assets
    NAMESPACE
        assets
    WHENCE
        assets
    PREFIX
        assets
    assets/bg.png
    assets/Beaufort-Bold.ttf
    assets/Beaufort-Regular.ttf
    assets/Roboto-Bold.ttf
    assets/Roboto-Regular.ttf
    assets/fontawesome-webfont.ttf
    assets/exocet1.ttf
    assets/exocet2.ttf
)

set(LAUNCHER_SOURCES
    Application.cpp
    Application.hpp
    DPIHandler.cpp
    DPIHandler.hpp
    thirdparty/FileBrowser.cpp
    GameView.cpp
    GameView.hpp
    GameModel.hpp
    GameModel.cpp
    GamePresenter.cpp
    GamePresenter.hpp
)

if(LAUNCHER_DESKTOP)
  add_executable(devilution_launcher
      ${LAUNCHER_SOURCES}
      main.cpp
  )
else()
  add_devilutionx_object_library(devilution_launcher
      ${LAUNCHER_SOURCES}
  )
endif()

target_link_libraries(devilution_launcher
    PUBLIC
        SDL2::SDL2
        SDL2::SDL2_image
        # fmt::fmt
        imgui
        spdlog::spdlog
        launcher::assets
        zoe::zoe
        CURL::libcurl
        tomlplusplus::tomlplusplus
)

target_include_directories(devilution_launcher
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
