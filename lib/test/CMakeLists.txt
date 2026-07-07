# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

if(LUGIZMO_DF_BUILD_TESTING)

    add_executable(LuDataFrameTest
            internal/ConceptsTest.cpp
            internal/DeducingThisTest.cpp
            internal/MemoryTest.cpp
            index/IndexBaseTest.cpp
            index/IndexRangeTest.cpp
            index/IndexUniqueTest.cpp
            storage/LayoutRowMajorTest.cpp
            view/SelectorTest.cpp
            view/SliceTest.cpp
            view/SpanTest.cpp
            view/ViewTest.cpp
            view/ViewIndexedTest.cpp
            view/RangeCompositionTest.cpp

            algorithm/ForEachTest.cpp

            core/DataFrameConstructTest.cpp
            core/DataFrameTorsTest.cpp
            core/DataFrameAddingTest.cpp
            core/DataFrameAccessTest.cpp
            core/DataFrameMutatingTest.cpp
            core/DataFrameViewsTest.cpp
            core/DataFramePropertiesTest.cpp
            core/DataFrameUserTypesTest.cpp
            core/DataFrameDropTest.cpp
            core/DataFrameSortTest.cpp
            core/DataFrameIOTest.cpp

            # examples (usability, still asserting)
            examples/RM_DFExamples_RangeIndices.cpp

            main.cpp
    )

    target_sources(LuDataFrameTest PUBLIC
            core/HelperDFTestConfigs.h
    )

    target_link_libraries(LuDataFrameTest PUBLIC Lugizmo::DataFrame GTest::gtest)
    lugizmo_df_apply_build_options(LuDataFrameTest)

    add_test(NAME lugizmo_dataframe_tests COMMAND LuDataFrameTest)

endif()
