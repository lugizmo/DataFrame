# Filename: fetch_benchmark.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

if(NOT LUGIZMO_DF_BUILD_BENCHMARKS)
    return()
endif()

# ====== SETTINGS ======================================================================================================

set(LUGIZMO_DF_BENCHMARK_MINIMUM_VERSION "1.9")
set(LUGIZMO_DF_BENCHMARK_FETCH_VERSION   "v1.9.5")

# ====== PROVIDED TARGETS ==============================================================================================

if(TARGET benchmark::benchmark)
    message(STATUS "Lugizmo DF - Using provided Google Benchmark targets.")
    return()
endif()

if(TARGET benchmark)
    add_library(benchmark::benchmark ALIAS benchmark)

    if(TARGET benchmark_main)
        add_library(benchmark::benchmark_main ALIAS benchmark_main)
    endif()

    message(STATUS "Lugizmo DF - Using provided Google Benchmark targets.")
    return()
endif()

# ====== LOCAL PACKAGE =================================================================================================

if(NOT LUGIZMO_DF_DOWNLOAD_BENCHMARK)
    message(STATUS "Lugizmo DF - Checking for installed Google Benchmark...")
    find_package(benchmark ${LUGIZMO_DF_BENCHMARK_MINIMUM_VERSION} QUIET CONFIG)
endif()

if(TARGET benchmark::benchmark)
    message(STATUS "Lugizmo DF - Using installed Google Benchmark ${benchmark_VERSION}.")
    return()
endif()

# ====== FETCH CONTENT ================================================================================================

if(NOT LUGIZMO_DF_DOWNLOAD_BENCHMARK)
    message(FATAL_ERROR
            "Lugizmo DF - Google Benchmark not found. "
            "Provide benchmark::benchmark from a parent project, install Google Benchmark >= "
            "${LUGIZMO_DF_BENCHMARK_MINIMUM_VERSION}, or enable LUGIZMO_DF_DOWNLOAD_BENCHMARK.")
endif()

message(STATUS "Lugizmo DF - Fetching Google Benchmark...")
include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark self tests." FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install rules." FORCE)

FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG ${LUGIZMO_DF_BENCHMARK_FETCH_VERSION}
)

FetchContent_MakeAvailable(benchmark)
