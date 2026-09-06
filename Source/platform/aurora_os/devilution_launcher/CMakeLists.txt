cmake_minimum_required(VERSION 3.19)

project(devilution_launcher LANGUAGES CXX)

option(LAUNCHER_DESKTOP "Build the standalone desktop launcher executable" OFF)
# Отладочные env-ручки (DebugConfig.hpp): в Debug читаются из окружения,
# в Release без флага вырождаются в constexpr и выпиливаются компилятором.
if(NOT DEFINED DEVILUTIONX_DEBUG_ENV_GATES)
  if("${CMAKE_BUILD_TYPE}" MATCHES "^Debug$")
    set(DEVILUTIONX_DEBUG_ENV_GATES ON)
  else()
    set(DEVILUTIONX_DEBUG_ENV_GATES OFF)
  endif()
endif()
option(DEVILUTIONX_DEBUG_ENV_GATES "" ${DEVILUTIONX_DEBUG_ENV_GATES})
option(LAUNCHER_TESTS "Build launcher unit tests" ${LAUNCHER_DESKTOP})
# Отладка UI по скриншотам через vision-модели: обычная тёмная ручка
# включённого тумблера путает модели (читают «золотая ручка» как вкл),
# этот флаг делает ручку золотой в обоих состояниях.
option(LAUNCHER_VISION_DEBUG_TOGGLES "Постоянная золотая ручка тумблеров для vision-отладки" OFF)
# Кинетическая прокрутка: равномерное торможение (по умолчанию) или
# экспоненциальное затухание — сравниваем варианты на устройстве.
option(LAUNCHER_EXPONENTIAL_GLIDE "Инерция прокрутки с экспоненциальным затуханием" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT LAUNCHER_DESKTOP)
  # Part of the devilutionX Aurora OS build: use the project helpers.
  include(functions/FetchContent_MakeAvailableExcludeFromAll)
endif()

include(FetchContent)
include(CMakeRC.cmake)

cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(ZOE_BUILD_SHARED_LIBS OFF)
set(ZOE_BUILD_TESTS OFF)
set(SPDLOG_NO_EXCEPTIONS ON)
set(BUILD_SHARED_LIBS OFF)
set(SPDLOG_BUILD_SHARED OFF)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui"
  GIT_TAG 44aa9a4b3a6f27d09a4eb5770d095cbd376dfc4b # Branch: docking, date: 10.09.2024, 04:29 GMT+2
  GIT_SHALLOW
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY "https://github.com/gabime/spdlog"
  GIT_TAG v1.14.1
  GIT_SHALLOW
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  zoe
  GIT_REPOSITORY "https://github.com/winsoft666/zoe"
  GIT_TAG v3.6
  EXCLUDE_FROM_ALL
  # zoe несёт install-правила: ninja install тянет её заголовки/статик-либу
  # и cmake-конфиги в staging, и rpm-BT падает на "unpackaged files".
  # Патч гасит правила. Шаг идемпотентен (окружения SDK mb2/BT делят одно
  # build-дерево, патч может попытаться примениться повторно — скрипт это
  # распознаёт) и выполняется через cmake -P: единственный интерпретатор,
  # доступный и в Linux-контейнерах, и в cmd.exe Qt Creator (там нет sh).
  PATCH_COMMAND ${CMAKE_COMMAND} -P
      ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zoe-patch.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zoe-no-install.patch
)
FetchContent_MakeAvailable(zoe)
# zoe's own CMakeLists points at ${CMAKE_SOURCE_DIR}/include, which is
# wrong when zoe is embedded via FetchContent — fix the include path
# explicitly and expose a namespaced alias.
target_include_directories(zoe PUBLIC "$<BUILD_INTERFACE:${zoe_SOURCE_DIR}/include>")
if(NOT TARGET zoe::zoe)
  add_library(zoe::zoe ALIAS zoe)
endif()

# SimpleIni уже используется движком (3rdParty/simpleini — тот же
# pinned-тарбалл). На Aurora цель создаёт корневая сборка, здесь весь
# блок пропускается; в standalone-десктопе подтягиваем копию сами.
if(NOT TARGET simpleini)
  include(${CMAKE_CURRENT_SOURCE_DIR}/../../../../CMake/functions/FetchContent_MakeAvailableExcludeFromAll.cmake)
  FetchContent_Declare(simpleini
      URL https://github.com/brofield/simpleini/archive/56499b5af5d2195c6acfc58c4630b70e0c9c4c21.tar.gz
      URL_HASH MD5=02a561cea03ea11acb65848318ec4a81
  )
  FetchContent_MakeAvailableExcludeFromAll(simpleini)
  add_library(simpleini INTERFACE)
  target_include_directories(simpleini INTERFACE ${simpleini_SOURCE_DIR})
endif()

if(LAUNCHER_DESKTOP)
  # CONFIG mode picks the shared import library (SDL2.dll.a). Арты и
  # иконки декодирует вендоренный stb_image, SDL_image не нужен вовсе.
  find_package(SDL2 CONFIG REQUIRED)
  find_package(CURL REQUIRED)
endif()

if(DEVILUTIONX_DEBUG_ENV_GATES)
  add_compile_definitions(DEVILUTIONX_DEBUG_ENV_GATES=1)
