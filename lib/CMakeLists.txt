# Filename: CMakeLists.txt
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

set(LUGIZMO_DF_INCLUDES
        include/lugizmo/memory/Memory.h

        include/lugizmo/container/DataFrameMap.h

        include/lugizmo/dataframe/Layout.h
        include/lugizmo/dataframe/Index.h

        include/lugizmo/DataFrame.h
)

set(LUGIZMO_DF_SOURCES
        src/DataFrame.cpp
)

add_library(DataFrame ${LUGIZMO_DF_SOURCES})
target_sources(DataFrame PUBLIC ${LUGIZMO_DF_INCLUDES})

set_target_properties(DataFrame PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON)

target_link_libraries(DataFrame
        PRIVATE
        ${LUGIZMO_DF_PROJECT_NAME}_project_options
        ${LUGIZMO_DF_PROJECT_NAME}_project_warnings)

target_include_interface_directories(DataFrame "${CMAKE_CURRENT_SOURCE_DIR}/include/")






add_executable(DataFrameTest test/main.cpp test/DataframeTest.cpp)
target_link_libraries(DataFrameTest PUBLIC DataFrame gtest)