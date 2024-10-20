cmake_minimum_required(VERSION 3.19)

project(org.diasurgical.devilutionx CXX)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache) # Less useful to do it for linking, see edit2
endif(CCACHE_FOUND)

include(CheckIPOSupported)
check_ipo_supported(RESULT is_lto_supported OUTPUT has_lto_error)
if(is_lto_supported)
    message(INFO "Build with LTO ON")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
else()
    message(INFO "Build with LTO OFF")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/lib")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Disable sanitizers. They're not supported out-of-the-box.
set(NONET ON)
set(ASAN OFF)
set(UBSAN OFF)
set(DEBUG OFF)
set(CPACK OFF)
set(DEVILUTIONX_STATIC_CXX_STDLIB OFF)
set(NOEXIT ON)
set(PIE ON)
set(BUILD_TESTING OFF)

set(DEVILUTIONX_AURORA_OS ON)
set(DEVILUTIONX_AURORA_OS_DPI_GAMEPAD_FIX OFF)
set(DEVILUTIONX_SYSTEM_SDL2 OFF)
set(DEVILUTIONX_STATIC_SDL2 ON)
set(DEVILUTIONX_SYSTEM_SDL_IMAGE OFF)
set(DEVILUTIONX_STATIC_SDL_IMAGE ON)
set(DEVILUTIONX_SYSTEM_LIBPNG ON)
set(DEVILUTIONX_SYSTEM_BZIP2 ON)
set(DEVILUTIONX_SYSTEM_SDL_AUDIOLIB OFF)
set(DEVILUTIONX_SYSTEM_LIBSODIUM OFF)
set(DEVILUTIONX_SYSTEM_LIBFMT OFF)
set(DEVILUTIONX_SYSTEM_LUA OFF)
set(DEVILUTIONX_DISABLE_STRIP ON)
set(DEVILUTIONX_DISABLE_EXCEPTIONS OFF)

# Package the assets with the APK.
list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/smpq/${CMAKE_SYSTEM_PROCESSOR}")
set(BUILD_ASSETS_MPQ ON)
set(DEVILUTIONX_ASSETS_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/share/${PROJECT_NAME}/assets")

list(APPEND DEVILUTIONX_PLATFORM_SUBDIRECTORIES platform/aurora_os)
list(APPEND DEVILUTIONX_PLATFORM_LINK_LIBRARIES libdevilutionx_aurora_os)

add_definitions(-DAURORA_OS)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. devilutionX)

add_executable(${PROJECT_NAME} "src/dummy.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC devilution::lib)
target_link_options(${PROJECT_NAME} PUBLIC -Wl,--gc-sections -rdynamic)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_ASSETS_MPQ)
    install(FILES "${CMAKE_BINARY_DIR}/devilutionX/devilutionx.mpq"
      DESTINATION share/org.diasurgical.devilutionx/assets
    )
else()
    install(DIRECTORY ${CMAKE_BINARY_DIR}/share/${PROJECT_NAME}/assets
        DESTINATION
            share/org.diasurgical.devilutionx
    )
endif()

set(IconSize 86x86 108x108 128x128 172x172)
foreach(_size IN LISTS IconSize)
    install(FILES icons/${_size}/${PROJECT_NAME}.png DESTINATION share/icons/hicolor/${_size}/apps)
endforeach()

install(FILES ${PROJECT_NAME}.desktop
    DESTINATION share/applications
)

# Get the other files reachable from the project tree in Qt Creator
file(GLOB_RECURSE IconFiles "icons" "*.png")
set(RESOURCE_FILES ${IconFiles})

add_custom_target(distfiles
    SOURCES
        ${PROJECT_NAME}.desktop
        rpm/${PROJECT_NAME}.spec
        rpm/${PROJECT_NAME}.changes.in
        rpm/${PROJECT_NAME}.changes.run.in
        ${IconFiles}
        ${AssetsFiles}
)

file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
    "${CMAKE_INSTALL_PREFIX}
     ${CMAKE_BINARY_DIR}/${PROJECT_NAME}:bin")
