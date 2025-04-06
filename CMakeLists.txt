if(CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
    cmake_minimum_required(VERSION 3.14)
    project(NN_CLI_PROJECT)

    # GoogleTest requires at least C++14
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    option(BUILD_TESTING "Build the tests" ON)
endif()

add_library(linenoise_org
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/linenoise/repo/linenoise.c
)

target_include_directories(linenoise_org
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/linenoise/repo/
)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
