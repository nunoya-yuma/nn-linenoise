add_library(nn_linenoise
    STATIC
    nn_linenoise.c
)

target_link_libraries(nn_linenoise
    linenoise_org
)

target_include_directories(nn_linenoise
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(linenoise_org
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/linenoise.c
)

target_include_directories(linenoise_org
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/repo/
)
