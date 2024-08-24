cmake_minimum_required(VERSION 3.10)
project(nn_example)

set(CMAKE_CXX_STANDARD 11)

add_executable(nn_cli_sample sample_main.c)

# Set linker options to generate a map file
set_target_properties(nn_cli_sample PROPERTIES
    LINK_FLAGS "-Wl,-Map=${CMAKE_BINARY_DIR}/nn_cli_sample.map"
)

target_link_libraries(nn_cli_sample
    nn_cli
)

add_subdirectory(nn-linenoise)
