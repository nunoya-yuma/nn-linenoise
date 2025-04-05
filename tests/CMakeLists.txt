include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

add_executable(nn_cli_test
    nn_cli_test.cpp
)

target_link_libraries(nn_cli_test
    GTest::gtest_main
    nn_cli
)

target_include_directories(nn_cli_test
    PRIVATE
    ../third_party/linenoise/repo
    ../internal
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(nn_cli
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(nn_cli_test)
