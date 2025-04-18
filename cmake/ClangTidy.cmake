# Filename: ClangTidy.cmake
# Copyright 2025 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

# TODO test this on another system
if(LUGIZMO_USE_CLANG_TIDY)
    find_program(CLANG_TIDY_PATH NAMES clang-tidy)

    if (CLANG_TIDY_PATH)
        message(STATUS "Clang-Tidy found: ${CLANG_TIDY_PATH}")
        # Set the clang-tidy tool for CMake
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH})
    else()
        message(WARNING "Clang-Tidy requested, but not found on the system.")
    endif()
endif()