endif()

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui.h
  ${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_internal.h
  ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imstb_rectpack.h ${imgui_SOURCE_DIR}/imstb_textedit.h
  ${imgui_SOURCE_DIR}/imstb_truetype.h

  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp

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
        assets/bg.jpg
        assets/hero_diablo.jpg
        assets/hero_hellfire.jpg
        assets/hero_demo.jpg
        assets/icon_diablo.png
        assets/icon_hellfire.png
        assets/icon_demo.png
        assets/cover_face_diablo.png
        assets/cover_face_hellfire.png
        assets/Beaufort-Bold.ttf
        assets/Beaufort-Regular.ttf
        assets/Roboto-Bold.ttf
        assets/Roboto-Regular.ttf
        assets/fontawesome-webfont.ttf
        assets/exocet1.ttf
        assets/exocet2.ttf
        assets/exotec_cyrillic.ttf
)

# ---------------------------------------------------------------------------
# launcher_core: portable MVI core + services + mocks (no SDL, no ImGui,
# no Qt). Shared by the desktop executable and unit tests.
# ---------------------------------------------------------------------------
set(LAUNCHER_CORE_SOURCES
    core/EngineLaunch.cpp
    core/GameFiles.cpp
    core/Store.cpp
    services/ConfigService.cpp
    services/DesktopPathProvider.cpp
    services/EngineOptionsService.cpp
    services/GameFilesService.cpp
    services/ServiceFactory.cpp
    services/ZoeDownloadService.cpp
    services/mocks/MockDownloadService.cpp
    services/mocks/MockScenarios.cpp
    services/mocks/MockServices.cpp
    services/mocks/MockWorld.cpp
)

set(LAUNCHER_UI_SOURCES
    Application.cpp
    DPIHandler.cpp
    thirdparty/FileBrowser.cpp
    ui/Format.cpp
    ui/LauncherView.cpp
    ui/Scale.cpp
    ui/Theme.cpp
    ui/dialogs/Dialogs.cpp
    ui/screens/Screens.cpp
    ui/widgets/Widgets.cpp
)

# App version from the repository VERSION file.
set(LAUNCHER_APP_VERSION_STRING "dev")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../VERSION")
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../VERSION" LAUNCHER_VERSION_LINE LIMIT_COUNT 1)
  string(STRIP "${LAUNCHER_VERSION_LINE}" LAUNCHER_VERSION_LINE)
  if(NOT "${LAUNCHER_VERSION_LINE}" STREQUAL "")
    set(LAUNCHER_APP_VERSION_STRING "${LAUNCHER_VERSION_LINE}")
  endif()
endif()

if(LAUNCHER_DESKTOP)

  add_library(launcher_core OBJECT ${LAUNCHER_CORE_SOURCES})
  target_include_directories(launcher_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
  target_link_libraries(launcher_core PUBLIC spdlog::spdlog zoe::zoe simpleini)

  add_executable(devilution_launcher
      ${LAUNCHER_UI_SOURCES}
      main.cpp
  )
  target_include_directories(devilution_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(devilution_launcher PRIVATE LAUNCHER_APP_VERSION="${LAUNCHER_APP_VERSION_STRING}")
  target_link_libraries(devilution_launcher
      PRIVATE
          launcher_core
          spdlog::spdlog
          zoe::zoe
          imgui
          launcher::assets
          SDL2::SDL2
  )

  if(LAUNCHER_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif()

else()

  # Aurora OS: object libraries linked into the game binary.
  add_devilutionx_object_library(launcher_core ${LAUNCHER_CORE_SOURCES} services/AuroraPathProvider.cpp)
  target_include_directories(launcher_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
  # AuroraPathProvider includes StandartPaths.hpp from the parent directory.
  target_include_directories(launcher_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
  # Цель simpleini к этому моменту уже создана корневой сборкой
  # (CMake/Dependencies.cmake -> 3rdParty/simpleini).
  target_link_libraries(launcher_core PUBLIC spdlog::spdlog zoe::zoe simpleini)

  # AuroraStateWatch.cpp — наблюдатель D-Bus, GameCover.cpp — обложка
  # плитки на время игры (только Aurora-сборка).
  add_devilutionx_object_library(devilution_launcher ${LAUNCHER_UI_SOURCES} AuroraStateWatch.cpp GameCover.cpp)
  target_include_directories(devilution_launcher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  # Наблюдателю нужны заголовки D-Bus: те же пути инклюдов, что у
  # соседней libdevilutionx_aurora_os.
  target_include_directories(devilution_launcher PUBLIC "/usr/include/dbus-1.0" "/usr/lib/dbus-1.0/include")
  target_compile_definitions(devilution_launcher PRIVATE LAUNCHER_APP_VERSION="${LAUNCHER_APP_VERSION_STRING}")
  target_link_libraries(devilution_launcher
      PUBLIC
          launcher_core
          imgui
          launcher::assets
          SDL2::SDL2
  )

endif()

# Обе ветки (desktop exe и aurora object library) собирают ui/widgets.
if(LAUNCHER_VISION_DEBUG_TOGGLES)
  target_compile_definitions(devilution_launcher PRIVATE LAUNCHER_VISION_DEBUG_TOGGLES=1)
endif()
if(LAUNCHER_EXPONENTIAL_GLIDE)
  target_compile_definitions(devilution_launcher PRIVATE LAUNCHER_EXPONENTIAL_GLIDE=1)
endif()
