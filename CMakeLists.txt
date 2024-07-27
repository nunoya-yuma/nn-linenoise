cmake_minimum_required(VERSION 3.10)
project(NN_linenoise)

set(CMAKE_CXX_STANDARD 11)

add_library(linenoise_org
    STATIC
    repo/linenoise.c
)

include_directories(repo)

add_executable(nn_linenoise sample_main.c)

target_link_libraries(nn_linenoise linenoise_org)
