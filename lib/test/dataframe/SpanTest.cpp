// Filename: SpanTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the contiguous DFSpan DataFrame view.
//
// ✅ DefaultConstruction - empty default state
// ✅ RecordView          - contiguous positional access and iteration
// ✅ CheckedAccess       - checked positional operator()
// ✅ KeyAccess           - Contains / At
// ✅ RecordSubrange      - contiguous subrange and translated key lookup
// ✅ Constness           - wrapper constness and element constness
// ✅ RangeAdaptor        - standard range adaptor composition
//

#include "gtest/gtest.h"

#include <array>
#include <mdspan>
#include <ranges>
#include <type_traits>

#include "lugizmo/dataframe/Span.h"

namespace {

    using Index       = lugizmo::DFUniqueIndex<int>;
    using Span        = lugizmo::DFSpan<int, Index>;
    using ConstSpan   = lugizmo::DFSpan<int const, Index>;
    using Matrix      = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    using ConstMatrix = std::mdspan<int const, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;

    auto Fields() -> Index
    {
        auto fields = Index();
        fields.AddMultiple(std::array{10, 20, 30, 40});
        return fields;
    }

} // namespace

/**
 * @brief A default DFSpan is an empty contiguous range without an index.
 * @see   lugizmo::DFSpan::DFSpan
 */
TEST(DataframeSpan, DefaultConstruction)
{
    auto const span = Span();
    EXPECT_TRUE(span.Empty());
    EXPECT_EQ(span.Size(), 0);
    EXPECT_EQ(span.begin(), span.end());
    EXPECT_EQ(span.Front(), nullptr);
    EXPECT_EQ(span.Back(), nullptr);
}

/**
 * @brief RecordView exposes one row as adjacent values and pointer iterators.
 * @see   lugizmo::DFSpan::RecordView
 */
TEST(DataframeSpan, RecordView)
{
    auto values = std::array{0, 1, 2, 3,
                             4, 5, 6, 7,
                             8, 9, 10, 11};
    auto fields = Fields();
    auto span = Span::RecordView(Matrix(values.data(), 3, 4), &fields, 1);

    static_assert(std::ranges::contiguous_range<decltype(span)>);
    EXPECT_EQ(span.Size(), 4);
    EXPECT_FALSE(span.Empty());
    EXPECT_EQ(std::ranges::data(span), values.data() + 4);
    EXPECT_EQ(span[0], 4);
    EXPECT_EQ(span[3], 7);
    EXPECT_EQ(*span.Front(), 4);
    EXPECT_EQ(*span.Back(), 7);
    EXPECT_EQ(std::ranges::distance(span), 4);
}

/**
 * @brief operator() returns a value pointer only for positions inside the span.
 * @see   lugizmo::DFSpan::operator()
 */
TEST(DataframeSpan, CheckedAccess)
{
    auto values = std::array{1, 2, 3, 4};
    auto fields = Fields();
    auto span = Span::RecordView(Matrix(values.data(), 1, 4), &fields, 0);

    ASSERT_NE(span(2), nullptr);
    EXPECT_EQ(*span(2), 3);
    EXPECT_EQ(span(4), nullptr);
}

/**
 * @brief Contains and At resolve field keys without losing contiguous storage.
 * @see   lugizmo::DFSpan::Contains
 * @see   lugizmo::DFSpan::At
 */
TEST(DataframeSpan, KeyAccess)
{
    auto values = std::array{1, 2, 3, 4};
    auto fields = Fields();
    auto span = Span::RecordView(Matrix(values.data(), 1, 4), &fields, 0);

    EXPECT_TRUE(span.Contains(30));
    EXPECT_FALSE(span.Contains(99));
    ASSERT_NE(span.At(30), nullptr);
    EXPECT_EQ(*span.At(30), 3);
    EXPECT_EQ(span.At(99), nullptr);
}

/**
 * @brief A record subrange stays contiguous while key lookup accounts for its field offset.
 * @see   lugizmo::DFSpan::RecordView
 */
TEST(DataframeSpan, RecordSubrange)
{
    auto values = std::array{0, 1, 2, 3,
                             4, 5, 6, 7};
    auto fields = Fields();
    auto span = Span::RecordView(Matrix(values.data(), 2, 4), &fields, 1, 1, 3);

    EXPECT_EQ(span.Size(), 2);
    EXPECT_EQ(std::ranges::data(span), values.data() + 5);
    EXPECT_EQ(span[0], 5);
    EXPECT_FALSE(span.Contains(10));
    EXPECT_TRUE(span.Contains(20));
    EXPECT_TRUE(span.Contains(30));
    EXPECT_FALSE(span.Contains(40));
}

/**
 * @brief Wrapper constness preserves mutable elements while const element types remain read-only.
 * @see   lugizmo::DFSpan
 */
TEST(DataframeSpan, Constness)
{
    auto values = std::array{1, 2, 3, 4};
    auto fields = Fields();
    auto const mutableSpan = Span::RecordView(Matrix(values.data(), 1, 4), &fields, 0);
    *mutableSpan.begin() = 9;
    EXPECT_EQ(values[0], 9);

    auto const readOnlySpan = ConstSpan::RecordView(ConstMatrix(values.data(), 1, 4), &fields, 0);
    static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(readOnlySpan)>, int const&>);
    EXPECT_EQ(readOnlySpan[0], 9);
}

/**
 * @brief DFSpan composes directly with standard range adaptor closures.
 * @see   lugizmo::DFSpan
 */
TEST(DataframeSpan, RangeAdaptor)
{
    auto values = std::array{1, 2, 3, 4};
    auto fields = Fields();
    auto span = Span::RecordView(Matrix(values.data(), 1, 4), &fields, 0);
    auto transformed = span | std::views::transform([](int const value) { return value * 2; });

    EXPECT_TRUE(std::ranges::equal(transformed, std::array{2, 4, 6, 8}));
}
