# Filename: CMakeLists.txt
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

if(LUGIZMO_BUILD_TESTING)

    add_executable(DataFrameTest
            container/ConceptsTest.cpp
            container/DataFrameMapTest.cpp
            dataframe/IndexBaseTest.cpp
            dataframe/IndexRangeTest.cpp
            dataframe/IndexUniqueTest.cpp
            dataframe/LayoutRowMajorTest.cpp
            DataframeTest.cpp
            DataframeRangeIndexTest.cpp
            DataFrameConstructRowMajorTest.cpp
            DataFrameIterateRowMajorTest.cpp
            DataFrameAccessRowMajorTest.cpp

            # row major
            RM_DataframeAccessTest.cpp
            RM_DataframeMutatingTest.cpp
            RM_DataframeViewsTest.cpp
            RM_DataframePropertiesTest.cpp

            main.cpp
    )

    target_sources(DataFrameTest PUBLIC
            RM_DataframeTestData.h
    )

    target_link_libraries(DataFrameTest PUBLIC DataFrame gtest)

endif(LUGIZMO_BUILD_TESTING)
