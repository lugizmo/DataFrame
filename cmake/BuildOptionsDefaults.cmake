# Filename: BuildOptionsDefaults.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

# Computed defaults used by the root option block.
set(LUGIZMO_DF_DEB_OR_RELDEB OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(LUGIZMO_DF_DEB_OR_RELDEB ON)
endif()
