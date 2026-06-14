// Filename: DataFrameTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

TEST(lugizmo_dataframe_range_index_test, layout_default_is_row_major)
{
    using namespace lugizmo;
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;
    static_assert(std::is_same_v<DF::Layout, DFRowMajor<int>>);
}

TEST(lugizmo_dataframe_range_index_test, row_major_empty_initialization)
{
    using namespace lugizmo;

    DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>> const df;
    ASSERT_TRUE(df.Empty());
}

TEST(lugizmo_dataframe_range_index_test, row_major_initializations_range_only)
{
    using namespace lugizmo;
    using DF = DataFrame<float, DFRangeIndex<int>, DFRangeIndex<int>>;

    // initialize step by step
    {
        constexpr std::size_t FLD_COUNT = 10;
        constexpr std::size_t REC_COUNT = 10;

        auto df = DF(FLD_COUNT * REC_COUNT);

        // TODO add test where adding cols/rows in different order so first records then fields (some fields must be present for that).
        for(size_t fld = 0; fld <= FLD_COUNT; ++fld) df.SetFieldRange(-1 * static_cast<int>(fld), fld, 42.f);
        for(size_t rec = 0; rec <= REC_COUNT; ++rec) df.SetRecordRange(-1 * static_cast<int>(rec), rec, 42.f);

        auto const dfFlds = df.Fields();
        auto const dfRecs = df.Records();

        ASSERT_TRUE(!df.Empty());
        ASSERT_EQ(dfFlds.size(), FLD_COUNT * 2);
        ASSERT_EQ(dfRecs.size(), REC_COUNT * 2);
        ASSERT_TRUE(std::ranges::all_of(df.Values(), [](auto const val) { return val == 42.f; }));
    }

    // initialize fields
    {
        // build from span
        auto constexpr fields = DFRangeIndexBounds{.lower = -10, .upper = 10};
        auto const dfFromFld  = DF::FromFields(fields);
        ASSERT_TRUE(dfFromFld.Empty());
        ASSERT_TRUE(dfFromFld.Fields().size() == fields.upper - fields.lower);

        // build from lower bound "ctor" mimicking look when using fields based on unique indices.
        // other than that this does not test anything really.
        auto const dfFromFldInt = DF::FromFields({-10, 10});
        ASSERT_TRUE(dfFromFldInt.Empty());
        ASSERT_TRUE(dfFromFldInt.Fields().size() == 20);

        // TODO add test with "wrong" capacities and a backing mem-resource
    }

    // all at once
    {
        // build were all have the same records
        auto const dfFromFldAndRecDef = DF::FromFieldsAndRecord({.lower = -1, .upper = 2}, {.lower = -1, .upper = 2});
        auto const dfFromFldAndRec    = DF::FromFieldsAndRecord({.lower = -1, .upper = 2}, {.lower = -1, .upper = 2}, std::array{1.f, 2.f, 3.f});

        ASSERT_TRUE(!dfFromFldAndRecDef.Empty());
        ASSERT_TRUE(dfFromFldAndRecDef.Fields().size() == 3);
        ASSERT_TRUE(dfFromFldAndRecDef.Records().size() == 3);

        ASSERT_TRUE(!dfFromFldAndRec.Empty());
        ASSERT_TRUE(dfFromFldAndRec.Fields().size() == 3);
        ASSERT_TRUE(dfFromFldAndRec.Records().size() == 3);

        // build were all have the different records/values
        auto const dfFromFldAndRecs = DF::FromFieldsAndRecords({.lower = -1, .upper = 2}, {.lower = -1, .upper = 2},
                                                                   std::array{std::array{1.f, 2.f, 3.f}, std::array{4.f, 5.f, 6.f}, std::array{7.f, 8.f, 9.f}});
        ASSERT_TRUE(!dfFromFldAndRecs.Empty());
        ASSERT_TRUE(dfFromFldAndRecs.Fields().size() == 3);
        ASSERT_TRUE(dfFromFldAndRecs.Records().size() == 3);

        // build were all have the different records/values
        auto const dfFromInitListDef = DF::FromFieldsAndRecords({.lower = -1, .upper = 2}, {.lower = -1, .upper = 2});
        auto const dfFromInitList    = DF::FromFieldsAndRecords({.lower = -1, .upper = 2}, {.lower = -1, .upper = 2}, {{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}, {7.f, 8.f, 9.f}});
        ASSERT_TRUE(!dfFromInitListDef.Empty());
        ASSERT_TRUE(dfFromInitListDef.Fields().size() == 3);
        ASSERT_TRUE(dfFromInitListDef.Records().size() == 3);

        ASSERT_TRUE(!dfFromInitList.Empty());
        ASSERT_TRUE(dfFromInitList.Fields().size() == 3);
        ASSERT_TRUE(dfFromInitList.Records().size() == 3);
    }
}

