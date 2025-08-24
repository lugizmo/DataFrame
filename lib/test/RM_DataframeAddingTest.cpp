// Filename: RM_DataframeAddingTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions adding fields and records to the dataframe.
// The following functions are tested here (with names of tests):
//
// ✅ add_fields_val
// - AddField(F index, T const& defaultValue) -> bool requires DFValIndex<FldI>
// - AddFields(std::span<F const> const indices, T const& defaultValue) -> bool requires DFValIndex<FldI>
//
// ✅ add_records_val
// - AddRecord(R index, T const& defaultValue) -> bool requires DFValIndex<RecI>;
// - AddRecords(std::span<R const> indices, T const& defaultValue) -> std::size_t requires DFValIndex<RecI>;
//
// TODO
//     AddRecordPopulated(R index, std::span<T const> records) -> bool requires DFValIndex<RecI>
//     SetFieldRange(std::optional<FldT> lower, std::optional<FldT> upper, T const& defaultVal) -> bool requires DFSeqIndex<FldI>
//     SetFieldRange(DFRangeIndexBounds<FldT>, T const& defaultVal) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, T const& defaultVal) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(DFRangeIndexBounds<RecT>, T const& defaultVal) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::span<T const> records) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(DFRangeIndexBounds<RecT>, std::span<T const> records) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, IterableOfIterable auto const& records) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(DFRangeIndexBounds<RecT>, IterableOfIterable auto const& records) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::initializer_list<std::initializer_list<T>> records) -> bool requires DFSeqIndex<FldI>
//     SetRecordRange(DFRangeIndexBounds<RecT>, std::initializer_list<std::initializer_list<T>> records) -> bool requires DFSeqIndex<FldI>

#include <string>
#include <array>

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

/**
 *  @brief Adding field and fields to a value-indexed dataframe.
 *  @see   lugizmo::Dataframe.AddField(F index, T const& defaultValue) -> bool requires DFValIndex<FldI>
 *         lugizmo::Dataframe.AddFields(std::span<F const> const indices, T const& defaultValue) -> bool requires DFValIndex<FldI>
 */
TEST(lugizmo_dataframe_adding_row_major, add_fields_val)
{
    using namespace lugizmo;
    auto df = DataFrame<int, std::string, std::string>();

    // AddField
    EXPECT_TRUE(df.AddField("field1"));
    EXPECT_TRUE(df.AddField("field2"));
    EXPECT_TRUE(df.AddField("field3"));

    EXPECT_EQ(df.FieldSize(), 3);
    EXPECT_EQ(df.Fields().size(), 3);
    EXPECT_EQ(df.Fields()[0], "field1");
    EXPECT_EQ(df.Fields()[1], "field2");
    EXPECT_EQ(df.Fields()[2], "field3");
    EXPECT_TRUE(df.Empty());

    // adding existing should not change anything
    EXPECT_FALSE(df.AddField("field1"));
    EXPECT_FALSE(df.AddField("field2"));
    EXPECT_FALSE(df.AddField("field3"));

    EXPECT_EQ(df.FieldSize(), 3);
    EXPECT_EQ(df.Fields().size(), 3);
    EXPECT_EQ(df.Fields()[0], "field1");
    EXPECT_EQ(df.Fields()[1], "field2");
    EXPECT_EQ(df.Fields()[2], "field3");
    EXPECT_TRUE(df.Empty());

    // AddFields
    std::array<std::string, 3> fields = {"field4", "field5", "field6"};
    EXPECT_EQ(df.AddFields(fields), 3);

    EXPECT_EQ(df.FieldSize(), 6);
    EXPECT_EQ(df.Fields().size(), 6);
    EXPECT_EQ(df.Fields()[3], "field4");
    EXPECT_EQ(df.Fields()[4], "field5");
    EXPECT_EQ(df.Fields()[5], "field6");
    EXPECT_TRUE(df.Empty());

    // adding existing should not change anything
    EXPECT_EQ(df.AddFields(fields), 0);
    EXPECT_EQ(df.FieldSize(), 6);
    EXPECT_EQ(df.Fields().size(), 6);
    EXPECT_EQ(df.Fields()[3], "field4");
    EXPECT_EQ(df.Fields()[4], "field5");
    EXPECT_EQ(df.Fields()[5], "field6");
    EXPECT_TRUE(df.Empty());

    // add records after fields added
    EXPECT_TRUE(df.AddRecord("record1"));
    EXPECT_EQ(df.RecordSize(), 1);
    EXPECT_FALSE(df.Empty());
    EXPECT_EQ(df.Size(), 6 * 1);

    EXPECT_EQ(df.AddRecords(std::array<std::string, 2>{"record2", "record3"}), 2);
    EXPECT_EQ(df.RecordSize(), 3);
    EXPECT_FALSE(df.Empty());
    EXPECT_EQ(df.Size(), 6 * 3);
}

