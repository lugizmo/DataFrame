# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

cmake_minimum_required(VERSION 3.27)
project(LuDataFramePackageSmoke LANGUAGES CXX)

find_package(LuDataFrame CONFIG REQUIRED)

add_executable(lugizmo_dataframe_package_smoke main.cpp)

target_compile_features(lugizmo_dataframe_package_smoke PRIVATE cxx_std_23)
target_link_libraries(lugizmo_dataframe_package_smoke PRIVATE  Lugizmo::DataFrame)

include(CTest)
add_test(NAME lugizmo_dataframe_package_smoke COMMAND lugizmo_dataframe_package_smoke)
