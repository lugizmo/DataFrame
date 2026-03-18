// Filename: RM_DataframeDropTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <array>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  @brief Dropping fields and records preserves the order of the remaining indices.
 *  @see   lugizmo::DataFrame.DropField(F const& index) -> bool requires DFValIndex<FldI>
 *  @see   lugizmo::DataFrame.DropRecord(R const& index) -> bool requires DFValIndex<RecI>
 */
TEST(lugizmo_dataframe_dropping_row_major, drop_field_and_record_order)
{
    using namespace lugizmo;

    auto df = DataFrame<int, std::string, std::string>();

    ASSERT_EQ(df.AddFields(std::array<std::string, 4>{"field1", "field2", "field3", "field4"}), 4);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 4>{"record1", "record2", "record3", "record4"}), 4);

    ASSERT_TRUE(df.DropField("field2"));
    ASSERT_TRUE(df.DropRecord("record2"));

    ASSERT_EQ(df.Fields().size(), 3);
    EXPECT_EQ(df.Fields()[0], "field1");
    EXPECT_EQ(df.Fields()[1], "field3");
    EXPECT_EQ(df.Fields()[2], "field4");

    ASSERT_EQ(df.Records().size(), 3);
    EXPECT_EQ(df.Records()[0], "record1");
    EXPECT_EQ(df.Records()[1], "record3");
    EXPECT_EQ(df.Records()[2], "record4");
}

/**
 *  @brief Re-adding a dropped field or record appends it to the end of the current storage order.
 *  @see   lugizmo::DataFrame.DropField(F const& index) -> bool requires DFValIndex<FldI>
 *  @see   lugizmo::DataFrame.DropRecord(R const& index) -> bool requires DFValIndex<RecI>
 *  @see   lugizmo::DataFrame.AddField(F index, T const& defaultValue = T{}) -> bool requires DFValIndex<FldI>
 *  @see   lugizmo::DataFrame.AddRecord(R index, T const& defaultValue = T{}) -> bool requires DFValIndex<RecI>
 */
TEST(lugizmo_dataframe_dropping_row_major, reinsert_after_drop_appends_to_end)
{
    using namespace lugizmo;

    auto df = DataFrame<int, std::string, std::string>();

    ASSERT_EQ(df.AddFields(std::array<std::string, 4>{"field1", "field2", "field3", "field4"}), 4);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 4>{"record1", "record2", "record3", "record4"}), 4);

    ASSERT_TRUE(df.DropField("field2"));
    ASSERT_TRUE(df.DropRecord("record2"));

    ASSERT_TRUE(df.AddField("field2"));
    ASSERT_TRUE(df.AddRecord("record2"));

    ASSERT_EQ(df.Fields().size(), 4);
    EXPECT_EQ(df.Fields()[0], "field1");
    EXPECT_EQ(df.Fields()[1], "field3");
    EXPECT_EQ(df.Fields()[2], "field4");
    EXPECT_EQ(df.Fields()[3], "field2");

    ASSERT_EQ(df.Records().size(), 4);
    EXPECT_EQ(df.Records()[0], "record1");
    EXPECT_EQ(df.Records()[1], "record3");
    EXPECT_EQ(df.Records()[2], "record4");
    EXPECT_EQ(df.Records()[3], "record2");
}
