if(CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
    cmake_minimum_required(VERSION 3.14)
    project(nn_cli_project)

    # GoogleTest requires at least C++14
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    option(BUILD_TESTING "Build the tests" ON)
endif()

add_library(nn_cli
    STATIC
    nn_cli.c
)

target_link_libraries(nn_cli
    linenoise_org
)

target_include_directories(nn_cli
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
    internal/
)

add_library(linenoise_org
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/linenoise.c
)

target_include_directories(linenoise_org
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/
)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(test)
endif()
