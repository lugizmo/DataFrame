# Filename: BuildOptions.cmake
# Copyright 2025 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

function(set_lugizmo_global_build_flags)
    # === User Configurable Options ===
    option(LUGIZMO_ENABLE_WARNINGS "Enable warnings" ON)
    option(LUGIZMO_ENABLE_SANITIZERS "Enable sanitizers (UBSan, ASan)" OFF)
    option(LUGIZMO_ENABLE_OPTIMIZATIONS "Enable high optimizations (O3, native arch)" ON)
    option(LUGIZMO_ENABLE_LTO "Enable link time optimization" OFF)
    option(LUGIZMO_STRICT_MODE "Enable strict mode (pedantic, Werror, etc)" OFF)

    if(WIN32)
        message(FATAL_ERROR "Windows is not supported by set_lugizmo_global_build_flags()")
    endif()

    # === Defaults for Debug builds ===
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LUGIZMO_ENABLE_WARNINGS ON)
        set(LUGIZMO_STRICT_MODE ON)
        set(LUGIZMO_ENABLE_OPTIMIZATIONS OFF)
    endif()

    set(CXX_FLAGS "")
    set(LINK_FLAGS "")

    # === Warnings & Strict Mode ===
    if(LUGIZMO_ENABLE_WARNINGS)
        list(APPEND CXX_FLAGS -Wall -Wextra -Wshadow)
        if(LUGIZMO_STRICT_MODE)
            list(APPEND CXX_FLAGS -Wpedantic -Werror -Wconversion -Wsign-conversion)
        endif()
    endif()

    # === Optimizations ===
    if(LUGIZMO_ENABLE_OPTIMIZATIONS AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND CXX_FLAGS -O3 -march=native)
    endif()

    # === Sanitizers (only meaningful in Debug builds) ===
    if(LUGIZMO_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND CXX_FLAGS -fsanitize=address -fsanitize=undefined)
        list(APPEND LINK_FLAGS -fsanitize=address -fsanitize=undefined)
    endif()

    # === Link Time Optimization ===
    # Ensure `CheckIPOSupported` runs only after the project and languages are initialized
    if(LUGIZMO_ENABLE_LTO)
        if(NOT TARGET IPO_SUPPORTED_CHECK_DONE)
            include(CheckIPOSupported)
            # Create a dummy target to mark IPO check as completed
            add_custom_target(IPO_SUPPORTED_CHECK_DONE ALL COMMAND ${CMAKE_COMMAND} -E echo "IPO check done")
            check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
        endif()

        if(ipo_supported)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(WARNING "LTO not supported: ${ipo_error}")
        endif()
    endif()


    # === Platform-Specific: Apple Hardening for libc++ ===
    if(APPLE)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            # Mild hardening always in Debug mode
            list(APPEND CXX_FLAGS
                    -fstack-protector-strong
                    -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NORMAL
                    -stdlib=libc++
            )
            # Enable hardcore checks if strict mode is active
            if(LUGIZMO_STRICT_MODE)
                list(APPEND CXX_FLAGS
                        -fstrict-enums
                        -fstack-protector-all
                        -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
                )
            endif()
        elseif() # Release/RelWithDebInfo usually
            # Mild hardening only in strict mode for Release/RelWithDebInfo
            if(LUGIZMO_STRICT_MODE)
                list(APPEND CXX_FLAGS
                        -fstack-protector-strong
                        -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NORMAL
                        -stdlib=libc++
                )
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        # fPIC and common Linux safety options
        list(APPEND CXX_FLAGS -fPIC)
    endif()

    # === Apply globally ===
    string(REPLACE ";" " " CXX_FLAGS_STR "${CXX_FLAGS}")
    string(REPLACE ";" " " LINK_FLAGS_STR "${LINK_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS_STR}" PARENT_SCOPE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAGS_STR}" PARENT_SCOPE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINK_FLAGS_STR}" PARENT_SCOPE)

    message(STATUS "Lugizmo DF - build flags applied:")
    message(STATUS "  CXX_FLAGS: ${CXX_FLAGS_STR}")
    message(STATUS "  LINK_FLAGS: ${LINK_FLAGS_STR}")
endfunction()