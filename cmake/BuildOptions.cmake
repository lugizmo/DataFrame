# Filename: BuildOptions.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

# Platform scripts.
include(${CMAKE_CURRENT_LIST_DIR}/BuildOptionsApple.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/BuildOptionsLinux.cmake)

# Internal helper target that carries developer-only compile and link flags.
set(LUGIZMO_DF_BUILD_OPTIONS_TARGET "lu_dataframe_build_options")

#
# Appends a compiler flag to the named list variable only when the active compiler
# accepts it. The caller passes a variable name, not the list contents.
#
function(_lugizmo_df_add_flag_if_supported OUT_LIST FLAG)
    string(REPLACE "-" "_" FLAG_ID "${FLAG}")
    string(REPLACE "=" "_" FLAG_ID "${FLAG_ID}")
    check_cxx_compiler_flag("${FLAG}" "HAS_${FLAG_ID}")

    if(HAS_${FLAG_ID})
        list(APPEND ${OUT_LIST} "${FLAG}")
        set(${OUT_LIST} "${${OUT_LIST}}" PARENT_SCOPE)
    endif()
endfunction()

#
# Appends a compiler flag only for matching build configurations. CONDITION must
# be a generator expression condition such as $<CONFIG:Debug>.
#
function(_lugizmo_df_add_flag_if_supported_for_cond OUT_LIST FLAG CONDITION)
    string(REPLACE "-" "_" FLAG_ID "${FLAG}")
    string(REPLACE "=" "_" FLAG_ID "${FLAG_ID}")
    check_cxx_compiler_flag("${FLAG}" "HAS_${FLAG_ID}")

    if(HAS_${FLAG_ID})
        list(APPEND ${OUT_LIST} "$<${CONDITION}:${FLAG}>")
        set(${OUT_LIST} "${${OUT_LIST}}" PARENT_SCOPE)
    endif()
endfunction()

#
# Prints a flag list in a more readable way for single-config generators by
# resolving the config-gated generator expressions used in this project.
#
function(_lugizmo_df_log_flags TITLE FLAG_LIST)
    message(STATUS "  ${TITLE}:")

    if(NOT FLAG_LIST)
        message(STATUS "    <none>")
        return()
    endif()

    foreach(FLAG IN LISTS FLAG_LIST)
        set(LUGIZMO_DF_LOG_FLAG "${FLAG}")

        if(CMAKE_BUILD_TYPE)
            if(FLAG MATCHES "^\\$<\\$<CONFIG:Debug>:(.+)>$")
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    set(LUGIZMO_DF_LOG_FLAG "${CMAKE_MATCH_1}")
                else()
                    set(LUGIZMO_DF_LOG_FLAG "")
                endif()
            elseif(FLAG MATCHES "^\\$<\\$<CONFIG:RelWithDebInfo>:(.+)>$")
                if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
                    set(LUGIZMO_DF_LOG_FLAG "${CMAKE_MATCH_1}")
                else()
                    set(LUGIZMO_DF_LOG_FLAG "")
                endif()
            elseif(FLAG MATCHES "^\\$<\\$<NOT:\\$<CONFIG:Debug>>:(.+)>$")
                if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
                    set(LUGIZMO_DF_LOG_FLAG "${CMAKE_MATCH_1}")
                else()
                    set(LUGIZMO_DF_LOG_FLAG "")
                endif()
            elseif(FLAG MATCHES "^\\$<\\$<NOT:\\$<OR:\\$<CONFIG:Debug>,\\$<CONFIG:RelWithDebInfo>>>:(.+)>$")
                if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
                    set(LUGIZMO_DF_LOG_FLAG "${CMAKE_MATCH_1}")
                else()
                    set(LUGIZMO_DF_LOG_FLAG "")
                endif()
            endif()
        endif()

        if(LUGIZMO_DF_LOG_FLAG)
            message(STATUS "    ${LUGIZMO_DF_LOG_FLAG}")
        endif()
    endforeach()
endfunction()

