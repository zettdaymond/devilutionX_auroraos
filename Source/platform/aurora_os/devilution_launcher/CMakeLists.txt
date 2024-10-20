cmake_minimum_required(VERSION 3.19)

include(functions/FetchContent_MakeAvailableExcludeFromAll)

cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

include(FetchContent)

set(ZOE_BUILD_SHARED_LIBS OFF)
set(ZOE_BUILD_TESTS OFF)
set(SPDLOG_NO_EXCEPTIONS ON)
set(BUILD_SHARED_LIBS OFF)
set(SPDLOG_FMT_EXTERNAL ON)
set(SPDLOG_BUILD_SHARED OFF)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG 68aa9a86ec933510073932980a0940742ecc833c # Branch: docking, date: 10.09.2024, 04:29 GMT+2
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG v1.14.1
)

FetchContent_Declare(
  zoe
  GIT_REPOSITORY "https://github.com/winsoft666/zoe"
  GIT_TAG v3.1
)

FetchContent_Declare(
  imfilebrowser
  GIT_REPOSITORY "https://github.com/AirGuanZ/imgui-filebrowser"
  GIT_TAG 60d4e09ab1270d94d0115ad8ec40f939e801e105
)


FetchContent_MakeAvailableExcludeFromAll(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui.h
  ${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_internal.h
  ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imstb_rectpack.h ${imgui_SOURCE_DIR}/imstb_textedit.h
  ${imgui_SOURCE_DIR}/imstb_truetype.h
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.h ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.h ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp)

target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_compile_features(imgui PRIVATE cxx_std_20)
target_link_libraries(imgui PUBLIC SDL2::SDL2)


# Populate

FetchContent_MakeAvailableExcludeFromAll(spdlog zoe imfilebrowser)

include(CMakeRC.cmake)

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
)

add_devilutionx_object_library(devilution_launcher
    Application.cpp
    Application.hpp
    DPIHandler.cpp
    DPIHandler.hpp
    LauncherController.cpp
    LauncherController.hpp
    LauncherModel.hpp
    LauncherView.hpp
    LauncherView.cpp
    widgets/imgui/ImguiExtension.cpp
    widgets/imgui/ImguiExtension.h
)

find_package(CURL REQUIRED)

target_link_libraries(devilution_launcher
    PUBLIC
        SDL2::SDL2
        SDL2::SDL2_image
        fmt::fmt
        imgui
        spdlog::spdlog
        launcher::assets
        zoe-static
        CURL::libcurl
)

target_include_directories(devilution_launcher
    PRIVATE
        ${zoe_SOURCE_DIR}/include/zoe
        ${imfilebrowser_SOURCE_DIR}
    PUBLIC
    	${CMAKE_CURRENT_SOURCE_DIR}
)
