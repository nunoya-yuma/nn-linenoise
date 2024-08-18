cmake_minimum_required(VERSION 3.10)
project(NN_linenoise)

set(CMAKE_CXX_STANDARD 11)

add_executable(nn_cli_sample sample_main.c)

target_link_libraries(nn_cli_sample
    nn_linenoise
)

add_subdirectory(nn-linenoise)
