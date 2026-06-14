// Filename: RM_DataframeSortTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions sorting fields and records. Sorting reorders a value (unique) index and permutes
// the stored columns/rows to match; it requires a value index (`requires DFUnqIndex`), so this suite
// uses unique indices and is not parameterized over index configurations.
// The following functions are tested here (with names of tests):
//
// ✅ SortFields
// - SortFields(Compare comp = {}) requires DFUnqIndex<FldI>
//
// ✅ SortRecords
// - SortRecords(Compare comp = {}) requires DFUnqIndex<RecI>
//

#include "gtest/gtest.h"

#include <array>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  @brief SortFields reorders the field index and carries each column's values along.
 *  @see   lugizmo::DataFrame.SortFields(Compare comp = {})
 */
TEST(RM_DataframeSort, SortFields)
{
    using namespace lugizmo;

    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 3>{"field_b", "field_a", "field_c"}), 3);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"record_1", "record_2"}), 2);

    ASSERT_TRUE(df.AssignValue("field_b", "record_1", 10));
    ASSERT_TRUE(df.AssignValue("field_a", "record_1", 20));
    ASSERT_TRUE(df.AssignValue("field_c", "record_1", 30));
    ASSERT_TRUE(df.AssignValue("field_b", "record_2", 40));
    ASSERT_TRUE(df.AssignValue("field_a", "record_2", 50));
    ASSERT_TRUE(df.AssignValue("field_c", "record_2", 60));

    {
        // default comparator sorts ascending; each field keeps its values
        df.SortFields();

        ASSERT_EQ(df.Fields().size(), 3);
        EXPECT_EQ(df.Fields()[0], "field_a");
        EXPECT_EQ(df.Fields()[1], "field_b");
        EXPECT_EQ(df.Fields()[2], "field_c");

        EXPECT_EQ(*df.GetValue("field_a", "record_1"), 20);
        EXPECT_EQ(*df.GetValue("field_b", "record_1"), 10);
        EXPECT_EQ(*df.GetValue("field_c", "record_1"), 30);
        EXPECT_EQ(*df.GetValue("field_a", "record_2"), 50);
        EXPECT_EQ(*df.GetValue("field_b", "record_2"), 40);
        EXPECT_EQ(*df.GetValue("field_c", "record_2"), 60);
    }

    {
        // a custom comparator sorts descending; values still track the field order
        df.SortFields([](std::string const& lhs, std::string const& rhs) { return lhs > rhs; });

        ASSERT_EQ(df.Fields().size(), 3);
        EXPECT_EQ(df.Fields()[0], "field_c");
        EXPECT_EQ(df.Fields()[1], "field_b");
        EXPECT_EQ(df.Fields()[2], "field_a");

        EXPECT_EQ(*df.GetValue("field_c", "record_1"), 30);
        EXPECT_EQ(*df.GetValue("field_b", "record_1"), 10);
        EXPECT_EQ(*df.GetValue("field_a", "record_1"), 20);
    }
}

/**
 *  @brief SortRecords reorders the record index and carries each row's values along.
 *  @see   lugizmo::DataFrame.SortRecords(Compare comp = {})
 */
TEST(RM_DataframeSort, SortRecords)
{
    using namespace lugizmo;

    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"field_1", "field_2"}), 2);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 3>{"record_b", "record_a", "record_c"}), 3);

    ASSERT_TRUE(df.AssignValue("field_1", "record_b", 10));
    ASSERT_TRUE(df.AssignValue("field_2", "record_b", 20));
    ASSERT_TRUE(df.AssignValue("field_1", "record_a", 30));
    ASSERT_TRUE(df.AssignValue("field_2", "record_a", 40));
    ASSERT_TRUE(df.AssignValue("field_1", "record_c", 50));
    ASSERT_TRUE(df.AssignValue("field_2", "record_c", 60));

    {
        // default comparator sorts ascending; each record keeps its values
        df.SortRecords();

        ASSERT_EQ(df.Records().size(), 3);
        EXPECT_EQ(df.Records()[0], "record_a");
        EXPECT_EQ(df.Records()[1], "record_b");
        EXPECT_EQ(df.Records()[2], "record_c");

        EXPECT_EQ(*df.GetValue("field_1", "record_a"), 30);
        EXPECT_EQ(*df.GetValue("field_1", "record_b"), 10);
        EXPECT_EQ(*df.GetValue("field_1", "record_c"), 50);
        EXPECT_EQ(*df.GetValue("field_2", "record_a"), 40);
        EXPECT_EQ(*df.GetValue("field_2", "record_b"), 20);
        EXPECT_EQ(*df.GetValue("field_2", "record_c"), 60);
    }

    {
        // a custom comparator sorts descending; values still track the record order
        df.SortRecords([](std::string const& lhs, std::string const& rhs) { return lhs > rhs; });

        ASSERT_EQ(df.Records().size(), 3);
        EXPECT_EQ(df.Records()[0], "record_c");
        EXPECT_EQ(df.Records()[1], "record_b");
        EXPECT_EQ(df.Records()[2], "record_a");

        EXPECT_EQ(*df.GetValue("field_1", "record_c"), 50);
        EXPECT_EQ(*df.GetValue("field_1", "record_b"), 10);
        EXPECT_EQ(*df.GetValue("field_1", "record_a"), 30);
    }
}
