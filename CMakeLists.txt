# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

cmake_minimum_required(VERSION 3.27)

project(DataFrame
        VERSION 0.0.1
        DESCRIPTION "DataFrame type to store tabular data."
        LANGUAGES CXX)

# ====== PROJECT SETTINGS ==============================================================================================

set(LUGIZMO_DF_PACKAGE_NAME "LuDataFrame")
set(LUGIZMO_DF_TARGET_NAME  "lu_dataframe")
set(LUGIZMO_DF_NAMESPACE    "Lugizmo::")
set(LUGIZMO_DF_CXX_STANDARD 23)

# ====== OPTIONS =======================================================================================================
include(cmake/BuildOptionsDefaults.cmake)

# bench and testing options
option(BUILD_TESTING                   "Build tests."                                         ${PROJECT_IS_TOP_LEVEL})
option(BUILD_BENCHMARKS                "Build benchmarks."                                    OFF)
option(LUGIZMO_DF_BUILD_TESTING        "Build Lugizmo DF tests."                              ${BUILD_TESTING})
option(LUGIZMO_DF_BUILD_BENCHMARKS     "Build Lugizmo DF benchmarks."                         ${BUILD_BENCHMARKS})
option(LUGIZMO_DF_DOWNLOAD_GTEST       "Download GTest regardless of local installation."     OFF)
option(LUGIZMO_DF_DOWNLOAD_BENCHMARK   "Download Benchmark regardless of local installation." OFF)

# assertion options
option(LUGIZMO_DF_ENABLE_ASSERT        "Enable default runtime assertions in LUGIZMO_ASSERT." ${LUGIZMO_DF_DEB_OR_RELDEB})
option(LUGIZMO_DF_ENABLE_ASSERT_TRACE  "Enable internal trace runtime assertions."            OFF)

# build options
option(LUGIZMO_DF_ENABLE_WARNINGS             "Enable warnings."                                    ON)
option(LUGIZMO_DF_ENABLE_OPTIMIZATIONS        "Enable high optimizations (O3)."                     OFF)
option(LUGIZMO_DF_ENABLE_NATIVE_OPTIMIZATIONS "Enable native CPU opt. (for example -march=native)." OFF)
option(LUGIZMO_DF_ENABLE_HARDENING            "Enable platform hardening where supported."          OFF)
option(LUGIZMO_DF_ENABLE_LTO                  "Enable link time optimization."                      OFF)
option(LUGIZMO_DF_ENABLE_PIC                  "Enable position-independent code (-fPIC) on Linux."  OFF)
option(LUGIZMO_DF_STRICT_MODE                 "Enable strict mode (pedantic, Werror, etc)."         OFF)
option(LUGIZMO_DF_USE_CLANG_TIDY              "Enable Clang-Tidy analysis during builds."           OFF)

# sanitizer options
option(LUGIZMO_DF_ENABLE_SANITIZERS "Enable sanitizers (UBSan, ASan)." OFF)

# override testing and benchmark option with lugizmo version
# this allows to turn it off even if default option is used
set(BUILD_TESTING    ${LUGIZMO_DF_BUILD_TESTING}    CACHE BOOL "Build tests."      FORCE)
set(BUILD_BENCHMARKS ${LUGIZMO_DF_BUILD_BENCHMARKS} CACHE BOOL "Build benchmarks." FORCE)

# ====== CONDITIONS ====================================================================================================

if(NOT (CMAKE_SIZEOF_VOID_P GREATER_EQUAL 8))
    message(FATAL_ERROR "Currently dataframe does only support >= 64bit systems.")
endif()

# ====== MODULES =======================================================================================================

# cmake modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

# lugizmo-df modules
include(cmake/BuildOptions.cmake)
include(cmake/ToolsClangTidy.cmake)

# ====== BUILD FLAGS ===================================================================================================

lugizmo_df_prepare_build_options()

message(STATUS "Lugizmo DF - Building tests: ${LUGIZMO_DF_BUILD_TESTING}")
message(STATUS "Lugizmo DF - Building benchmarks: ${LUGIZMO_DF_BUILD_BENCHMARKS}")
message(STATUS "Lugizmo DF - Using Clang-Tidy: ${LUGIZMO_DF_USE_CLANG_TIDY}")

# ====== DEPENDENCIES ==================================================================================================

if(LUGIZMO_DF_BUILD_TESTING)
    include(vendor/fetch_gtest.cmake)
endif()

if(LUGIZMO_DF_BUILD_BENCHMARKS)
    include(vendor/fetch_benchmark.cmake)
endif()

# ====== SOURCE ========================================================================================================

add_subdirectory(lib)

if(LUGIZMO_DF_BUILD_TESTING)
    add_subdirectory(lib/test)
endif()

if(LUGIZMO_DF_BUILD_BENCHMARKS)
    add_subdirectory(lib/bench)
endif()

# ====== PACKAGE =======================================================================================================

include(cmake/PackageProject.cmake)

# ====== EXTRAS ========================================================================================================

# lldb formatters
# add_custom_target(lugizmo-lldb-formatters-dataframe
#     COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/llvm_format.cmake
#     COMMENT "Install LLDB formatters to ~/.lldb/lugizmo and update ~/.lldbinit"
# )
