# Filename: CMakeLists.txt
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

set(LUGIZMO_DF_INCLUDES
        include/lugizmo/memory/Memory.h
        include/lugizmo/memory/References.h
#        include/lugizmo/memory/DFMemory.h

        include/lugizmo/container/Concepts.h
        include/lugizmo/container/DataFrameMap.h

        include/lugizmo/dataframe/LayoutColMajor.h
        include/lugizmo/dataframe/LayoutRowMajor.h
        include/lugizmo/dataframe/IndexBase.h
        include/lugizmo/dataframe/IndexUnique.h
        include/lugizmo/dataframe/IndexRange.h
        include/lugizmo/dataframe/Selector.h
        include/lugizmo/dataframe/View.h
        include/lugizmo/dataframe/ViewIndexed.h

        include/lugizmo/Error.h
        include/lugizmo/DataFrame.h
)

set(LUGIZMO_DF_SOURCES
        src/Error.cpp
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

add_subdirectory(bench)

# TODO move to separate CMakeLists.txt
if(OPT_BUILD_TESTS)
    add_executable(DataFrameTest

#            test/memory/DFMemoryTest.cpp

            test/container/ConceptsTest.cpp
            test/container/DataFrameMapTest.cpp

            test/dataframe/IndexBaseTest.cpp
            test/dataframe/IndexRangeTest.cpp
            test/dataframe/IndexUniqueTest.cpp
            test/dataframe/LayoutRowMajorTest.cpp

            test/DataframeTest.cpp
            test/DataframeRangeIndexTest.cpp
            test/main.cpp
    )
    target_link_libraries(DataFrameTest PUBLIC DataFrame gtest)
endif(OPT_BUILD_TESTS)