TEST(lugizmo_dataframe_range_index_test, row_major_set_get_records)
{
    using namespace lugizmo;
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

    auto df = DF();

    auto fields  = DFRangeIndexBounds<int>();
    auto records = DFRangeIndexBounds<int>();

    // add columns one by one
    // don't do this at home
    for(auto col = 1; col < 6; ++col)
    {
        fields = {.lower = col * -1, .upper = 0};
        df.SetFieldRange(fields);
    }

    ASSERT_TRUE(df.Empty());
    ASSERT_EQ(df.Fields().size(), 5);

    for(auto col = 1; col < 6; ++col)
    {
        fields = {.lower = fields.lower, .upper = col};
        df.SetFieldRange(fields);
    }

    ASSERT_TRUE(df.Empty());
    ASSERT_EQ(df.Fields().size(), 10);

    // add records one by one
    // don't do this at home
    for(auto row = 1; row < 6; ++row)
    {
        records = {.lower = row * -1, .upper = 0};
        df.SetRecordRange(records, 0);
    }
    ASSERT_FALSE(df.Empty());
    ASSERT_EQ(df.Records().size(), 5);

    for(auto row = 1; row < 6; ++row)
    {
        records = {.lower = records.lower, .upper = row};
        df.SetRecordRange(records, 0);
    }
    ASSERT_FALSE(df.Empty());
    ASSERT_EQ(df.Records().size(), 10);

    auto values = df.Values();
    ASSERT_TRUE(std::ranges::all_of(values, [](auto const& v) { return v == 0; }));

    // get values with GetValue()
    for(auto col = fields.lower; col < fields.upper; ++col)
        for(auto row = records.lower; row < records.upper; ++row)
            ASSERT_EQ(df.GetValue(col, row), 0);

    // get values with [] operator
    for(auto col = fields.lower; col < fields.upper; ++col)
        for(auto row = records.lower; row < records.upper; ++row) {
            auto const& val = df[col, row];
            ASSERT_EQ(val, 0);
        }

    // replace values with replace_val()
    for(auto col = fields.lower; col < fields.upper; ++col)
        for(auto row = records.lower; row < records.upper; ++row)
            ASSERT_TRUE(df.AssignValue(col, row, 42));

    values = df.Values();
    ASSERT_TRUE(std::ranges::all_of(values, [](auto const& v) { return v == 42; }));

    // set values with [] operator
    for(auto col = fields.lower; col < fields.upper; ++col)
        for(auto row = records.lower; row < records.upper; ++row)
            df[col, row] = 43;

    values = df.Values();
    ASSERT_TRUE(std::ranges::all_of(values, [](auto const& v) { return v == 43; }));

    // ====== SHRINK IT! ==========================================================================================

    auto lower = records.lower;
    auto upper = records.upper;

    ASSERT_TRUE(lower < 0);
    ASSERT_TRUE(upper > 0);

    // remove records one by one
    // don't do this at home
    while(lower <= 0)
    {
        records = {.lower = lower++, .upper = upper};
        df.SetRecordRange(records, 0);
    }
    ASSERT_FALSE(df.Empty());
    ASSERT_EQ(df.Records().size(), 5);

    while(upper >= 1)
    {
        records = {.lower = lower, .upper = upper--};
        df.SetRecordRange(records, 0);
    }
    ASSERT_TRUE(df.Empty());
    ASSERT_EQ(df.Records().size(), 0);
}

TEST(lugizmo_dataframe_range_index_test, row_major_set_with_records)
{

}

