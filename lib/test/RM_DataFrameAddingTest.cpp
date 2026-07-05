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
// Partial: SetFieldRange (default values and preservation)
// - SetFieldRange(std::optional<FldT> lower, std::optional<FldT> upper, T const& defaultVal) -> bool requires DFRngIndex<FldI>
// - SetFieldRange(DFRangeIndexBounds<FldT> bounds, T const& defaultVal) -> bool requires DFRngIndex<FldI>
//
// Partial: SetRecordRange (default values and preservation)
// - SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, T const& defaultVal) -> bool requires DFRngIndex<RecI>
// - SetRecordRange(DFRangeIndexBounds<RecT> bounds, T const& defaultVal) -> bool requires DFRngIndex<RecI>
//
// TODO:
//   AddRecordPopulated(R index, std::span<T const> records) -> bool requires DFUnqIndex<RecI>
//   SetRecordRange (record-value overloads) requires DFSeqIndex
//

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  @brief Adding a single field appends it (when new) and grows the value buffer once records exist.
 *  @see   lgz::DataFrame.AddField(F index, T const& defaultValue) -> bool requires DFUnqIndex<FldI>
 */
TEST(RM_DataframeAdding, AddField)
{
    using namespace lgz;
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
        // with records present, an explicit fill value populates the new field and existing cells survive
        ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"record1", "record2"}), 2);
        ASSERT_TRUE(df.AssignValue("field1", "record1", 11));
        ASSERT_TRUE(df.AssignValue("field2", "record1", 12));
        ASSERT_TRUE(df.AssignValue("field3", "record1", 13));
        ASSERT_TRUE(df.AssignValue("field1", "record2", 21));
        ASSERT_TRUE(df.AssignValue("field2", "record2", 22));
        ASSERT_TRUE(df.AssignValue("field3", "record2", 23));

        ASSERT_TRUE(df.AddField("field4", -4));
        ASSERT_EQ(df.FieldSize(), 4);
        ASSERT_EQ(df.Size(), 4 * 2);

        EXPECT_EQ(*df.GetValue("field1", "record1"), 11);
        EXPECT_EQ(*df.GetValue("field2", "record1"), 12);
        EXPECT_EQ(*df.GetValue("field3", "record1"), 13);
        EXPECT_EQ(*df.GetValue("field1", "record2"), 21);
        EXPECT_EQ(*df.GetValue("field2", "record2"), 22);
        EXPECT_EQ(*df.GetValue("field3", "record2"), 23);
        EXPECT_EQ(*df.GetValue("field4", "record1"), -4);
        EXPECT_EQ(*df.GetValue("field4", "record2"), -4);
    }

    {
        // omitting the fill value uses T{} and still preserves existing cells
        ASSERT_TRUE(df.AddField("field5"));
        ASSERT_EQ(df.FieldSize(), 5);

        auto const fields   = std::array<std::string, 4>{"field1", "field2", "field3", "field4"};
        auto const records  = std::array<std::string, 2>{"record1", "record2"};
        auto const expected = std::array{std::array{11, 12, 13, -4}, std::array{21, 22, 23, -4}};
        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                EXPECT_EQ(*df.GetValue(fields[f], records[r]), expected[r][f]);
            }
            EXPECT_EQ(*df.GetValue("field5", records[r]), 0);
        }
    }
}

/**
 *  @brief Adding multiple fields returns the number inserted and appends new ones in order.
 *  @see   lgz::DataFrame.AddFields(std::span<F const> indices, T const& defaultValue) -> std::size_t requires DFUnqIndex<FldI>
 */
TEST(RM_DataframeAdding, AddFields)
{
    using namespace lgz;
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

    {
        // with records present, an explicit fill value populates new fields and existing cells survive
        auto const fields  = std::array<std::string, 5>{"field1", "field2", "field3", "field4", "field5"};
        auto const records = std::array<std::string, 2>{"record1", "record2"};
        ASSERT_EQ(df.AddRecords(records), records.size());

        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                ASSERT_TRUE(df.AssignValue(fields[f], records[r], static_cast<int>((r + 1) * 10 + f + 1)));
            }
        }

        ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"field6", "field7"}, -2), 2);
        ASSERT_EQ(df.FieldSize(), 7);

        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                EXPECT_EQ(*df.GetValue(fields[f], records[r]), static_cast<int>((r + 1) * 10 + f + 1));
            }
            EXPECT_EQ(*df.GetValue("field6", records[r]), -2);
            EXPECT_EQ(*df.GetValue("field7", records[r]), -2);
        }
    }

    {
        // omitting the fill value uses T{} for every new field
        auto const fields   = std::array<std::string, 7>{"field1", "field2", "field3", "field4", "field5", "field6", "field7"};
        auto const records  = std::array<std::string, 2>{"record1", "record2"};
        auto const expected = std::array{std::array{11, 12, 13, 14, 15, -2, -2}, std::array{21, 22, 23, 24, 25, -2, -2}};

        ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"field8", "field9"}), 2);
        ASSERT_EQ(df.FieldSize(), 9);

        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                EXPECT_EQ(*df.GetValue(fields[f], records[r]), expected[r][f]);
            }
            EXPECT_EQ(*df.GetValue("field8", records[r]), 0);
            EXPECT_EQ(*df.GetValue("field9", records[r]), 0);
        }
    }
}

