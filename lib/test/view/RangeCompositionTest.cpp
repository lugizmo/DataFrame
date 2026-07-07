// Filename: RangeCompositionTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test that DataFrame views and selectors compose with standard range adaptors.
//
// ✅ RangeAdaptors - View*/Select* compose with filter and transform
//

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>

#include "lugizmo/dataframe/DataFrame.h"

using namespace lgz;

namespace {

    using DF = DataFrame<int, int, int>;

    auto BuildFrame(std::span<int const> const record) -> DF
    {
        auto dataframe = DF();
        for (int field = 0; field < static_cast<int>(record.size()); ++field) dataframe.AddField(field);
        for (int row = 0; row < 10; ++row) dataframe.AddRecordPopulated(row, record);
        return dataframe;
    }

} // namespace

/**
 * @brief DataFrame views and selector results compose with standard range adaptors.
 * @see   lgz::DFView
 * @see   lgz::DFViewIndexed
 */
TEST(DataFrameRangeComposition, RangeAdaptors)
{
    constexpr auto record = std::array{1, 2, 3, 4, 5};
    auto dataframe        = BuildFrame(record);
    auto const& constDataframe = dataframe;

    auto const even = [](auto const value) { return value % 2 == 0; };

    auto field = dataframe.ViewField(1) | std::views::filter(even);
    EXPECT_TRUE(std::ranges::all_of(field, even));

    auto selectedField = constDataframe | SelectField(1) | std::views::filter(even)
                       | std::views::transform([](auto const value) { return value; });
    EXPECT_TRUE(std::ranges::all_of(selectedField, even));

    auto recordView = constDataframe.ViewRecord(0) | std::views::filter(even);
    EXPECT_TRUE(std::ranges::all_of(recordView, even));

    auto selectedRecord = dataframe | SelectRecord(0) | std::views::filter(even);
    EXPECT_TRUE(std::ranges::all_of(selectedRecord, even));

    auto entries = dataframe.ViewFieldIndexed(1)
                 | std::views::filter([&even](auto const entry) { return even(entry.val); });
    EXPECT_TRUE(std::ranges::all_of(entries, [&even](auto const entry) { return even(entry.val); }));

    auto values = dataframe.ViewFieldIndexed(1).Values() | std::views::filter(even);
    EXPECT_TRUE(std::ranges::all_of(values, even));

    auto selectedValues = (constDataframe | SelectRecordIndexed(0)).Values() | std::views::filter(even);
    EXPECT_TRUE(std::ranges::all_of(selectedValues, even));
}
