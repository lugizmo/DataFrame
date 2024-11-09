# Filename: CMakeLists.txt
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

if(OPT_BUILD_BENCH)

    add_executable(DataFrameBenchmarks

            concepts/ViewTypesBenchmark.cpp

            main.cpp
    )

    target_link_libraries(DataFrameBenchmarks PRIVATE benchmark::benchmark DataFrame)

endif(OPT_BUILD_BENCH)