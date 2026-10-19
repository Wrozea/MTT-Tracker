add_executable(mtt_tests
    version_test.cpp)
target_link_libraries(mtt_tests PRIVATE mtt_core GTest::gtest_main)
mtt_enable_warnings(mtt_tests)

include(GoogleTest)
gtest_discover_tests(mtt_tests)
