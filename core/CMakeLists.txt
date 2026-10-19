cmake_minimum_required(VERSION 3.24)

project(mtt_tracker
    VERSION 0.1.0
    DESCRIPTION "Real-time multi-target tracker"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fusing a * b + c into one FMA instruction rounds once instead of twice, so
# results depend on whether the compiler chose to fuse. Turning it off keeps
# GCC and Clang closer to MSVC, whose default /fp:precise does not fuse.
if(NOT MSVC)
    add_compile_options(-ffp-contract=off)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MTT_BUILD_TESTS "Build unit tests" ON)
option(MTT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(cmake/Warnings.cmake)
include(cmake/Dependencies.cmake)

add_library(mtt_core
    src/version.cpp)
target_include_directories(mtt_core PUBLIC include)
mtt_enable_warnings(mtt_core)

add_executable(tracker apps/tracker_main.cpp)
target_link_libraries(tracker PRIVATE mtt_core)
mtt_enable_warnings(tracker)

if(MTT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
