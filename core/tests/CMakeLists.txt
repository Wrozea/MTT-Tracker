add_executable(mtt_tests
    motion_test.cpp
    random_test.cpp
    sensor_test.cpp
    simulator_test.cpp)
target_link_libraries(mtt_tests PRIVATE mtt_core GTest::gtest_main)
mtt_enable_warnings(mtt_tests)
# Must come before gtest_discover_tests, which runs the binary after linking.
mtt_copy_asan_runtime(mtt_tests)

include(GoogleTest)
gtest_discover_tests(mtt_tests)
