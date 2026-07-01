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
// ✅ FieldViewSubrange         - FieldView(..., begin, end) key translation
// ✅ RecordViewSubrange        - RecordView(..., begin, end) key translation
// ✅ Size                      - DFView::Size
// ✅ Empty                     - DFView::Empty
// ✅ Contains                  - DFView::Contains
// ✅ IndexOperator             - DFView::operator[]
// ✅ CheckedIndexOperator      - DFView::operator()
// ✅ At                        - DFView::At
// ✅ Begin                     - DFView::begin
// ✅ End                       - DFView::end
// ✅ CBegin                    - DFView::cbegin
// ✅ CEnd                      - DFView::cend
// ✅ Front                     - DFView::Front
// ✅ Back                      - DFView::Back
// ✅ RBegin                    - DFView::rbegin
// ✅ REnd                      - DFView::rend
// ✅ CRBegin                   - DFView::crbegin
// ✅ CREnd                     - DFView::crend
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

/**
 * @brief Default construction produces an empty view.
 * @see   lugizmo::DFView::DFView
 */
TEST(DataframeView, Construct)
{
    using namespace lugizmo;

    auto const empty = DFView<int, DFUniqueIndex<int>>();
    ASSERT_TRUE(empty.Empty());
}

/**
 * @brief FieldView creates a half-open field subrange with translated key lookup.
 * @see   lugizmo::DFView::FieldView
 */
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

/**
 * @brief RecordView creates a half-open record subrange with translated key lookup.
 * @see   lugizmo::DFView::RecordView
 */
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

/**
 * @brief View iterators support the complete random-access operation set.
 * @see   lugizmo::DFView::Iterator
 */
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

/**
 * @brief Iterator movement follows the logical view stride.
 * @see   lugizmo::DFView::Iterator
 */
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

/**
 * @brief Iterators of an empty view form a valid empty range.
 * @see   lugizmo::DFView::begin
 * @see   lugizmo::DFView::end
 */
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

/**
 * @brief An lvalue view composes with standard range adaptors.
 * @see   lugizmo::DFView
 */
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

/**
 * @brief Standard adaptors safely own a temporary view wrapper.
 * @see   lugizmo::DFView
 */
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

/**
 * @brief Range adaptors preserve const-element access.
 * @see   lugizmo::DFView
 */
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

/**
 * @brief A mutable view models output_range and writes through to storage.
 * @see   lugizmo::DFView
 */
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

/**
 * @brief Iterators returned from temporary view wrappers remain usable.
 * @see   std::ranges::enable_borrowed_range
 */
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

namespace {

    class DataframeViewFunctionTest: public testing::Test
    {
    protected:
        using Index       = lugizmo::DFUniqueIndex<int>;
        using View        = lugizmo::DFView<int, Index>;
        using ConstView   = lugizmo::DFView<int const, Index>;
        using Matrix      = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
        using ConstMatrix = std::mdspan<int const, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;

        std::array<int, 8> values{0, 1,
                                  2, 3,
                                  4, 5,
                                  6, 7};
        Index records;
        Matrix matrix{values.data(), 4, 2};

        DataframeViewFunctionTest()
        {
            records.AddMultiple(std::array{10, 20, 30, 40});
        }

        [[nodiscard]] auto Field() noexcept -> View
        {
            return View::FieldView(matrix, &records, 1);
        }

        [[nodiscard]] auto ConstField() const noexcept -> ConstView
        {
            auto constMatrix = ConstMatrix(values.data(), 4, 2);
            return ConstView::FieldView(constMatrix, &records, 1);
        }
    };

} // namespace

/**
 * @brief Contains performs read-only keyed lookup on a const view object.
 * @see   lugizmo::DFView::Contains
 */
TEST_F(DataframeViewFunctionTest, Contains)
{
    auto const view = Field();
    EXPECT_TRUE(view.Contains(10));
    EXPECT_TRUE(view.Contains(40));
    EXPECT_FALSE(view.Contains(50));
}

/**
 * @brief Size reports the number of logical values in the view.
 * @see   lugizmo::DFView::Size
 */
TEST_F(DataframeViewFunctionTest, Size)
{
    EXPECT_EQ(Field().Size(), 4);
}

/**
 * @brief Empty distinguishes default-constructed and populated views.
 * @see   lugizmo::DFView::Empty
 */
TEST_F(DataframeViewFunctionTest, Empty)
{
    EXPECT_TRUE(View().Empty());
    EXPECT_FALSE(Field().Empty());
}

/**
 * @brief Positional indexing preserves element-based constness and mutability.
 * @see   lugizmo::DFView::operator[]
 */
TEST_F(DataframeViewFunctionTest, IndexOperator)
{
    auto const view = Field();
    static_assert(not std::is_const_v<std::remove_reference_t<decltype(view[0])>>);

    EXPECT_EQ(view[2], 5);
    view[2] = 50;
    EXPECT_EQ(values[5], 50);
}

/**
 * @brief Checked positional access returns an optional reference and reports misses.
 * @see   lugizmo::DFView::operator()
 */
