if(CMAKE_CURRENT_SOURCE_DIR STREQUAL "${CMAKE_SOURCE_DIR}")
    cmake_minimum_required(VERSION 3.14)
    project(NN_CLI_PROJECT)

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
    PRIVATE
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
    add_subdirectory(test)
endif()
