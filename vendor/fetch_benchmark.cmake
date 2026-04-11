# Filename: fetch_benchmark.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

if(LUGIZMO_DF_BUILD_BENCHMARKS)

    # Build Benchmarks
    set(LUGIZMO_DF_BENCHMARK_MINIMUM 1.9)
    set(LUGIZMO_DF_BENCHMARK_VERSION v1.9.2)

    if(NOT TARGET benchmark::benchmark AND NOT TARGET benchmark::benchmark_main)
        # Check if dependencies are present

        if(NOT LUGIZMO_DF_DOWNLOAD_BENCHMARK)
            # Find local package
            message(STATUS "Lugizmo DF - Checking for system-installed Google Benchmark...")
            find_package(benchmark ${LUGIZMO_DF_BENCHMARK_MINIMUM} EXACT QUIET)
        endif()

        if(benchmark_FOUND AND NOT LUGIZMO_DF_DOWNLOAD_BENCHMARK)
            # Found local package
            message(STATUS "Lugizmo DF - Google Benchmark ${benchmark_VERSION}")
        elseif(NOT benchmark_FOUND AND NOT LUGIZMO_DF_DOWNLOAD_BENCHMARK)
            # Couldn't find local package even though should be provided
            message(FATAL_ERROR "Lugizmo DF - Google Benchmark not found! Please install Google Benchmark >= ${LUGIZMO_DF_BENCHMARK_MINIMUM} or enable LUGIZMO_DF_DOWNLOAD_BENCHMARK to fetch it.")
        else()
            # Get benchmark lib from remote
            message(STATUS "Lugizmo DF - Fetching Google Benchmark...")
            include(FetchContent)
            FetchContent_Declare(
                    benchmark
                    GIT_REPOSITORY https://github.com/google/benchmark.git
                    GIT_TAG ${LUGIZMO_DF_BENCHMARK_VERSION}
            )
            FetchContent_MakeAvailable(benchmark)
        endif()

    else()
        # Check target properties
        get_target_property(BENCHMARK_VERSION benchmark INTERFACE_VERSION)
        if(BENCHMARK_VERSION VERSION_LESS ${LUGIZMO_DF_BENCHMARK_MINIMUM})
            message(FATAL_ERROR "Benchmark version ${BENCHMARK_VERSION} is less than required ${LUGIZMO_DF_BENCHMARK_MINIMUM}.")
        else()
            message(STATUS "Using provided Benchmark version ${BENCHMARK_VERSION}.")
        endif()
    endif()
endif()
