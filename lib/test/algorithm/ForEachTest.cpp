// Filename: ForEachTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the free ForEach algorithm overloads.
//
// ✅ DataFrame - ForEach(dataframe, function)
// ✅ Range     - ForEach(range, function)
//

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <type_traits>

#include "lugizmo/dataframe/DataFrame.h"

using namespace lgz;

namespace {

    using DF = DataFrame<int, int, int>;

    constexpr int FIELD_COUNT  = 5;
    constexpr int RECORD_COUNT = 10;

    auto BuildFrame(std::span<int const> const record) -> DF
    {
        auto dataframe = DF();
        for (int field = 0; field < FIELD_COUNT; ++field) dataframe.AddField(field);
        for (int row = 0; row < RECORD_COUNT; ++row) dataframe.AddRecordPopulated(row, record);
        return dataframe;
    }

} // namespace

/**
 * @brief ForEach traverses mutable and const DataFrames in physical storage order.
 * @see   lgz::ForEach
 */
TEST(ForEach, DataFrame)
{
    constexpr auto record = std::array{1, 2, 3, 4, 5};
    auto dataframe        = BuildFrame(record);

    auto result = ForEach(dataframe, [](int& value) { ++value; });
    EXPECT_EQ(result.in, dataframe.Values().end());
    ASSERT_NE(dataframe.GetValue(0, 0), nullptr);
    ASSERT_NE(dataframe.GetValue(4, 0), nullptr);
    EXPECT_EQ(*dataframe.GetValue(0, 0), 2);
    EXPECT_EQ(*dataframe.GetValue(4, 0), 6);

    auto visited = std::size_t{0};
    using ConstValues = decltype(std::as_const(dataframe).Values());
    static_assert(std::is_same_v<std::ranges::range_reference_t<ConstValues>, int const&>);
    ForEach(std::as_const(dataframe), [&visited](int const& value) {
        EXPECT_GT(value, 0);
        ++visited;
    });
    EXPECT_EQ(visited, dataframe.Size());
}

/**
 * @brief ForEach applies uniformly to ordinary ranges, DataFrame views, and slices.
 * @see   lgz::ForEach
 */
TEST(ForEach, Range)
{
    auto values = std::array{1, 2, 3};
    ForEach(values, [](int& value) { ++value; });
    EXPECT_EQ(values, (std::array{2, 3, 4}));

    constexpr auto record = std::array{1, 2, 3, 4, 5};
    auto dataframe        = BuildFrame(record);

    auto field = dataframe.ViewField(1);
    ForEach(field, [](int& value) { value += 2; });
    EXPECT_TRUE(std::ranges::all_of(field, [](int const value) { return value == 4; }));

    auto slice = dataframe.Slice({0, 2}, {0, 1});
    ForEach(slice, [](int& value) { value *= 2; });
    EXPECT_EQ((slice[0, 0]), 2);
    EXPECT_EQ((slice[1, 1]), 6);
}
