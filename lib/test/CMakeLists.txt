# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

if(LUGIZMO_DF_BUILD_TESTING)

    add_executable(LuDataFrameTest
            container/ConceptsTest.cpp
            dataframe/IndexBaseTest.cpp
            dataframe/IndexRangeTest.cpp
            dataframe/IndexUniqueTest.cpp
            dataframe/LayoutRowMajorTest.cpp
            dataframe/SelectorTest.cpp
            dataframe/SpanTest.cpp
            dataframe/ViewTest.cpp
            dataframe/ViewIndexedTest.cpp
            meta/DeducingThisTest.cpp

            # memory
            memory/MemoryTest.cpp

            # row major
            RM_DataFrameConstructTest.cpp
            RM_DataFrameTorsTest.cpp
            RM_DataFrameAddingTest.cpp
            RM_DataFrameAccessTest.cpp
            RM_DataFrameMutatingTest.cpp
            RM_DataFrameViewsTest.cpp
            RM_DataFramePropertiesTest.cpp
            RM_DataFrameUserTypesTest.cpp
            RM_DataFrameDropTest.cpp
            RM_DataFrameSortTest.cpp
            RM_DataFrameFunctionalTest.cpp
            RM_DataFrameIOTest.cpp

            # examples (usability, still asserting)
            examples/RM_DFExamples_RangeIndices.cpp

            main.cpp
    )

    target_sources(LuDataFrameTest PUBLIC
            RM_HelperDFTestConfigs.h
    )

    target_link_libraries(LuDataFrameTest PUBLIC Lugizmo::DataFrame GTest::gtest)
    lugizmo_df_apply_build_options(LuDataFrameTest)

    add_test(NAME lugizmo_dataframe_tests COMMAND LuDataFrameTest)

endif()
