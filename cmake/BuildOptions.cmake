# Filename: BuildOptions.cmake
# Copyright 2025 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

function(set_lugizmo_global_build_flags)

    # === Flag check function ===
    include(CheckCXXCompilerFlag)
    function(_add_flag_if_supported out_list flag)
        string(REPLACE "-" "_" _flag_id "${flag}")
        string(REPLACE "=" "_" _flag_id "${_flag_id}")
        check_cxx_compiler_flag("${flag}" "HAS_${_flag_id}")
        if(HAS_${_flag_id})
            list(APPEND ${out_list} "${flag}")
            set(${out_list} "${${out_list}}" PARENT_SCOPE)
        endif()
    endfunction()

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
        foreach(f -Wall -Wextra -Wshadow)
            _add_flag_if_supported(CXX_FLAGS "${f}")
        endforeach()
        if(LUGIZMO_STRICT_MODE)
            foreach(f -Wpedantic -Werror -Wconversion -Wsign-conversion)
                _add_flag_if_supported(CXX_FLAGS "${f}")
            endforeach()
        endif()
    endif()

    # === Optimizations ===
    if(LUGIZMO_ENABLE_OPTIMIZATIONS AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        foreach(f -O3 -march=native)
            _add_flag_if_supported(CXX_FLAGS "${f}")
        endforeach()
    endif()

    # === Sanitizers (only meaningful in Debug builds) ===
    if(LUGIZMO_ENABLE_SANITIZERS AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
        check_cxx_compiler_flag("-fsanitize=address" HAS_fsanitize_address)
        if(HAS_fsanitize_address)
            _add_flag_if_supported(CXX_FLAGS "-fsanitize-address-use-after-scope")
        endif()
        foreach(f -fsanitize=address -fsanitize=undefined)
            _add_flag_if_supported(CXX_FLAGS "${f}")
        endforeach()
        foreach(f -fsanitize=address -fsanitize=undefined)
            _add_flag_if_supported(LINK_FLAGS "${f}")
        endforeach()
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
            if(NOT LUGIZMO_STRICT_MODE)
                # Mild hardening always in Debug mode
                foreach(f -fstack-protector-strong -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${f}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
            else()
                # Enable hardcore checks if strict mode is active
                foreach(f -fstrict-enums -fstack-protector-all -fno-omit-frame-pointer -fno-optimize-sibling-calls -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${f}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
            endif()
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            if(LUGIZMO_STRICT_MODE)
                foreach(f -fstack-protector-strong -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${f}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
            endif()
        else()
            # Mild hardening only in strict mode for Release/RelWithDebInfo
            if(LUGIZMO_STRICT_MODE)
                foreach(f -fstack-protector-strong -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${f}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        # fPIC and common Linux safety options
        _add_flag_if_supported(CXX_FLAGS -fPIC)
    endif()

    # === Apply globally ===
    set(LUGIZMO_CXX_FLAGS  "${CXX_FLAGS}"  PARENT_SCOPE)
    set(LUGIZMO_LINK_FLAGS "${LINK_FLAGS}" PARENT_SCOPE)

    # Pretty printing only
    list(JOIN CXX_FLAGS " " CXX_FLAGS_STR)
    list(JOIN LINK_FLAGS " " LINK_FLAGS_STR)
    message(STATUS "Lugizmo DF - build flags prepared (per-target):")
    message(STATUS "  CXX_FLAGS: ${CXX_FLAGS_STR}")
    message(STATUS "  LINK_FLAGS: ${LINK_FLAGS_STR}")
endfunction()