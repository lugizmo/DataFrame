// Filename: DFViewTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the DFView lightweight view over a single field / record.
// The following functions are tested here (with names of tests):
//
// ✅ Construct                 - DFView() / Empty
// ✅ IndexOperatorOptionalRef  - operator()(key) -> OptionalRef / At
// ✅ FieldViewSubrange         - FieldView(..., begin, end) key translation
// ✅ RecordViewSubrange        - RecordView(..., begin, end) key translation
// ✅ IteratorRandomAccess      - random-access operations on strided and contiguous views
// ✅ IteratorStride            - iterator movement follows the mdspan stride
// ✅ EmptyIterator             - safe iterator arithmetic on empty views
// ✅ RangeAdaptorLValue        - standard filter/transform composition on an lvalue view
// ✅ RangeAdaptorTemporary     - standard adaptor composition owns a temporary view wrapper
// ✅ RangeAdaptorConstElements - standard adaptors preserve read-only elements
// ✅ OutputRange               - standard algorithms mutate through a mutable view
// ✅ BorrowedRange             - algorithms return usable iterators from temporary wrappers
//

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <mdspan>
#include <ranges>
#include <type_traits>

#include "lugizmo/DataFrame.h"
#include "lugizmo/dataframe/IndexRange.h"
#include "lugizmo/dataframe/IndexUnique.h"
#include "lugizmo/dataframe/View.h"

TEST(DataframeView, Construct)
{
    using namespace lugizmo;

    auto const empty = DFView<int, DFUniqueIndex<int>>();
    ASSERT_TRUE(empty.Empty());
}

TEST(DataframeView, IndexOperatorOptionalRef)
{
    using namespace lugizmo;

    auto empty = DFView<int, DFUniqueIndex<int>>();
    auto miss  = empty(0);
    ASSERT_FALSE(miss.HasValue());

    auto df   = DataFrame<int, int, int>::FromFieldsAndRecords({0}, {0}, {{7}});
    auto view = df.ViewField(0);
    auto hit  = view(0);

    ASSERT_TRUE(hit.HasValue());
    ASSERT_EQ(hit.Value(), 7);

    hit.Value() = 9;
    ASSERT_EQ(*view.At(0), 9);
}

TEST(DataframeView, FieldViewSubrange)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto const records = DFRangeIndex(10, 18, 2); // 10, 12, 14, 16
    auto view = DFView<int, DFRangeIndex<int>>::FieldView(matrix, &records, 1, 1, 3);

    ASSERT_EQ(view.Size(), 2);
    EXPECT_EQ(view[0], 3);
    EXPECT_EQ(view[1], 5);

    EXPECT_FALSE(view.Contains(10));
    EXPECT_TRUE(view.Contains(12));
    EXPECT_TRUE(view.Contains(14));
    EXPECT_FALSE(view.Contains(16));

    ASSERT_NE(view.At(12), nullptr);
    ASSERT_NE(view.At(14), nullptr);
    EXPECT_EQ(*view.At(12), 3);
    EXPECT_EQ(*view.At(14), 5);
    EXPECT_EQ(view.At(10), nullptr);
    EXPECT_EQ(view.At(16), nullptr);
}

TEST(DataframeView, RecordViewSubrange)
{
    using namespace lugizmo;

    auto values = std::array{0, 1, 2, 3,
                             4, 5, 6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 2, 4);

    auto fields = DFUniqueIndex<int>();
    fields.AddMultiple(std::array{10, 20, 30, 40});
    auto view = DFView<int, DFUniqueIndex<int>>::RecordView(matrix, &fields, 1, 1, 3);

    ASSERT_EQ(view.Size(), 2);
    EXPECT_EQ(view[0], 5);
    EXPECT_EQ(view[1], 6);

    EXPECT_FALSE(view.Contains(10));
    EXPECT_TRUE(view.Contains(20));
    EXPECT_TRUE(view.Contains(30));
    EXPECT_FALSE(view.Contains(40));

    ASSERT_NE(view.At(20), nullptr);
    ASSERT_NE(view.At(30), nullptr);
    EXPECT_EQ(*view.At(20), 5);
    EXPECT_EQ(*view.At(30), 6);
    EXPECT_EQ(view.At(10), nullptr);
    EXPECT_EQ(view.At(40), nullptr);
}

