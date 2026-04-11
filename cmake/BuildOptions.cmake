# Filename: BuildOptions.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

function(set_lugizmo_global_build_flags)

    # ====== FLAG CHECK ================================================================================================
    include(CheckCXXCompilerFlag)

    function(_add_flag_if_supported OUT_LIST FLAG)
        string(REPLACE "-" "_" FLAG_ID "${FLAG}")
        string(REPLACE "=" "_" FLAG_ID "${FLAG_ID}")
        check_cxx_compiler_flag("${FLAG}" "HAS_${FLAG_ID}")
        if(HAS_${FLAG_ID})
            list(APPEND ${OUT_LIST} "${FLAG}")
            set(${OUT_LIST} "${${OUT_LIST}}" PARENT_SCOPE)
        endif()
    endfunction()

    # ====== GUARDS ====================================================================================================

    if(WIN32)
        message(FATAL_ERROR "Windows is not supported by set_lugizmo_global_build_flags().")
    endif()

    # ====== DEFAULTS ==================================================================================================

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LUGIZMO_DF_ENABLE_WARNINGS      ON)
        set(LUGIZMO_DF_STRICT_MODE          ON)
        set(LUGIZMO_DF_ENABLE_OPTIMIZATIONS OFF)
    endif()

    set(CXX_FLAGS "")
    set(LINK_FLAGS "")

    # ====== WARNINGS ==================================================================================================

    if(LUGIZMO_DF_ENABLE_WARNINGS)
        foreach(FLAG -Wall -Wextra -Wshadow)
            _add_flag_if_supported(CXX_FLAGS "${FLAG}")
        endforeach()

        if(LUGIZMO_DF_STRICT_MODE)
            foreach(FLAG -Wpedantic -Werror -Wconversion -Wsign-conversion)
                _add_flag_if_supported(CXX_FLAGS "${FLAG}")
            endforeach()
        endif()
    endif()

    # ====== OPTIMIZATIONS =============================================================================================

    if(LUGIZMO_DF_ENABLE_OPTIMIZATIONS AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        foreach(FLAG -O3 -march=native)
            _add_flag_if_supported(CXX_FLAGS "${FLAG}")
        endforeach()
    endif()

    # ====== SANITIZERS ================================================================================================

    if(LUGIZMO_DF_ENABLE_SANITIZERS AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
        check_cxx_compiler_flag("-fsanitize=address" HAS_FSANITIZE_ADDRESS)
        if(HAS_FSANITIZE_ADDRESS)
            _add_flag_if_supported(CXX_FLAGS "-fsanitize-address-use-after-scope")
        endif()

        foreach(FLAG -fsanitize=address -fsanitize=undefined)
            _add_flag_if_supported(CXX_FLAGS "${FLAG}")
            _add_flag_if_supported(LINK_FLAGS "${FLAG}")
        endforeach()
    endif()

    # ====== IPO =======================================================================================================

    if(LUGIZMO_DF_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)

        if(IPO_SUPPORTED)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(WARNING "LTO not supported: ${IPO_ERROR}")
        endif()
    endif()

    # ====== PLATFORM ==================================================================================================

    if(APPLE)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            if(NOT LUGIZMO_DF_STRICT_MODE)
                foreach(FLAG -fstack-protector-strong -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${FLAG}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
            else()
                foreach(FLAG -fstrict-enums -fstack-protector-all -fno-omit-frame-pointer -fno-optimize-sibling-calls -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${FLAG}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
            endif()
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            if(LUGIZMO_DF_STRICT_MODE)
                foreach(FLAG -fstack-protector-strong -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${FLAG}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE)
            endif()
        else()
            if(LUGIZMO_DF_STRICT_MODE)
                foreach(FLAG -fstack-protector-strong -stdlib=libc++)
                    _add_flag_if_supported(CXX_FLAGS "${FLAG}")
                endforeach()
                add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        _add_flag_if_supported(CXX_FLAGS -fPIC)
    endif()

    # ====== EXPORT ====================================================================================================

    set(LUGIZMO_DF_CXX_FLAGS  "${CXX_FLAGS}"  PARENT_SCOPE)
    set(LUGIZMO_DF_LINK_FLAGS "${LINK_FLAGS}" PARENT_SCOPE)

    list(JOIN CXX_FLAGS " " CXX_FLAGS_STR)
    list(JOIN LINK_FLAGS " " LINK_FLAGS_STR)
    message(STATUS "Lugizmo DF - build flags prepared (per-target):")
    message(STATUS "  CXX_FLAGS: ${CXX_FLAGS_STR}")
    message(STATUS "  LINK_FLAGS: ${LINK_FLAGS_STR}")

endfunction()
