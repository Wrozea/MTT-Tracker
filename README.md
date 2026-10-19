# mtt-tracker

A real-time multi-target tracking system: a C++ core that tracks targets from
noisy simulated sensor detections, and a live web dashboard that shows it
working.

Work in progress. The full README (architecture, algorithms, metrics, known
limitations) is written in the final milestone.

## Build and test the core

Requires CMake 3.24+ and a C++20 compiler (MSVC 2022+, GCC 11+, or Clang 14+).

    cmake -S core -B build
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure
