add_executable(
    nn_linenoise_test
    nn_linenoise_test.cpp
)

target_link_libraries(
    nn_linenoise_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nn_linenoise_test)
