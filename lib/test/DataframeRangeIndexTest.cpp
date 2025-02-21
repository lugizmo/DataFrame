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
        for(auto fld = 0; fld <= FLD_COUNT; ++fld) df.SetFieldRange(-1 * fld, fld, 42.f);
        for(auto rec = 0; rec <= REC_COUNT; ++rec) df.SetRecordRange(-1 * rec, rec, 42.f);

        auto const dfFlds = df.Fields();
        auto const dfRecs = df.Records();

        ASSERT_TRUE(!df.Empty());
        ASSERT_EQ(dfFlds.size(), FLD_COUNT * 2);
        ASSERT_EQ(dfRecs.size(), REC_COUNT * 2);
        ASSERT_TRUE(std::ranges::all_of(df.ValuesSpan(), [](auto const val) { return val == 42.f; }));
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

    auto values = df.ValuesSpan();
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

    // set values with SetValue()
    for(auto col = fields.lower; col < fields.upper; ++col)
        for(auto row = records.lower; row < records.upper; ++row)
            ASSERT_TRUE(df.SetValue(col, row, 42));

    values = df.ValuesSpan();
    ASSERT_TRUE(std::ranges::all_of(values, [](auto const& v) { return v == 42; }));

    // set values with [] operator
    for(auto col = fields.lower; col < fields.upper; ++col)
        for(auto row = records.lower; row < records.upper; ++row)
            df[col, row] = 43;

    values = df.ValuesSpan();
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

    while(upper >= 0)
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

// TODO test adding ranges by using span<T>, initializer<T> & iterable<iterable<T>>