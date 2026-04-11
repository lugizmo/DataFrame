# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

if(LUGIZMO_DF_BUILD_BENCHMARKS)

    add_executable(LuDataFrameBenchmarks
            concepts/ViewTypesBenchmark.cpp
            main.cpp
    )

    target_link_libraries(LuDataFrameBenchmarks PRIVATE Lugizmo::DataFrame benchmark::benchmark)

endif()
