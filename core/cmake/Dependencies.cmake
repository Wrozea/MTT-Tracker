# Every dependency is pinned to a release archive and verified by SHA256, so a
# tag that is moved or an archive that is tampered with fails the build instead
# of silently changing what we compile.

include(FetchContent)

# Eigen is header-only. SOURCE_SUBDIR names a folder with no CMakeLists.txt,
# so FetchContent only downloads Eigen instead of running Eigen's own build,
# which would configure its test suite and documentation.
FetchContent_Declare(eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR headers-only)
FetchContent_MakeAvailable(eigen)
# SYSTEM hides warnings from inside Eigen's headers, which would otherwise
# fail our warnings-as-errors build.
add_library(Eigen3::Eigen INTERFACE IMPORTED)
target_include_directories(Eigen3::Eigen SYSTEM INTERFACE ${eigen_SOURCE_DIR})

if(MTT_BUILD_TESTS)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    # Match our runtime library on MSVC; otherwise linking tests fails with
    # mismatched /MD vs /MT.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()
