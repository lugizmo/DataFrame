// Filename: RM_DataframeSortTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions sorting fields and records in the dataframe.
// The following functions are tested here (with names of tests):
//
// ✅ sort_fields
// - SortFields(Compare comp = {})
//
// ✅ sort_records
// - SortRecords(Compare comp = {})
//

#include "gtest/gtest.h"

#include <array>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  @brief Sorting fields reorders both the field indices and the stored column values.
 *  @see   lugizmo::DataFrame.SortFields(Compare comp = {})
 */
TEST(lugizmo_dataframe_sorting_row_major, sort_fields)
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

    df.SortFields();

    ASSERT_EQ(df.Fields().size(), 3);
    EXPECT_EQ(df.Fields()[0], "field_a");
    EXPECT_EQ(df.Fields()[1], "field_b");
    EXPECT_EQ(df.Fields()[2], "field_c");

    auto record1 = df.ViewRecordIndexed("record_1");
    ASSERT_EQ(record1.Size(), 3);
    ASSERT_NE(record1.At("field_a"), nullptr);
    ASSERT_NE(record1.At("field_b"), nullptr);
    ASSERT_NE(record1.At("field_c"), nullptr);
    EXPECT_EQ(*record1.At("field_a"), 20);
    EXPECT_EQ(*record1.At("field_b"), 10);
    EXPECT_EQ(*record1.At("field_c"), 30);

    auto record2 = df.ViewRecordIndexed("record_2");
    ASSERT_EQ(record2.Size(), 3);
    ASSERT_NE(record2.At("field_a"), nullptr);
    ASSERT_NE(record2.At("field_b"), nullptr);
    ASSERT_NE(record2.At("field_c"), nullptr);
    EXPECT_EQ(*record2.At("field_a"), 50);
    EXPECT_EQ(*record2.At("field_b"), 40);
    EXPECT_EQ(*record2.At("field_c"), 60);

    df.SortFields([](std::string const& lhs, std::string const& rhs) { return lhs > rhs; });

    ASSERT_EQ(df.Fields().size(), 3);
    EXPECT_EQ(df.Fields()[0], "field_c");
    EXPECT_EQ(df.Fields()[1], "field_b");
    EXPECT_EQ(df.Fields()[2], "field_a");

    record1 = df.ViewRecordIndexed("record_1");
    ASSERT_EQ(record1.Size(), 3);
    ASSERT_NE(record1.At("field_c"), nullptr);
    ASSERT_NE(record1.At("field_b"), nullptr);
    ASSERT_NE(record1.At("field_a"), nullptr);
    EXPECT_EQ(*record1.At("field_c"), 30);
    EXPECT_EQ(*record1.At("field_b"), 10);
    EXPECT_EQ(*record1.At("field_a"), 20);
}

/**
 *  @brief Sorting records reorders both the record indices and the stored row values.
 *  @see   lugizmo::DataFrame.SortRecords(Compare comp = {})
 */
TEST(lugizmo_dataframe_sorting_row_major, sort_records)
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

    df.SortRecords();

    ASSERT_EQ(df.Records().size(), 3);
    EXPECT_EQ(df.Records()[0], "record_a");
    EXPECT_EQ(df.Records()[1], "record_b");
    EXPECT_EQ(df.Records()[2], "record_c");

    auto field1 = df.ViewFieldIndexed("field_1");
    ASSERT_EQ(field1.Size(), 3);
    ASSERT_NE(field1.At("record_a"), nullptr);
    ASSERT_NE(field1.At("record_b"), nullptr);
    ASSERT_NE(field1.At("record_c"), nullptr);
    EXPECT_EQ(*field1.At("record_a"), 30);
    EXPECT_EQ(*field1.At("record_b"), 10);
    EXPECT_EQ(*field1.At("record_c"), 50);

    auto field2 = df.ViewFieldIndexed("field_2");
    ASSERT_EQ(field2.Size(), 3);
    ASSERT_NE(field2.At("record_a"), nullptr);
    ASSERT_NE(field2.At("record_b"), nullptr);
    ASSERT_NE(field2.At("record_c"), nullptr);
    EXPECT_EQ(*field2.At("record_a"), 40);
    EXPECT_EQ(*field2.At("record_b"), 20);
    EXPECT_EQ(*field2.At("record_c"), 60);

    df.SortRecords([](std::string const& lhs, std::string const& rhs) { return lhs > rhs; });

    ASSERT_EQ(df.Records().size(), 3);
    EXPECT_EQ(df.Records()[0], "record_c");
    EXPECT_EQ(df.Records()[1], "record_b");
    EXPECT_EQ(df.Records()[2], "record_a");

    field1 = df.ViewFieldIndexed("field_1");
    ASSERT_EQ(field1.Size(), 3);
    ASSERT_NE(field1.At("record_c"), nullptr);
    ASSERT_NE(field1.At("record_b"), nullptr);
    ASSERT_NE(field1.At("record_a"), nullptr);
    EXPECT_EQ(*field1.At("record_c"), 50);
    EXPECT_EQ(*field1.At("record_b"), 10);
    EXPECT_EQ(*field1.At("record_a"), 30);
}
