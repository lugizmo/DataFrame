# Filename: ToolsClangTidy.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

# ====== CLANG-TIDY ====================================================================================================

if(LUGIZMO_DF_USE_CLANG_TIDY)
    find_program(CLANG_TIDY_PATH NAMES clang-tidy)

    if(CLANG_TIDY_PATH)
        message(STATUS "Clang-Tidy found: ${CLANG_TIDY_PATH}")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PATH})
    else()
        message(WARNING "Clang-Tidy requested, but not found on the system.")
    endif()
endif()
