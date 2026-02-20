# Filename: CMakeLists.txt
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

cmake_minimum_required(VERSION 3.27)
set(LUGIZMO_DF_PROJECT_NAME DataFrame)
set(CMAKE_CXX_STANDARD 23)

project(${LUGIZMO_DF_PROJECT_NAME}
        VERSION 0.0.1
        DESCRIPTION "DataFrame type to store tabular data."
        LANGUAGES CXX)

# ====== OPTIONS ===========================================================================================================================

option(BUILD_TESTING    "Build tests."      ON)
option(BUILD_BENCHMARKS "Build benchmarks." OFF)

option(LUGIZMO_DOWNLOAD_GTEST     "Download GTest regardless of local installation."     OFF)
option(LUGIZMO_DOWNLOAD_BENCHMARK "Download Benchmark regardless of local installation." OFF)

option(LUGIZMO_USE_CLANG_TIDY "Enable Clang-Tidy analysis during builds." OFF)

set(LUGIZMO_BUILD_TESTING ${BUILD_TESTING})
set(LUGIZMO_BUILD_BENCH ${BUILD_BENCHMARKS})

# apply build flags based on options
include(cmake/BuildOptions.cmake)
include(cmake/ClangTidy.cmake)

set_lugizmo_global_build_flags()

message(STATUS "Lugizmo DF - Building tests: ${BUILD_TESTING}")
message(STATUS "Lugizmo DF - Building benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "Lugizmo DF - Using Clang-Tidy: ${LUGIZMO_USE_CLANG_TIDY}")

# ====== CONDITIONS ===========================================================================================================================

if(NOT (CMAKE_SIZEOF_VOID_P GREATER_EQUAL 8))
    message(FATAL_ERROR "Currently dataframe does only support >= 64bit systems.")
endif()

# ====== DEPENDENCIES ======================================================================================================================

# Fetch dependencies: Ensures that gtest and benchmark are downloaded automatically
# System-installed dependencies can also be used if required.
include(vendor/fetch_gtest.cmake)
include(vendor/fetch_benchmark.cmake)

# ====== SOURCE ============================================================================================================================

add_subdirectory(lib)
add_subdirectory(lib/test)
add_subdirectory(lib/bench)

# ====== LLDB FORMATTERS ==================================================================================================================

#add_custom_target(lugizmo-lldb-formatters-dataframe
#    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/llvm_format.cmake
#    COMMENT "Install LLDB formatters to ~/.lldb/lugizmo and update ~/.lldbinit"
#)
