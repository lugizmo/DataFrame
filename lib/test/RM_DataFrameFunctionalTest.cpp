// Filename: RM_DataFrameFunctionalTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the functional helpers and that views compose with std::ranges. These use a value (unique)
// index (ForEachOn* / Select* require DFValIndex), so the suite is not parameterized over configs.
// The following functions are tested here (with names of tests):
//
// ✅ ForEachOnField   - ForEachOnField<Func>(field, func) [const]  -> DFView<T[ const], RecI>
// ✅ ForEachOnRecord  - ForEachOnRecord<Func>(record, func) [const] -> DFView<T[ const], FldI>
// ✅ RangesComposition - View*/Select* compose with std::views (filter/transform)
//

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>

#include "lugizmo/DataFrame.h"

using namespace lgz;

namespace {

    using DF = DataFrame<int, int, int>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;

    // Builds a COL_COUNT x ROW_COUNT int frame; every record is populated with `row`,
    // so field f's column holds `row[f]` for all records.
    auto BuildFrame(std::span<int const> const row) -> DF
    {
        auto df = DF();
        for (int col = 0; col < COL_COUNT; ++col) df.AddField(col);
        for (int rec = 0; rec < ROW_COUNT; ++rec) df.AddRecordPopulated(rec, row);
        return df;
    }

} // namespace

/**
 *  @brief ForEachOnField applies a function over a field's column and returns the (mutable) view.
 *  @see   lgz::DataFrame.ForEachOnField<Func>(field, func)
 */
TEST(RM_DataFrameFunctional, ForEachOnField)
{
    constexpr auto row = std::array{1, 2, 3, 4, 5}; // field f column == f + 1

    {
        // non-const: mutates the column in place and returns a mutable view that stays in sync
        auto df = BuildFrame(row);

        auto view = df.ForEachOnField(1, [](auto& val) { val += 2; }); // field 1: 2 -> 4
        ASSERT_NE(view.Size(), 0u);
        EXPECT_TRUE(std::ranges::all_of(view, [](auto const val) { return val == 4; }));

        std::ranges::for_each(view, [](int& val) { val += 2; }); // 4 -> 6
        EXPECT_TRUE(std::ranges::all_of(view, [](auto const val) { return val == 6; }));
    }

    {
        // const: read-only application over the column
        auto const df = BuildFrame(row);
        auto const view = df.ForEachOnField(2, [](auto const& val) { EXPECT_EQ(val, 3); }); // field 2 == 3
        ASSERT_NE(view.Size(), 0u);
    }
}

/**
 *  @brief ForEachOnRecord applies a function over a record's row and returns the view.
 *  @see   lgz::DataFrame.ForEachOnRecord<Func>(record, func)
 */
TEST(RM_DataFrameFunctional, ForEachOnRecord)
{
    constexpr auto row = std::array{1, 2, 3, 4, 5};

    {
        // const: read-only application over the record's row, in field order
        auto const df = BuildFrame(row);
        auto const view = df.ForEachOnRecord(2, [](auto const& val) { EXPECT_GT(val, 0); });
        ASSERT_NE(view.Size(), 0u);

        std::size_t f = 0;
        std::ranges::for_each(view, [&](auto const val) { EXPECT_EQ(val, row.at(f++)); });
        EXPECT_EQ(f, row.size());
    }

    // TODO non-const (mutating) ForEachOnRecord once supported
}

/**
 *  @brief Views and selectors compose with std::ranges view adaptors (filter / transform).
 *  @see   lgz::DataFrame view/selector pipelines
 */
TEST(RM_DataFrameFunctional, RangesComposition)
{
    constexpr auto row = std::array{1, 2, 3, 4, 5};
    auto df            = BuildFrame(row);
    auto const& cdf    = df;

    auto const even = [](auto const v) { return v % 2 == 0; };

    {
        // ViewField / SelectField (field 1 column == 2, so the even filter keeps everything)
        auto v1 = df.ViewField(1) | std::views::filter(even);
        EXPECT_TRUE(std::ranges::all_of(v1, even));

        auto v2 = cdf | SelectField(1) | std::views::filter(even) | std::views::transform([](auto v) { return v; });
        EXPECT_TRUE(std::ranges::all_of(v2, even));
    }

    {
        // ViewRecord / SelectRecord (a record row is 1,2,3,4,5, so the even filter keeps 2 and 4)
        auto v1 = cdf.ViewRecord(0) | std::views::filter(even);
        EXPECT_TRUE(std::ranges::all_of(v1, even));

        auto v2 = df | SelectRecord(0) | std::views::filter(even);
        EXPECT_TRUE(std::ranges::all_of(v2, even));
    }

    {
        // indexed entries compose directly through their named members
        auto entries = df.ViewFieldIndexed(1) | std::views::filter([&even](auto const entry) { return even(entry.val); });
        EXPECT_TRUE(std::ranges::all_of(entries, [&even](auto const entry) { return even(entry.val); }));

        // named component views avoid projection lambdas when only one component is needed
        auto v1 = df.ViewFieldIndexed(1).Values() | std::views::filter(even);
        EXPECT_TRUE(std::ranges::all_of(v1, even));

        auto v2 = (cdf | SelectRecordIndexed(0)).Values() | std::views::filter(even);
        EXPECT_TRUE(std::ranges::all_of(v2, even));
    }
}
