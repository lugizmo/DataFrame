// Filename: DataFrameTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include <mdspan>

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

TEST(lugizmo_dataframe_range_index_test, row_major_initializations)
{
    using namespace lugizmo;
    using TestDF = DataFrame<float, DFRangeIndex<int>, DFRangeIndex<int>>;

    // initialize step by step
    {
        constexpr std::size_t FLD_COUNT = 10;
        constexpr std::size_t REC_COUNT = 10;

        auto df = TestDF(FLD_COUNT * REC_COUNT);

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
//        // build from span
//        auto const fields    = std::array{std::string("0"), std::string("1"), std::string("2")};
//        auto const dfFromFld = TestDF::FromFields(fields);
//        ASSERT_TRUE(dfFromFld.Empty());
//        ASSERT_TRUE(dfFromFld.Fields().size() == 3);
//
//        // build from initializer_list
//        auto const dfFromFldInt = TestDF::FromFields({"0", "1", "2"});
//        ASSERT_TRUE(dfFromFldInt.Empty());
//        ASSERT_TRUE(dfFromFldInt.Fields().size() == 3);
//
//        // TODO add test with "wrong" capacities and a backing mem-resource
    }

    // all at once
    {
//        // build were all have the same records
//        auto const dfFromFldAndRecDef = TestDF::FromFieldsAndRecord(std::array{std::string("0"), std::string("1"), std::string("2")},
//                                                                    std::array{0, 1, 2});
//        auto const dfFromFldAndRec = TestDF::FromFieldsAndRecord(std::array{std::string("0"), std::string("1"), std::string("2")},
//                                                                 std::array{0, 1, 2},
//                                                                 std::array{1.f, 2.f, 3.f});
//        ASSERT_TRUE(!dfFromFldAndRecDef.Empty());
//        ASSERT_TRUE(dfFromFldAndRecDef.Fields().size() == 3);
//        ASSERT_TRUE(dfFromFldAndRecDef.Records().size() == 3);
//
//        ASSERT_TRUE(!dfFromFldAndRec.Empty());
//        ASSERT_TRUE(dfFromFldAndRec.Fields().size() == 3);
//        ASSERT_TRUE(dfFromFldAndRec.Records().size() == 3);
//
//        // build were all have the different records/values
//        auto const dfFromFldAndRecs = TestDF::FromFieldsAndRecords(std::array{std::string("0"), std::string("1"), std::string("2")},
//                                                                   std::array{0, 1, 2},
//                                                                   std::array{std::array{1.f, 2.f, 3.f}, std::array{4.f, 5.f, 6.f}, std::array{7.f, 8.f, 9.f}});
//        ASSERT_TRUE(!dfFromFldAndRecs.Empty());
//        ASSERT_TRUE(dfFromFldAndRecs.Fields().size() == 3);
//        ASSERT_TRUE(dfFromFldAndRecs.Records().size() == 3);
//
//        // build were all have the different records/values
//        auto const dfFromInitListDef = TestDF::FromFieldsAndRecords({"0", "1", "2"}, {0, 1, 2});
//        auto const dfFromInitList    = TestDF::FromFieldsAndRecords({"0", "1", "2"},
//                                                                    {0, 1, 2},
//                                                                    {{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}, {7.f, 8.f, 9.f}});
//        ASSERT_TRUE(!dfFromInitListDef.Empty());
//        ASSERT_TRUE(dfFromInitListDef.Fields().size() == 3);
//        ASSERT_TRUE(dfFromInitListDef.Records().size() == 3);
//
//        ASSERT_TRUE(!dfFromInitList.Empty());
//        ASSERT_TRUE(dfFromInitList.Fields().size() == 3);
//        ASSERT_TRUE(dfFromInitList.Records().size() == 3);
    }
}