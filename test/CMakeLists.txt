add_executable(
    nn_cli_test
    nn_cli_test.cpp
)

target_link_libraries(
    nn_cli_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nn_cli_test)