TEST(DataframeView, IteratorRandomAccess)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30, 40});
    auto field = DFView<int, DFUniqueIndex<int>>::FieldView(matrix, &records, 1);

    auto const begin = field.begin();
    auto const end   = field.end();

    EXPECT_EQ(end - begin, 4);
    EXPECT_EQ(begin - end, -4);
    EXPECT_EQ(std::ranges::distance(field), 4);
    EXPECT_EQ(*begin, 1);
    EXPECT_EQ(begin.operator->(), &values[1]);
    EXPECT_EQ(begin[2], 5);
    EXPECT_EQ(*(begin + 3), 7);
    EXPECT_EQ(*(3 + begin), 7);
    EXPECT_EQ(*(end - 1), 7);
    EXPECT_LT(begin, end);
    EXPECT_LE(begin, end);
    EXPECT_GT(end, begin);
    EXPECT_GE(end, begin);
    EXPECT_NE(begin, end);

    auto iterator = begin;
    EXPECT_EQ(*iterator++, 1);
    EXPECT_EQ(*iterator, 3);
    EXPECT_EQ(*++iterator, 5);
    EXPECT_EQ(*iterator--, 5);
    EXPECT_EQ(*iterator, 3);
    EXPECT_EQ(*--iterator, 1);
    iterator += 3;
    EXPECT_EQ(*iterator, 7);
    iterator -= 2;
    EXPECT_EQ(*iterator, 3);

    auto fields = DFUniqueIndex<int>();
    fields.AddMultiple(std::array{10, 20});
    auto record = DFView<int, DFUniqueIndex<int>>::RecordView(matrix, &fields, 2);

    EXPECT_EQ(record.end() - record.begin(), 2);
    EXPECT_EQ(std::ranges::distance(record), 2);
    EXPECT_EQ(record.begin()[0], 4);
    EXPECT_EQ(record.begin()[1], 5);
}

TEST(DataframeView, IteratorStride)
{
    using namespace lugizmo;

    auto values = std::array{0,  1,  2,  3,
                             4,  5,  6,  7,
                             8,  9, 10, 11};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 3, 4);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30});
    auto field = DFView<int, DFUniqueIndex<int>>::FieldView(matrix, &records, 2);

    auto const first  = field.begin();
    auto const second = first + 1;
    auto const third  = first + 2;

    EXPECT_EQ(second.operator->() - first.operator->(), 4);
    EXPECT_EQ(third.operator->() - second.operator->(), 4);
    EXPECT_EQ(*first, 2);
    EXPECT_EQ(*second, 6);
    EXPECT_EQ(*third, 10);

    *second = 60;
    EXPECT_EQ(values[6], 60);
    EXPECT_EQ(values[3], 3);
    EXPECT_EQ(values[7], 7);
}

TEST(DataframeView, EmptyIterator)
{
    using namespace lugizmo;

    auto const empty = DFView<int, DFUniqueIndex<int>>();
    EXPECT_EQ(empty.begin(), empty.end());
    EXPECT_EQ(empty.end() - empty.begin(), 0);
    EXPECT_EQ(empty.begin() + 0, empty.end());
    EXPECT_EQ(std::ranges::distance(empty), 0);

    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(static_cast<int*>(nullptr), 0, 2);
    auto records = DFUniqueIndex<int>();
    auto const field = DFView<int, DFUniqueIndex<int>>::FieldView(matrix, &records, 1);

    EXPECT_TRUE(field.Empty());
    EXPECT_EQ(field.begin(), field.end());
    EXPECT_EQ(field.end() - field.begin(), 0);
    EXPECT_EQ(std::ranges::distance(field), 0);
}

TEST(DataframeView, RangeAdaptorLValue)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30, 40});
    auto field = DFView<int, DFUniqueIndex<int>>::FieldView(matrix, &records, 1);

    auto adapted = field |
                   std::views::filter([](int const value) { return value > 1; }) |
                   std::views::transform([](int const value) { return value * 2; });
    constexpr auto expected = std::array{6, 10, 14};

    EXPECT_TRUE(std::ranges::equal(adapted, expected));
}

TEST(DataframeView, RangeAdaptorTemporary)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30, 40});

    auto adapted = DFView<int, DFUniqueIndex<int>>::FieldView(matrix, &records, 0) |
                   std::views::filter([](int const value) { return value >= 4; });
    constexpr auto expected = std::array{4, 6};

    EXPECT_TRUE(std::ranges::equal(adapted, expected));
}

TEST(DataframeView, RangeAdaptorConstElements)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int const, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30, 40});
    auto field = DFView<int const, DFUniqueIndex<int>>::FieldView(matrix, &records, 1);
    auto adapted = field | std::views::filter([](int const value) { return value < 7; });

    static_assert(std::is_const_v<std::remove_reference_t<std::ranges::range_reference_t<decltype(adapted)>>>);
    constexpr auto expected = std::array{1, 3, 5};
    EXPECT_TRUE(std::ranges::equal(adapted, expected));
}

TEST(DataframeView, OutputRange)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30, 40});
    auto field = DFView<int, DFUniqueIndex<int>>::FieldView(matrix, &records, 0);

    static_assert(std::ranges::output_range<decltype(field), int>);
    static_assert(!std::ranges::output_range<DFView<int const, DFUniqueIndex<int>>, int>);
    std::ranges::fill(field, 42);

    constexpr auto expected = std::array{42, 1,
                                         42, 3,
                                         42, 5,
                                         42, 7};
    EXPECT_EQ(values, expected);
}

TEST(DataframeView, BorrowedRange)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto const matrix = Matrix(values.data(), 4, 2);

    auto records = DFUniqueIndex<int>();
    records.AddMultiple(std::array{10, 20, 30, 40});
    using View = DFView<int, DFUniqueIndex<int>>;

    auto found = std::ranges::find(View::FieldView(matrix, &records, 1), 5);
    static_assert(std::same_as<decltype(found), View::Iterator>);

    EXPECT_EQ(*found, 5);
}
