// Filename: DataFrameIterateRowMajorTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions iterating over dataframe data.
// These tests work with classical iterators and ranges.
//

#include "gtest/gtest.h"

#include <array>

#include "lugizmo/DataFrame.h"

namespace lugizmo::test {

    inline constexpr size_t DFFldCount = 5;
    inline constexpr size_t DFRecCount = 10;

    inline constexpr std::array DFFields  = {0, 1, 2, 3, 4};
    inline constexpr std::array DFRecords = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    inline constexpr std::array DFData = {
            std::array{0,  1,  2,  3,  4 },
            std::array{5,  6,  7,  8,  9 },
            std::array{10, 11, 12, 13, 14},
            std::array{15, 16, 17, 18, 19},
            std::array{20, 21, 22, 23, 24},
            std::array{25, 26, 27, 28, 29},
            std::array{30, 31, 32, 33, 34},
            std::array{35, 36, 37, 38, 39},
            std::array{40, 41, 42, 43, 44},
            std::array{45, 46, 47, 48, 49}
    };

    inline auto DefaultDataframe() -> DataFrame<int, int, int>
    {
        return DataFrame<int, int, int>::FromFieldsAndRecords(DFFields, DFRecords, DFData);
    }
}

/**
 *  @brief Iterating over fields (not values);
 *  @see  lugizmo::Dataframe.Fields();
 */
TEST(lugizmo_dataframe_iterate_row_major, fields)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    auto const df   = DefaultDataframe();
    auto const flds = df.Fields();

    ASSERT_EQ(flds.size(), DFFldCount);

    auto count = 0;
    for(auto const fld : flds) ASSERT_EQ(fld, DFFields[count++]);
}

/**
 *  @brief Iterating over records (not values);
 *  @see  lugizmo::Dataframe.Records();
 */
TEST(lugizmo_dataframe_iterate_row_major, records)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    auto const df   = DefaultDataframe();
    auto const recs = df.Records();

    ASSERT_EQ(recs.size(), DFRecCount);

    auto count = 0;
    for(auto const rec : recs) ASSERT_EQ(rec, DFRecords[count++]);
}

/**
 *  @brief     Viewing values by manually going over each value in the underlying data.
 *  @attention This is not something a user should do as it can be error-prone. Better
 *             to use a version working on records/fields directly or using view types.
 *
 *  @see lugizmo::Dataframe.ValuesSpan();
 */
TEST(lugizmo_dataframe_iterate_row_major, values_data)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    auto const df = DefaultDataframe();
    auto const dp = df.Data();

    auto   rI = 0;
    auto   fI = 0;
    size_t i  = 0;
    for(auto const val : df.ValuesSpan())
    {
        ASSERT_EQ(val, DFData[rI][fI]);
        ASSERT_EQ(val, dp[i++]);

        if(++fI >= df.FieldSize())
        {
            fI = 0;
            ++rI;
        }
    }
}

// TODO add tests for
// auto ViewField(F const& index) noexcept -> DFView<T, RecI> requires DFValIndex<RecI>;
// auto ViewField(F const& index) const noexcept -> DFView<T const, RecI> requires DFValIndex<RecI>;
// auto ViewField(F const& index) noexcept -> DFView<T, RecI> requires DFSeqIndex<RecI>;
// auto ViewField(F const& index) const noexcept -> DFView<T const, RecI> requires DFSeqIndex<RecI>;
// auto ViewFieldIndexed(F const& index) noexcept -> DFViewIndexed<T, RecI const> requires DFValIndex<RecI>;
// auto ViewFieldIndexed(F const& index) const noexcept -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI>;
// auto ViewFieldIndexed(FldIndex const& index) noexcept -> DFViewIndexed<T, RecIndex const> requires DFSeqIndex<RecI>;
// auto ViewRecord(R const& index) noexcept -> DFView<T, FldI> requires DFValIndex<RecI>;
// auto ViewRecord(R const& index) const noexcept -> DFView<T const, FldI> requires DFValIndex<RecI>;
// auto ViewRecordIndexed(R const& index) noexcept -> DFViewIndexed<T, FldI const> requires DFValIndex<RecI>;
// auto ViewRecordIndexed(R const& index) const noexcept -> DFViewIndexed<T const, FldI const> requires DFValIndex<RecI>;