/**
 *  @brief Adding a single record appends it (when new) and grows the value buffer once fields exist.
 *  @see   lgz::DataFrame.AddRecord(R index, T const& defaultValue) -> bool requires DFUnqIndex<RecI>
 */
TEST(RM_DataframeAdding, AddRecord)
{
    using namespace lgz;
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
        // with fields present, an explicit fill value populates the new record and existing cells survive
        ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"field1", "field2"}), 2);
        ASSERT_TRUE(df.AssignValue("field1", "record1", 11));
        ASSERT_TRUE(df.AssignValue("field2", "record1", 12));
        ASSERT_TRUE(df.AssignValue("field1", "record2", 21));
        ASSERT_TRUE(df.AssignValue("field2", "record2", 22));
        ASSERT_TRUE(df.AssignValue("field1", "record3", 31));
        ASSERT_TRUE(df.AssignValue("field2", "record3", 32));

        ASSERT_TRUE(df.AddRecord("record4", -4));
        ASSERT_EQ(df.RecordSize(), 4);
        ASSERT_EQ(df.Size(), 2 * 4);

        EXPECT_EQ(*df.GetValue("field1", "record1"), 11);
        EXPECT_EQ(*df.GetValue("field2", "record1"), 12);
        EXPECT_EQ(*df.GetValue("field1", "record2"), 21);
        EXPECT_EQ(*df.GetValue("field2", "record2"), 22);
        EXPECT_EQ(*df.GetValue("field1", "record3"), 31);
        EXPECT_EQ(*df.GetValue("field2", "record3"), 32);
        EXPECT_EQ(*df.GetValue("field1", "record4"), -4);
        EXPECT_EQ(*df.GetValue("field2", "record4"), -4);
    }

    {
        // omitting the fill value uses T{} and still preserves existing cells
        ASSERT_TRUE(df.AddRecord("record5"));
        ASSERT_EQ(df.RecordSize(), 5);

        auto const fields   = std::array<std::string, 2>{"field1", "field2"};
        auto const records  = std::array<std::string, 4>{"record1", "record2", "record3", "record4"};
        auto const expected = std::array{std::array{11, 12}, std::array{21, 22}, std::array{31, 32}, std::array{-4, -4}};
        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                EXPECT_EQ(*df.GetValue(fields[f], records[r]), expected[r][f]);
            }
        }
        EXPECT_EQ(*df.GetValue("field1", "record5"), 0);
        EXPECT_EQ(*df.GetValue("field2", "record5"), 0);
    }
}

/**
 *  @brief Adding multiple records returns the number inserted and appends new ones in order.
 *  @see   lgz::DataFrame.AddRecords(std::span<R const> indices, T const& defaultValue) -> std::size_t requires DFUnqIndex<RecI>
 */
TEST(RM_DataframeAdding, AddRecords)
{
    using namespace lgz;
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

    {
        // with fields present, an explicit fill value populates new records and existing cells survive
        auto const fields  = std::array<std::string, 2>{"field1", "field2"};
        auto const records = std::array<std::string, 5>{"record1", "record2", "record3", "record4", "record5"};
        ASSERT_EQ(df.AddFields(fields), fields.size());

        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                ASSERT_TRUE(df.AssignValue(fields[f], records[r], static_cast<int>((r + 1) * 10 + f + 1)));
            }
        }

        ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"record6", "record7"}, -2), 2);
        ASSERT_EQ(df.RecordSize(), 7);

        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                EXPECT_EQ(*df.GetValue(fields[f], records[r]), static_cast<int>((r + 1) * 10 + f + 1));
            }
        }
        for (auto const& field : fields)
        {
            EXPECT_EQ(*df.GetValue(field, "record6"), -2);
            EXPECT_EQ(*df.GetValue(field, "record7"), -2);
        }
    }

    {
        // omitting the fill value uses T{} for every new record
        auto const fields  = std::array<std::string, 2>{"field1", "field2"};
        auto const records = std::array<std::string, 5>{"record1", "record2", "record3", "record4", "record5"};

        ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"record8", "record9"}), 2);
        ASSERT_EQ(df.RecordSize(), 9);

        for (std::size_t r = 0; r < records.size(); ++r)
        {
            for (std::size_t f = 0; f < fields.size(); ++f)
            {
                EXPECT_EQ(*df.GetValue(fields[f], records[r]), static_cast<int>((r + 1) * 10 + f + 1));
            }
        }
        for (auto const& field : fields)
        {
            EXPECT_EQ(*df.GetValue(field, "record6"), -2);
            EXPECT_EQ(*df.GetValue(field, "record7"), -2);
            EXPECT_EQ(*df.GetValue(field, "record8"), 0);
            EXPECT_EQ(*df.GetValue(field, "record9"), 0);
        }
    }
}