TEST(lugizmo_dataframe_range_index_test, row_major_strided_record_range)
{
    using namespace lugizmo;
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

    auto df = DF();
    df.SetFieldRange(DFRangeIndexBounds<int>{.lower = 0, .upper = 3});                 // fields  0, 1, 2
    df.SetRecordRange(DFRangeIndexBounds<int>{.lower = 0, .upper = 10, .step = 2}, 0); // records 0, 2, 4, 6, 8

    // the matrix holds one row per on-grid record, not per key span
    ASSERT_EQ(df.FieldSize(), 3);
    ASSERT_EQ(df.RecordSize(), 5);
    ASSERT_EQ(df.Records().size(), 5);

    // staying on the step grid: assign and read back every (field, record) cell
    int expected = 0;
    for (int rec = 0; rec < 10; rec += 2)
    {
        for (int fld = 0; fld < 3; ++fld)
        {
            ASSERT_TRUE(df.AssignValue(fld, rec, expected));
            auto const val = df.GetValue(fld, rec);
            ASSERT_TRUE(val.HasValue());
            ASSERT_EQ(*val, expected);
            ++expected;
        }
    }

    // off-grid record keys are not members -> access fails
    ASSERT_FALSE(df.AssignValue(0, 1, 99));
    ASSERT_FALSE(df.AssignValue(0, 3, 99));
    ASSERT_FALSE(df.GetValue(0, 5).HasValue());
    ASSERT_FALSE(df.GetValue(0, 7).HasValue());

    // out-of-range record key fails as well
    ASSERT_FALSE(df.GetValue(0, 10).HasValue());

    // the physical buffer is dense (5 x 3) and in position order
    ASSERT_NE(df.Data(), nullptr);
    for (int i = 0; i < 15; ++i) ASSERT_EQ(df.Data()[i], i);
}

TEST(lugizmo_dataframe_range_index_test, row_major_strided_mdspan_mutation)
{
    using namespace lugizmo;
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

    auto df = DF();
    df.SetFieldRange(DFRangeIndexBounds<int>{.lower = 0, .upper = 2});                // fields  0, 1
    df.SetRecordRange(DFRangeIndexBounds<int>{.lower = 0, .upper = 6, .step = 2}, 0); // records 0, 2, 4

    // fill on-grid cells, observe the result through the mdspan
    int v = 0;
    for (int rec = 0; rec < 6; rec += 2)
        for (int fld = 0; fld < 2; ++fld)
            ASSERT_TRUE(df.AssignValue(fld, rec, v++));

    {
        auto const md = df.MDSpan();
        ASSERT_EQ(md.extent(0), 3); // rows  == record count
        ASSERT_EQ(md.extent(1), 2); // cols  == field count
        for (size_t i = 0; i < 6; ++i) ASSERT_EQ(md.data_handle()[i], static_cast<int>(i));
    }

    // grow the record range from the back: upper 6 -> 10 adds records 6, 8 (default 0)
    ASSERT_TRUE(df.SetRecordRange(DFRangeIndexBounds<int>{.lower = 0, .upper = 10, .step = 2}, 0));
    ASSERT_EQ(df.RecordSize(), 5);

    ASSERT_TRUE(df.AssignValue(0, 8, 80));
    ASSERT_TRUE(df.AssignValue(1, 8, 81));
    {
        auto const md = df.MDSpan();
        ASSERT_EQ(md.extent(0), 5);
        ASSERT_EQ(md.extent(1), 2);
        ASSERT_EQ((md[4, 0]), 80); // record 8 -> position 4
        ASSERT_EQ((md[4, 1]), 81);
        ASSERT_EQ((md[0, 0]), 0);  // original data preserved
        ASSERT_EQ((md[2, 1]), 5);  // record 4 (pos 2), field 1
        ASSERT_EQ((md[3, 0]), 0);  // record 6 (pos 3) was default-filled
    }

    // shrink the record range from the front: lower 0 -> 4 removes records 0, 2
    ASSERT_TRUE(df.SetRecordRange(DFRangeIndexBounds<int>{.lower = 4, .upper = 10, .step = 2}, 0));
    ASSERT_EQ(df.RecordSize(), 3); // records 4, 6, 8
    {
        auto const md = df.MDSpan();
        ASSERT_EQ(md.extent(0), 3);
        ASSERT_EQ((md[0, 0]), 4);  // record 4 is now position 0
        ASSERT_EQ((md[0, 1]), 5);
        ASSERT_EQ((md[2, 0]), 80); // record 8 is now position 2
        ASSERT_EQ((md[2, 1]), 81);
    }

    // changing the step on a populated range is rejected; the dataframe is left untouched
    auto const recordsBefore = df.RecordSize();
    ASSERT_FALSE(df.SetRecordRange(DFRangeIndexBounds<int>{.lower = 4, .upper = 10, .step = 3}, 0));
    ASSERT_EQ(df.RecordSize(), recordsBefore);
    {
        auto const md = df.MDSpan();
        ASSERT_EQ((md[0, 0]), 4);
        ASSERT_EQ((md[2, 1]), 81);
    }
}

// TODO test adding ranges by using span<T>, initializer<T> & iterable<iterable<T>>