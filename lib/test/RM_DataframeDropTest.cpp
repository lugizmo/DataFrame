// Filename: RM_DataframeDropTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test dropping fields and records. DropField/DropRecord require a value (unique) index
// (a range index cannot drop a middle element without losing contiguity), so this suite is
// not parameterized over index configurations.
// The following functions are tested here (with names of tests):
//
// ✅ DropField
// - DropField(F const& index) -> bool requires DFUnqIndex<FldI>
//
// ✅ DropRecord
// - DropRecord(R const& index) -> bool requires DFUnqIndex<RecI>
//

#include "gtest/gtest.h"

#include <array>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  @brief Dropping a field removes its column, preserves the remaining order and data, and
 *         re-adding appends to the end.
 *  @see   lugizmo::DataFrame.DropField(F const& index) -> bool requires DFUnqIndex<FldI>
 */
TEST(RM_DataframeDrop, DropField)
{
    using namespace lugizmo;

    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 4>{"f0", "f1", "f2", "f3"}), 4);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"r0", "r1"}), 2);

    // value at (field f, record r) = r * 4 + f
    for (int r = 0; r < 2; ++r)
    {
        for (int f = 0; f < 4; ++f)
        {
            df.AssignValue("f" + std::to_string(f), "r" + std::to_string(r), r * 4 + f);
        }
    }

    {
        // dropping a middle field removes its column and preserves order + data of the rest
        ASSERT_TRUE(df.DropField("f1"));

        ASSERT_EQ(df.Fields().size(), 3);
        EXPECT_EQ(df.Fields()[0], "f0");
        EXPECT_EQ(df.Fields()[1], "f2");
        EXPECT_EQ(df.Fields()[2], "f3");

        EXPECT_EQ((df["f0", "r0"]), 0);
        EXPECT_EQ((df["f2", "r1"]), 6);
        EXPECT_EQ((df["f3", "r1"]), 7);
    }

    {
        // re-adding a dropped field appends it to the end
        ASSERT_TRUE(df.AddField("f1"));

        ASSERT_EQ(df.Fields().size(), 4);
        EXPECT_EQ(df.Fields()[3], "f1");
    }

    {
        // dropping a missing field returns false and leaves the frame unchanged
        EXPECT_FALSE(df.DropField("missing"));
        EXPECT_EQ(df.Fields().size(), 4);
    }
}

/**
 *  @brief Dropping a record removes its row, preserves the remaining order and data, and
 *         re-adding appends to the end.
 *  @see   lugizmo::DataFrame.DropRecord(R const& index) -> bool requires DFUnqIndex<RecI>
 */
TEST(RM_DataframeDrop, DropRecord)
{
    using namespace lugizmo;

    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"f0", "f1"}), 2);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 4>{"r0", "r1", "r2", "r3"}), 4);

    // value at (field f, record r) = r * 2 + f
    for (int r = 0; r < 4; ++r)
    {
        for (int f = 0; f < 2; ++f)
        {
            df.AssignValue("f" + std::to_string(f), "r" + std::to_string(r), r * 2 + f);
        }
    }

    {
        // dropping a middle record removes its row and preserves order + data of the rest
        ASSERT_TRUE(df.DropRecord("r1"));

        ASSERT_EQ(df.Records().size(), 3);
        EXPECT_EQ(df.Records()[0], "r0");
        EXPECT_EQ(df.Records()[1], "r2");
        EXPECT_EQ(df.Records()[2], "r3");

        EXPECT_EQ((df["f0", "r2"]), 4);
        EXPECT_EQ((df["f1", "r3"]), 7);
    }

    {
        // re-adding a dropped record appends it to the end
        ASSERT_TRUE(df.AddRecord("r1"));

        ASSERT_EQ(df.Records().size(), 4);
        EXPECT_EQ(df.Records()[3], "r1");
    }

    {
        // dropping a missing record returns false and leaves the frame unchanged
        EXPECT_FALSE(df.DropRecord("missing"));
        EXPECT_EQ(df.Records().size(), 4);
    }
}