/**
 *  @brief Moving field bounds fills new columns and preserves retained columns.
 *  @see   lgz::DataFrame.SetFieldRange(...)
 */
TEST(RM_DataframeAdding, SetFieldRange)
{
    using namespace lgz;
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

    {
        // optional bounds with step 1
        auto df = DF::FromFieldsAndRecords({.lower = 1, .upper = 3}, {.lower = 10, .upper = 12},
                                           {{11, 12}, {21, 22}});

        ASSERT_TRUE(df.SetFieldRange(std::optional{0}, std::optional{4}, -5));
        ASSERT_EQ(df.FieldSize(), 4);
        ASSERT_EQ(df.Size(), 4 * 2);

        EXPECT_EQ(*df.GetValue(1, 10), 11);
        EXPECT_EQ(*df.GetValue(2, 10), 12);
        EXPECT_EQ(*df.GetValue(1, 11), 21);
        EXPECT_EQ(*df.GetValue(2, 11), 22);
        EXPECT_EQ(*df.GetValue(0, 10), -5);
        EXPECT_EQ(*df.GetValue(3, 10), -5);
        EXPECT_EQ(*df.GetValue(0, 11), -5);
        EXPECT_EQ(*df.GetValue(3, 11), -5);
    }

    {
        // bounds object with step 2
        auto df = DF::FromFieldsAndRecords({.lower = 2, .upper = 6, .step = 2},
                                           {.lower = 10, .upper = 14, .step = 2},
                                           {{11, 12}, {21, 22}});

        ASSERT_TRUE(df.SetFieldRange(DFRangeIndexBounds<int>{.lower = 0, .upper = 8, .step = 2}, -6));
        ASSERT_EQ(df.FieldSize(), 4);
        ASSERT_EQ(df.Size(), 4 * 2);

        EXPECT_EQ(*df.GetValue(2, 10), 11);
        EXPECT_EQ(*df.GetValue(4, 10), 12);
        EXPECT_EQ(*df.GetValue(2, 12), 21);
        EXPECT_EQ(*df.GetValue(4, 12), 22);
        EXPECT_EQ(*df.GetValue(0, 10), -6);
        EXPECT_EQ(*df.GetValue(6, 10), -6);
        EXPECT_EQ(*df.GetValue(0, 12), -6);
        EXPECT_EQ(*df.GetValue(6, 12), -6);
    }
}

/**
 *  @brief Moving record bounds fills new rows and preserves retained rows.
 *  @see   lgz::DataFrame.SetRecordRange(...)
 */
TEST(RM_DataframeAdding, SetRecordRange)
{
    using namespace lgz;
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

    {
        // optional bounds with step 1
        auto df = DF::FromFieldsAndRecords({.lower = 1, .upper = 3}, {.lower = 10, .upper = 12},
                                           {{11, 12}, {21, 22}});

        ASSERT_TRUE(df.SetRecordRange(std::optional{9}, std::optional{13}, -7));
        ASSERT_EQ(df.RecordSize(), 4);
        ASSERT_EQ(df.Size(), 2 * 4);

        EXPECT_EQ(*df.GetValue(1, 10), 11);
        EXPECT_EQ(*df.GetValue(2, 10), 12);
        EXPECT_EQ(*df.GetValue(1, 11), 21);
        EXPECT_EQ(*df.GetValue(2, 11), 22);
        EXPECT_EQ(*df.GetValue(1, 9), -7);
        EXPECT_EQ(*df.GetValue(2, 9), -7);
        EXPECT_EQ(*df.GetValue(1, 12), -7);
        EXPECT_EQ(*df.GetValue(2, 12), -7);
    }

    {
        // bounds object with step 2
        auto df = DF::FromFieldsAndRecords({.lower = 2, .upper = 6, .step = 2},
                                           {.lower = 10, .upper = 14, .step = 2},
                                           {{11, 12}, {21, 22}});

        ASSERT_TRUE(df.SetRecordRange(DFRangeIndexBounds<int>{.lower = 8, .upper = 16, .step = 2}, -8));
        ASSERT_EQ(df.RecordSize(), 4);
        ASSERT_EQ(df.Size(), 2 * 4);

        EXPECT_EQ(*df.GetValue(2, 10), 11);
        EXPECT_EQ(*df.GetValue(4, 10), 12);
        EXPECT_EQ(*df.GetValue(2, 12), 21);
        EXPECT_EQ(*df.GetValue(4, 12), 22);
        EXPECT_EQ(*df.GetValue(2, 8), -8);
        EXPECT_EQ(*df.GetValue(4, 8), -8);
        EXPECT_EQ(*df.GetValue(2, 14), -8);
        EXPECT_EQ(*df.GetValue(4, 14), -8);
    }
}
