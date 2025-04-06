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
    ${CMAKE_CURRENT_SOURCE_DIR}/../internal
)