#
# Prepares the internal build-options target from the user-facing root options.
# This function intentionally consumes option values only; it does not rewrite them.
#
function(lugizmo_df_prepare_build_options)

    # ====== FLAG CHECK ================================================================================================

    include(CheckCXXCompilerFlag)

    # ====== GUARDS ====================================================================================================

    if(TARGET ${LUGIZMO_DF_BUILD_OPTIONS_TARGET})
        return()
    endif()

    if(WIN32)
        message(FATAL_ERROR "Windows is not supported by lugizmo_df_prepare_build_options().")
    endif()

    # Local accumulators for the internal helper target.
    set(LUGIZMO_DF_LOCAL_CXX_FLAGS "")
    set(LUGIZMO_DF_LOCAL_LINK_FLAGS "")
    set(LUGIZMO_DF_LOCAL_DEFINITIONS "")
    set(LUGIZMO_DF_LOCAL_IPO_ENABLED OFF)

    # ====== WARNINGS ==================================================================================================

    if(LUGIZMO_DF_ENABLE_WARNINGS)
        foreach(FLAG -Wall -Wextra -Wshadow)
            _lugizmo_df_add_flag_if_supported(LUGIZMO_DF_LOCAL_CXX_FLAGS "${FLAG}")
        endforeach()

        if(LUGIZMO_DF_STRICT_MODE)
            foreach(FLAG -Wpedantic -Werror -Wconversion -Wsign-conversion)
                _lugizmo_df_add_flag_if_supported(LUGIZMO_DF_LOCAL_CXX_FLAGS "${FLAG}")
            endforeach()
        endif()
    endif()

    # ====== OPTIMIZATIONS =============================================================================================

    # Compiler optimization level.
    if(LUGIZMO_DF_ENABLE_OPTIMIZATIONS)
        foreach(FLAG -O3)
            _lugizmo_df_add_flag_if_supported_for_cond(LUGIZMO_DF_LOCAL_CXX_FLAGS "${FLAG}" "$<NOT:$<CONFIG:Debug>>")
        endforeach()
    endif()

    # Machine-specific tuning.
    if(LUGIZMO_DF_ENABLE_NATIVE_OPTIMIZATIONS)
        foreach(FLAG -march=native)
            _lugizmo_df_add_flag_if_supported_for_cond(LUGIZMO_DF_LOCAL_CXX_FLAGS "${FLAG}" "$<NOT:$<CONFIG:Debug>>")
        endforeach()
    endif()

    # Link time optimizations.
    if(LUGIZMO_DF_ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)

        if(IPO_SUPPORTED)
            set(LUGIZMO_DF_LOCAL_IPO_ENABLED ON)
        else()
            message(WARNING "LTO not supported: ${IPO_ERROR}")
        endif()
    endif()

    # ====== SANITIZERS ================================================================================================

    if(LUGIZMO_DF_ENABLE_SANITIZERS)
        check_cxx_compiler_flag("-fsanitize=address" HAS_FSANITIZE_ADDRESS)
        if(HAS_FSANITIZE_ADDRESS)
            _lugizmo_df_add_flag_if_supported(LUGIZMO_DF_LOCAL_CXX_FLAGS "-fsanitize-address-use-after-scope")
        endif()

        foreach(FLAG -fsanitize=address -fsanitize=undefined)
            _lugizmo_df_add_flag_if_supported(LUGIZMO_DF_LOCAL_CXX_FLAGS "${FLAG}")
            _lugizmo_df_add_flag_if_supported(LUGIZMO_DF_LOCAL_LINK_FLAGS "${FLAG}")
        endforeach()
    endif()

    # ====== PLATFORM ==================================================================================================

    # Platform-specific options.
    if(APPLE)
        lugizmo_df_apply_apple_build_options(LUGIZMO_DF_LOCAL_CXX_FLAGS LUGIZMO_DF_LOCAL_DEFINITIONS)
    elseif(UNIX AND NOT APPLE)
        lugizmo_df_apply_linux_build_options(LUGIZMO_DF_LOCAL_CXX_FLAGS LUGIZMO_DF_LOCAL_DEFINITIONS)
    endif()

    # ====== TARGET ====================================================================================================

    add_library(${LUGIZMO_DF_BUILD_OPTIONS_TARGET} INTERFACE)

    if(LUGIZMO_DF_LOCAL_CXX_FLAGS)
        target_compile_options(${LUGIZMO_DF_BUILD_OPTIONS_TARGET} INTERFACE ${LUGIZMO_DF_LOCAL_CXX_FLAGS})
    endif()

    if(LUGIZMO_DF_LOCAL_LINK_FLAGS)
        target_link_options(${LUGIZMO_DF_BUILD_OPTIONS_TARGET} INTERFACE ${LUGIZMO_DF_LOCAL_LINK_FLAGS})
    endif()

    if(LUGIZMO_DF_LOCAL_DEFINITIONS)
        target_compile_definitions(${LUGIZMO_DF_BUILD_OPTIONS_TARGET} INTERFACE ${LUGIZMO_DF_LOCAL_DEFINITIONS})
    endif()

    set_property(GLOBAL PROPERTY LUGIZMO_DF_IPO_ENABLED ${LUGIZMO_DF_LOCAL_IPO_ENABLED})

    # ====== STATUS ====================================================================================================

    message(STATUS "Lugizmo DF - build flags prepared (per-target):")
    message(STATUS "(Note: applied to internal developer targets such as tests and benchmarks.)")
    _lugizmo_df_log_flags("CXX_FLAGS" "${LUGIZMO_DF_LOCAL_CXX_FLAGS}")
    _lugizmo_df_log_flags("LINK_FLAGS" "${LUGIZMO_DF_LOCAL_LINK_FLAGS}")
    message(STATUS "  ADDITIONAL_FLAGS:")
    if(LUGIZMO_DF_LOCAL_IPO_ENABLED)
        message(STATUS "    IPO/LTO")
    else()
        message(STATUS "    <none>")
    endif()

endfunction(lugizmo_df_prepare_build_options)

#
# Applies the prepared internal build-options target to a concrete target
# such as tests or benchmarks.
#
function(lugizmo_df_apply_build_options TARGET_NAME)
    if(NOT TARGET ${LUGIZMO_DF_BUILD_OPTIONS_TARGET})
        set(ERR_MSG "lugizmo_df_apply_build_options(${TARGET_NAME}) called before lugizmo_df_prepare_build_options().")
        message(FATAL_ERROR ${ERR_MSG})
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE ${LUGIZMO_DF_BUILD_OPTIONS_TARGET})
    get_property(LUGIZMO_DF_LOCAL_IPO_ENABLED GLOBAL PROPERTY LUGIZMO_DF_IPO_ENABLED)

    if(LUGIZMO_DF_LOCAL_IPO_ENABLED)
        set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction(lugizmo_df_apply_build_options)
