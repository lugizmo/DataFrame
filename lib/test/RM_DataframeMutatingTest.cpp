// Filename: RM_DataframeMutatingTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions mutating values in the dataframe.
// The following functions are tested here (with names of tests):
//
// ✅ replace_val
// - Replace(FldT const& field, RecT const& record, T const& value) -> bool
//

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include "RM_DataframeTestData.h"

/**
 *  @brief Setting individual values from the dataframe.
 *  @see   lugizmo::Dataframe.Replace(FldT const& field, RecT const& record, T const& value) -> bool
 */
TEST(lugizmo_dataframe_mutating_row_major, replace_val)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;
        auto df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const recName : DFRecords)
            {

                df.Replace(fldName, recName, DFData[recCount][fldCount] * 2);
                recCount++;
            }
            fldCount++;
        }

        fldCount = 0;
        for(auto const fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const recName : DFRecords)
            {
                auto const val = df.GetValue(fldName, recName);
                ASSERT_TRUE(val.has_value());
                EXPECT_EQ(*val, DFData[recCount][fldCount] * 2);
                recCount++;
            }
            fldCount++;
        }
    }

    {
        using namespace lugizmo::test::str;
        auto df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const& recName : DFRecords)
            {

                df.Replace(fldName, recName, DFData[recCount][fldCount] * 2);
                recCount++;
            }
            fldCount++;
        }

        fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const& recName : DFRecords)
            {
                auto const val = df.GetValue(fldName, recName);
                ASSERT_TRUE(val.has_value());
                EXPECT_EQ(*val, DFData[recCount][fldCount] * 2);
                recCount++;
            }
            fldCount++;
        }
    }
}