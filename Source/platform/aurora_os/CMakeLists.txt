include(functions/devilutionx_library)

include(FindPkgConfig)
pkg_search_module(WAYLAND wayland-client )
pkg_search_module(DBUS REQUIRED dbus-1)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(AUDIORESOURCE REQUIRED audioresource)

add_devilutionx_object_library(libdevilutionx_aurora_os
    TextureRotatorGLES.cpp
    VirtualPadSDLRender.cpp
    Utilities.cpp
    GLUtils.cpp
    GLFunctions.cpp
    ComposerAdapter.cpp
    ScreenOrientation.cpp
    InputAdapter.cpp
    StandartPaths.cpp
    DisplayBlankerController.cpp
    DBus.cpp
    AuroraOSAudio.cpp
)

target_include_directories(libdevilutionx_aurora_os
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(libdevilutionx_aurora_os
    PRIVATE
#        ${DBUS_INCLUDE_DIRS}
        "/usr/include/dbus-1.0"
        "/usr/lib/dbus-1.0/include"
        ${AUDIORESOURCE_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(libdevilutionx_aurora_os
    PUBLIC
        SDL2::SDL2
        SDL2::SDL2_image
    PRIVATE
        simpleini
        ${AUDIORESOURCE_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${WAYLAND_LIBRARIES}
        ${DBUS_LIBRARIES}
)

add_subdirectory(devilution_launcher)
target_link_libraries(libdevilutionx_aurora_os PUBLIC devilution_launcher)