/**
 *  @brief Adding record and records to a value-indexed dataframe.
 *  @see   lugizmo::Dataframe.AddField(F index, T const& defaultValue) -> bool requires DFValIndex<FldI>
 *         lugizmo::Dataframe.AddFields(std::span<F const> const indices, T const& defaultValue) -> bool requires DFValIndex<FldI>
 */
TEST(lugizmo_dataframe_adding_row_major, add_records_val)
{
    using namespace lugizmo;
    auto df = DataFrame<int, std::string, std::string>();

    // AddRecord
    EXPECT_TRUE(df.AddRecord("record1"));
    EXPECT_TRUE(df.AddRecord("record2"));
    EXPECT_TRUE(df.AddRecord("record3"));

    EXPECT_EQ(df.RecordSize(), 3);
    EXPECT_EQ(df.Records().size(), 3);
    EXPECT_EQ(df.Records()[0], "record1");
    EXPECT_EQ(df.Records()[1], "record2");
    EXPECT_EQ(df.Records()[2], "record3");
    EXPECT_TRUE(df.Empty());

    // adding existing should not change anything
    EXPECT_FALSE(df.AddRecord("record1"));
    EXPECT_FALSE(df.AddRecord("record2"));
    EXPECT_FALSE(df.AddRecord("record3"));

    EXPECT_EQ(df.RecordSize(), 3);
    EXPECT_EQ(df.Records().size(), 3);
    EXPECT_EQ(df.Records()[0], "record1");
    EXPECT_EQ(df.Records()[1], "record2");
    EXPECT_EQ(df.Records()[2], "record3");
    EXPECT_TRUE(df.Empty());

    // AddRecords
    std::array<std::string, 3> records = {"record4", "record5", "record6"};
    EXPECT_EQ(df.AddRecords(records), 3);

    EXPECT_EQ(df.RecordSize(), 6);
    EXPECT_EQ(df.Records().size(), 6);
    EXPECT_EQ(df.Records()[3], "record4");
    EXPECT_EQ(df.Records()[4], "record5");
    EXPECT_EQ(df.Records()[5], "record6");
    EXPECT_TRUE(df.Empty());

    // adding existing should not change anything
    EXPECT_EQ(df.AddRecords(records), 0);
    EXPECT_EQ(df.Records().size(), 6);
    EXPECT_EQ(df.Records()[3], "record4");
    EXPECT_EQ(df.Records()[4], "record5");
    EXPECT_EQ(df.Records()[5], "record6");
    EXPECT_TRUE(df.Empty());

    // add fields after records added
    EXPECT_TRUE(df.AddField("field1"));
    EXPECT_EQ(df.FieldSize(), 1);
    EXPECT_FALSE(df.Empty());
    EXPECT_EQ(df.Size(), 6 * 1);

    EXPECT_EQ(df.AddFields(std::array<std::string, 2>{"field2", "field3"}), 2);
    EXPECT_EQ(df.FieldSize(), 3);
    EXPECT_FALSE(df.Empty());
    EXPECT_EQ(df.Size(), 6 * 3);
}
