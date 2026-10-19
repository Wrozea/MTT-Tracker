# AddressSanitizer instruments every memory access so that reading freed,
# out-of-bounds, or uninitialized-heap memory stops the program with a report
# instead of silently returning garbage. Included before Dependencies.cmake so
# GoogleTest is instrumented too.

option(MTT_ENABLE_ASAN "Build with AddressSanitizer" OFF)
# UndefinedBehaviorSanitizer catches what ASan does not: signed overflow,
# out-of-range shifts, misaligned access, invalid enum or bool values.
option(MTT_ENABLE_UBSAN "Build with UndefinedBehaviorSanitizer (GCC and Clang only)" OFF)

if(MTT_ENABLE_UBSAN)
    if(MSVC)
        message(FATAL_ERROR "MSVC has no UndefinedBehaviorSanitizer; build with GCC or Clang")
    endif()
    # By default UBSan prints a message and keeps running, so a test could
    # still pass. -fno-sanitize-recover makes every report abort the program.
    add_compile_options(-fsanitize=undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

if(MTT_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        # MSVC's Debug defaults clash with ASan: runtime checks (/RTC1) are
        # rejected outright and incremental linking is ignored with a warning.
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/INCREMENTAL" "/INCREMENTAL:NO" CMAKE_EXE_LINKER_FLAGS_DEBUG
               "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# On Windows the ASan runtime is a DLL that lives next to cl.exe and is not on
# PATH. Copying it next to the executable lets it run from a plain shell, and
# lets gtest_discover_tests run the test binary at build time.
function(mtt_copy_asan_runtime target)
    if(NOT (MTT_ENABLE_ASAN AND MSVC))
        return()
    endif()
    get_filename_component(compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    file(GLOB runtime_dlls "${compiler_dir}/clang_rt.asan*_dynamic-x86_64.dll")
    if(NOT runtime_dlls)
        message(FATAL_ERROR "ASan runtime DLL not found next to ${CMAKE_CXX_COMPILER}")
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${runtime_dlls} "$<TARGET_FILE_DIR:${target}>")
endfunction()
