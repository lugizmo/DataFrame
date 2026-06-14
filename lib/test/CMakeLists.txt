# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

if(LUGIZMO_DF_BUILD_TESTING)

    add_executable(LuDataFrameTest
            container/ConceptsTest.cpp
            container/OptionalRefTest.cpp
            dataframe/IndexBaseTest.cpp
            dataframe/IndexRangeTest.cpp
            dataframe/IndexUniqueTest.cpp
            dataframe/LayoutRowMajorTest.cpp
            #dataframe/DFViewTest.cpp
            meta/DeducingThisTest.cpp
            DataframeTest.cpp
            DataframeRangeIndexTest.cpp
            DataFrameConstructRowMajorTest.cpp
            DataFrameIterateRowMajorTest.cpp
            DataFrameAccessRowMajorTest.cpp

            # memory
            memory/MemoryTest.cpp

            # row major
            RM_DataframeTorsTest.cpp
            RM_DataframeAddingTest.cpp
            RM_DataframeAccessTest.cpp
            RM_DataframeMutatingTest.cpp
            RM_DataframeViewsTest.cpp
            RM_DataframePropertiesTest.cpp
            RM_DataframeUserTypesTest.cpp
            RM_DataframeDropTest.cpp
            RM_DataframeSortTest.cpp
            #RM_DataframeTypedExampleTest.cpp

            main.cpp
    )

    target_sources(LuDataFrameTest PUBLIC
            RM_DataframeTestData.h
            RM_DataframeTestConfigs.h
    )

    target_link_libraries(LuDataFrameTest PUBLIC Lugizmo::DataFrame GTest::gtest)
    lugizmo_df_apply_build_options(LuDataFrameTest)

    add_test(NAME lugizmo_dataframe_tests COMMAND LuDataFrameTest)

endif()