TEST_F(DataframeViewFunctionTest, CheckedIndexOperator)
{
    auto const view = Field();
    auto value = view(1);

    ASSERT_TRUE(value.HasValue());
    EXPECT_EQ(value.Value(), 3);
    value.Value() = 30;
    EXPECT_EQ(values[3], 30);
    EXPECT_FALSE(view(4).HasValue());
}

/**
 * @brief At returns a pointer whose constness follows the element type.
 * @see   lugizmo::DFView::At
 */
TEST_F(DataframeViewFunctionTest, At)
{
    auto const view = Field();
    auto* value = view.At(30);
    static_assert(not std::is_const_v<std::remove_pointer_t<decltype(value)>>);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 5);
    *value = 50;
    EXPECT_EQ(values[5], 50);
    EXPECT_EQ(view.At(50), nullptr);

    auto const constView = ConstField();
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(constView.At(10))>>);
}

/**
 * @brief Begin points to the first logical value and preserves mutability.
 * @see   lugizmo::DFView::begin
 */
TEST_F(DataframeViewFunctionTest, Begin)
{
    auto const view = Field();
    static_assert(not std::is_const_v<std::remove_reference_t<decltype(*view.begin())>>);

    EXPECT_EQ(*view.begin(), 1);
    *view.begin() = 10;
    EXPECT_EQ(values[1], 10);
}

/**
 * @brief End terminates traversal after every logical value.
 * @see   lugizmo::DFView::end
 */
TEST_F(DataframeViewFunctionTest, End)
{
    auto const view = Field();
    EXPECT_EQ(view.end() - view.begin(), 4);
    EXPECT_EQ(*(view.end() - 1), 7);
}

/**
 * @brief CBegin starts traversal of a const-element view.
 * @see   lugizmo::DFView::cbegin
 */
TEST_F(DataframeViewFunctionTest, CBegin)
{
    auto const view = ConstField();
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*view.cbegin())>>);
    EXPECT_EQ(*view.cbegin(), 1);
}

/**
 * @brief CEnd terminates traversal of a const-element view.
 * @see   lugizmo::DFView::cend
 */
TEST_F(DataframeViewFunctionTest, CEnd)
{
    auto const view = ConstField();
    EXPECT_EQ(view.cend() - view.cbegin(), 4);
    EXPECT_EQ(*(view.cend() - 1), 7);
}

/**
 * @brief Front returns the first value or nullptr for an empty view.
 * @see   lugizmo::DFView::Front
 */
TEST_F(DataframeViewFunctionTest, Front)
{
    auto const view = Field();
    auto* front = view.Front();

    ASSERT_NE(front, nullptr);
    EXPECT_EQ(*front, 1);
    *front = 10;
    EXPECT_EQ(values[1], 10);
    EXPECT_EQ(View().Front(), nullptr);
}

/**
 * @brief Back returns the final value or nullptr for an empty view.
 * @see   lugizmo::DFView::Back
 */
TEST_F(DataframeViewFunctionTest, Back)
{
    auto const view = Field();
    auto* back = view.Back();

    ASSERT_NE(back, nullptr);
    EXPECT_EQ(*back, 7);
    *back = 70;
    EXPECT_EQ(values[7], 70);
    EXPECT_EQ(View().Back(), nullptr);
}

/**
 * @brief RBegin starts mutable reverse traversal at the final value.
 * @see   lugizmo::DFView::rbegin
 */
TEST_F(DataframeViewFunctionTest, RBegin)
{
    auto const view = Field();
    auto reverse = view.rbegin();

    ASSERT_NE(reverse, view.rend());
    EXPECT_EQ(*reverse, 7);
    *reverse = 70;
    EXPECT_EQ(values[7], 70);
}

/**
 * @brief REnd terminates reverse traversal before the first value.
 * @see   lugizmo::DFView::rend
 */
TEST_F(DataframeViewFunctionTest, REnd)
{
    auto const view = Field();
    EXPECT_EQ(view.rend() - view.rbegin(), 4);
    EXPECT_EQ(*(view.rend() - 1), 1);
    EXPECT_EQ(View().rbegin(), View().rend());
}

/**
 * @brief CRBegin starts const reverse traversal at the final value.
 * @see   lugizmo::DFView::crbegin
 */
TEST_F(DataframeViewFunctionTest, CRBegin)
{
    auto const view = ConstField();
    auto reverse = view.crbegin();

    static_assert(std::is_const_v<std::remove_reference_t<decltype(*reverse)>>);
    ASSERT_NE(reverse, view.crend());
    EXPECT_EQ(*reverse, 7);
}

/**
 * @brief CREnd terminates const reverse traversal before the first value.
 * @see   lugizmo::DFView::crend
 */
TEST_F(DataframeViewFunctionTest, CREnd)
{
    auto const view = ConstField();
    EXPECT_EQ(view.crend() - view.crbegin(), 4);
    EXPECT_EQ(*(view.crend() - 1), 1);
}
