include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
set(GTEST_HAS_ABSL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)

add_executable(launcher_tests
    GameFilesTests.cpp
    StoreTests.cpp
)
target_link_libraries(launcher_tests PRIVATE
    launcher_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(launcher_tests)
