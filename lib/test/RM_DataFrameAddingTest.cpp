// Filename: RM_DataframeAddingTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions adding fields and records. AddField/AddFields/AddRecord/AddRecords require a
// value (unique) index, so this suite is not parameterized over index configurations.
// The following functions are tested here (with names of tests):
//
// ✅ AddField
// - AddField(F index, T const& defaultValue = T{}) -> bool requires DFUnqIndex<FldI>
//
// ✅ AddFields
// - AddFields(std::span<F const> indices, T const& defaultValue = T{}) -> std::size_t requires DFUnqIndex<FldI>
//
// ✅ AddRecord
// - AddRecord(R index, T const& defaultValue = T{}) -> bool requires DFUnqIndex<RecI>
//
// ✅ AddRecords
// - AddRecords(std::span<R const> indices, T const& defaultValue = T{}) -> std::size_t requires DFUnqIndex<RecI>
//
// TODO (range-family):
//   AddRecordPopulated(R index, std::span<T const> records) -> bool requires DFUnqIndex<RecI>
//   SetFieldRange / SetRecordRange (all overloads) requires DFSeqIndex
//

#include "gtest/gtest.h"

#include <array>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  @brief Adding a single field appends it (when new) and grows the value buffer once records exist.
 *  @see   lugizmo::DataFrame.AddField(F index, T const& defaultValue) -> bool requires DFUnqIndex<FldI>
 */
TEST(RM_DataframeAdding, AddField)
{
    using namespace lugizmo;
    auto df = DataFrame<int, std::string, std::string>();

    {
        // adding new fields returns true and preserves insertion order
        EXPECT_TRUE(df.AddField("field1"));
        EXPECT_TRUE(df.AddField("field2"));
        EXPECT_TRUE(df.AddField("field3"));

        ASSERT_EQ(df.FieldSize(), 3);
        EXPECT_EQ(df.Fields()[0], "field1");
        EXPECT_EQ(df.Fields()[1], "field2");
        EXPECT_EQ(df.Fields()[2], "field3");
        EXPECT_TRUE(df.Empty()); // no records yet -> no values
    }

    {
        // adding an existing field returns false and changes nothing
        EXPECT_FALSE(df.AddField("field1"));
        EXPECT_EQ(df.FieldSize(), 3);
    }

    {
        // with records present, a new field grows the value buffer
        ASSERT_TRUE(df.AddRecord("record1"));
        EXPECT_TRUE(df.AddField("field4"));
        EXPECT_EQ(df.FieldSize(), 4);
        EXPECT_EQ(df.Size(), 4 * 1);
    }
}

/**
 *  @brief Adding multiple fields returns the number inserted and appends new ones in order.
 *  @see   lugizmo::DataFrame.AddFields(std::span<F const> indices, T const& defaultValue) -> std::size_t requires DFUnqIndex<FldI>
 */
TEST(RM_DataframeAdding, AddFields)
{
    using namespace lugizmo;
    auto df = DataFrame<int, std::string, std::string>();

    {
        // bulk add returns the number inserted and appends in order
        EXPECT_EQ(df.AddFields(std::array<std::string, 3>{"field1", "field2", "field3"}), 3);
        ASSERT_EQ(df.FieldSize(), 3);
        EXPECT_EQ(df.Fields()[0], "field1");
        EXPECT_EQ(df.Fields()[2], "field3");
    }

    {
        // re-adding existing fields inserts none
        EXPECT_EQ(df.AddFields(std::array<std::string, 3>{"field1", "field2", "field3"}), 0);
        EXPECT_EQ(df.FieldSize(), 3);
    }

    {
        // a mix inserts only the new ones, appended after the existing
        EXPECT_EQ(df.AddFields(std::array<std::string, 3>{"field1", "field4", "field5"}), 2);
        ASSERT_EQ(df.FieldSize(), 5);
        EXPECT_EQ(df.Fields()[3], "field4");
        EXPECT_EQ(df.Fields()[4], "field5");
    }
}

/**
 *  @brief Adding a single record appends it (when new) and grows the value buffer once fields exist.
 *  @see   lugizmo::DataFrame.AddRecord(R index, T const& defaultValue) -> bool requires DFUnqIndex<RecI>
 */
TEST(RM_DataframeAdding, AddRecord)
{
    using namespace lugizmo;
    auto df = DataFrame<int, std::string, std::string>();

    {
        // adding new records returns true and preserves insertion order
        EXPECT_TRUE(df.AddRecord("record1"));
        EXPECT_TRUE(df.AddRecord("record2"));
        EXPECT_TRUE(df.AddRecord("record3"));

        ASSERT_EQ(df.RecordSize(), 3);
        EXPECT_EQ(df.Records()[0], "record1");
        EXPECT_EQ(df.Records()[1], "record2");
        EXPECT_EQ(df.Records()[2], "record3");
        EXPECT_TRUE(df.Empty()); // no fields yet -> no values
    }

    {
        // adding an existing record returns false and changes nothing
        EXPECT_FALSE(df.AddRecord("record1"));
        EXPECT_EQ(df.RecordSize(), 3);
    }

    {
        // with fields present, a new record grows the value buffer
        ASSERT_TRUE(df.AddField("field1"));
        EXPECT_TRUE(df.AddRecord("record4"));
        EXPECT_EQ(df.RecordSize(), 4);
        EXPECT_EQ(df.Size(), 1 * 4);
    }
}

/**
 *  @brief Adding multiple records returns the number inserted and appends new ones in order.
 *  @see   lugizmo::DataFrame.AddRecords(std::span<R const> indices, T const& defaultValue) -> std::size_t requires DFUnqIndex<RecI>
 */
TEST(RM_DataframeAdding, AddRecords)
{
    using namespace lugizmo;
    auto df = DataFrame<int, std::string, std::string>();

    {
        // bulk add returns the number inserted and appends in order
        EXPECT_EQ(df.AddRecords(std::array<std::string, 3>{"record1", "record2", "record3"}), 3);
        ASSERT_EQ(df.RecordSize(), 3);
        EXPECT_EQ(df.Records()[0], "record1");
        EXPECT_EQ(df.Records()[2], "record3");
    }

    {
        // re-adding existing records inserts none
        EXPECT_EQ(df.AddRecords(std::array<std::string, 3>{"record1", "record2", "record3"}), 0);
        EXPECT_EQ(df.RecordSize(), 3);
    }

    {
        // a mix inserts only the new ones, appended after the existing
        EXPECT_EQ(df.AddRecords(std::array<std::string, 3>{"record1", "record4", "record5"}), 2);
        ASSERT_EQ(df.RecordSize(), 5);
        EXPECT_EQ(df.Records()[3], "record4");
        EXPECT_EQ(df.Records()[4], "record5");
    }
}
