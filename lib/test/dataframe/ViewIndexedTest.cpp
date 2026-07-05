// Filename: ViewIndexedTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the DFViewIndexed value/index view.
// The following functions are tested here (with names of tests):
//
// ✅ Construct             - DFViewIndexed()
// ✅ FieldView             - DFViewIndexed::FieldView
// ✅ RecordView            - DFViewIndexed::RecordView
// ✅ Size                  - DFViewIndexed::Size
// ✅ Empty                 - DFViewIndexed::Empty
// ✅ Values                - DFViewIndexed::Values
// ✅ Indices               - DFViewIndexed::Indices
// ✅ IndexOperator         - DFViewIndexed::operator[]
// ✅ CheckedIndexOperator  - DFViewIndexed::operator()
// ✅ StandardFront         - view_interface::front
// ✅ StandardBack          - view_interface::back
// ✅ Front                 - DFViewIndexed::Front
// ✅ Back                  - DFViewIndexed::Back
// ✅ Begin                 - DFViewIndexed::begin
// ✅ End                   - DFViewIndexed::end
// ✅ CBegin                - DFViewIndexed::cbegin
// ✅ CEnd                  - DFViewIndexed::cend
// ✅ RBegin                - DFViewIndexed::rbegin
// ✅ REnd                  - DFViewIndexed::rend
// ✅ CRBegin               - DFViewIndexed::crbegin
// ✅ CREnd                 - DFViewIndexed::crend
// ✅ Contains              - DFViewIndexed::Contains
// ✅ At                    - DFViewIndexed::At
// ✅ IteratorRandomAccess  - named-entry iterator arithmetic
// ✅ EmptyIterator         - empty iterator behavior
// ✅ RangeIndexEntry       - generated range-index values remain independent
//

#include "gtest/gtest.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <mdspan>
#include <ranges>
#include <type_traits>
#include <utility>

#include "lugizmo/dataframe/IndexRange.h"
#include "lugizmo/dataframe/IndexUnique.h"
#include "lugizmo/dataframe/ViewIndexed.h"

namespace {

    using Index      = lgz::DFUniqueIndex<int>;
    using View       = lgz::DFViewIndexed<int, Index>;
    using ConstView  = lgz::DFViewIndexed<int const, Index>;
    using Matrix     = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    using ConstMatrix = std::mdspan<int const, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;

    class ViewIndexedTest: public testing::Test
    {
    protected:
        std::array<int, 8> values{0, 1,
                                  2, 3,
                                  4, 5,
                                  6, 7};
        Index records;
        Index fields;
        Matrix matrix{values.data(), 4, 2};

        ViewIndexedTest()
        {
            records.AddMultiple(std::array{10, 20, 30, 40});
            fields.AddMultiple(std::array{100, 200});
        }

        [[nodiscard]] auto Field() noexcept -> View
        {
            return View::FieldView(matrix, &records, 1, records.Keys());
        }

        [[nodiscard]] auto Record() noexcept -> View
        {
            return View::RecordView(matrix, &fields, 2, fields.Keys());
        }

        [[nodiscard]] auto ConstField() const noexcept -> ConstView
        {
            auto constMatrix = ConstMatrix(values.data(), 4, 2);
            return ConstView::FieldView(constMatrix, &records, 1, records.Keys());
        }
    };

} // namespace

/**
 * @brief Default construction produces a valid indexed-view wrapper.
 * @see   lgz::DFViewIndexed::DFViewIndexed
 */
TEST_F(ViewIndexedTest, Construct)
{
    static_assert(std::default_initializable<View>);
    auto const view = View();
    static_cast<void>(view);
    SUCCEED();
}

/**
 * @brief FieldView pairs one field's values with the record index.
 * @see   lgz::DFViewIndexed::FieldView
 */
TEST_F(ViewIndexedTest, FieldView)
{
    auto view = Field();
    constexpr auto expectedValues = std::array{1, 3, 5, 7};
    constexpr auto expectedIndices = std::array{10, 20, 30, 40};

    std::size_t position = 0;
    for(auto [key, val] : view)
    {
        EXPECT_EQ(val, expectedValues[position]);
        EXPECT_EQ(key, expectedIndices[position]);
        ++position;
    }
    EXPECT_EQ(position, expectedValues.size());
}

/**
 * @brief RecordView pairs one record's values with the field index.
 * @see   lgz::DFViewIndexed::RecordView
 */
TEST_F(ViewIndexedTest, RecordView)
{
    auto view = Record();
    auto first = view[0];
    auto second = view[1];

    EXPECT_EQ(first.val, 4);
    EXPECT_EQ(first.key, 100);
    EXPECT_EQ(second.val, 5);
    EXPECT_EQ(second.key, 200);
}

/**
 * @brief Size reports the number of paired value/index entries.
 * @see   lgz::DFViewIndexed::Size
 */
TEST_F(ViewIndexedTest, Size)
{
    EXPECT_EQ(Field().Size(), 4);
    EXPECT_EQ(Record().Size(), 2);
}

/**
 * @brief Empty distinguishes default-constructed and populated indexed views.
 * @see   lgz::DFViewIndexed::Empty
 */
TEST_F(ViewIndexedTest, Empty)
{
    EXPECT_TRUE(View().Empty());
    EXPECT_FALSE(Field().Empty());
}

/**
 * @brief Values exposes the writable dataframe-value component.
 * @see   lgz::DFViewIndexed::Values
 */
TEST_F(ViewIndexedTest, Values)
{
    auto view = Field();
    auto vals = view.Values();
    constexpr auto expected = std::array{1, 3, 5, 7};

    EXPECT_TRUE(std::ranges::equal(vals, expected));
    vals[1] = 30;
    EXPECT_EQ(view[1].val, 30);
}

/**
 * @brief Indices exposes the read-only index component in entry order.
 * @see   lgz::DFViewIndexed::Indices
 */
TEST_F(ViewIndexedTest, Indices)
{
    auto const indices = Field().Indices();
    constexpr auto expected = std::array{10, 20, 30, 40};
    EXPECT_TRUE(std::ranges::equal(indices, expected));
}

/**
 * @brief Positional indexing returns the paired value and index entry.
 * @see   lgz::DFViewIndexed::operator[]
 */
TEST_F(ViewIndexedTest, IndexOperator)
{
    auto const view = Field();
    auto entry = view[2];

    static_assert(not std::is_const_v<std::remove_reference_t<decltype(entry.val)>>);
    EXPECT_EQ(entry.val, 5);
    EXPECT_EQ(entry.key, 30);
    entry.val = 50;
    EXPECT_EQ(values[5], 50);
}

/**
 * @brief Checked positional access returns an optional entry and reports misses.
 * @see   lgz::DFViewIndexed::operator()
 */
TEST_F(ViewIndexedTest, CheckedIndexOperator)
{
    auto view = Field();
    auto entry = view(1);

    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->val, 3);
    EXPECT_EQ(entry->key, 20);
    entry->val = 30;
    EXPECT_EQ(values[3], 30);
    EXPECT_FALSE(view(4).has_value());
}

/**
 * @brief The inherited standard front function returns the first entry.
 * @see   std::ranges::view_interface::front
 */
TEST_F(ViewIndexedTest, StandardFront)
{
    auto view = Field();
    auto entry = view.front();

    EXPECT_EQ(entry.val, 1);
    EXPECT_EQ(entry.key, 10);
    entry.val = 10;
    EXPECT_EQ(values[1], 10);
}

/**
 * @brief The inherited standard back function returns the final entry.
 * @see   std::ranges::view_interface::back
 */
TEST_F(ViewIndexedTest, StandardBack)
{
    auto view = Field();
    auto entry = view.back();

    EXPECT_EQ(entry.val, 7);
    EXPECT_EQ(entry.key, 40);
    entry.val = 70;
    EXPECT_EQ(values[7], 70);
}

/**
 * @brief Front returns the first entry or an empty optional for an empty view.
 * @see   lgz::DFViewIndexed::Front
 */
TEST_F(ViewIndexedTest, Front)
{
    auto entry = Field().Front();

    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->val, 1);
    EXPECT_EQ(entry->key, 10);
    entry->val = 10;
    EXPECT_EQ(values[1], 10);
    EXPECT_FALSE(View().Front().has_value());
}

/**
 * @brief Back returns the final entry or an empty optional for an empty view.
 * @see   lgz::DFViewIndexed::Back
 */
TEST_F(ViewIndexedTest, Back)
{
    auto entry = Field().Back();

    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->val, 7);
    EXPECT_EQ(entry->key, 40);
    entry->val = 70;
    EXPECT_EQ(values[7], 70);
    EXPECT_FALSE(View().Back().has_value());
}

/**
 * @brief Begin points to the first paired entry and preserves value mutability.
 * @see   lgz::DFViewIndexed::begin
 */
TEST_F(ViewIndexedTest, Begin)
{
    auto const view = Field();
    auto first = *view.begin();

    EXPECT_EQ(first.val, 1);
    EXPECT_EQ(first.key, 10);
    first.val = 11;
    EXPECT_EQ(values[1], 11);
}

/**
 * @brief End terminates traversal after every paired entry.
 * @see   lgz::DFViewIndexed::end
 */
TEST_F(ViewIndexedTest, End)
{
    auto const view = Field();
    EXPECT_EQ(view.end() - view.begin(), 4);
    EXPECT_EQ((view.end() - 1)->val, 7);
    EXPECT_EQ((view.end() - 1)->key, 40);
}

/**
 * @brief CBegin starts traversal of a const-element indexed view.
 * @see   lgz::DFViewIndexed::cbegin
 */
TEST_F(ViewIndexedTest, CBegin)
{
    auto const view = ConstField();
    auto first = *view.cbegin();

    static_assert(std::is_const_v<std::remove_reference_t<decltype(first.val)>>);
    EXPECT_EQ(first.val, 1);
    EXPECT_EQ(first.key, 10);
}

/**
 * @brief CEnd terminates traversal of a const-element indexed view.
 * @see   lgz::DFViewIndexed::cend
 */
TEST_F(ViewIndexedTest, CEnd)
{
    auto const view = ConstField();
    EXPECT_EQ(view.cend() - view.cbegin(), 4);
    EXPECT_EQ((view.cend() - 1)->val, 7);
    EXPECT_EQ((view.cend() - 1)->key, 40);
}

/**
 * @brief RBegin starts mutable reverse traversal at the final paired entry.
 * @see   lgz::DFViewIndexed::rbegin
 */
TEST_F(ViewIndexedTest, RBegin)
{
    auto view = Field();
    auto reverse = view.rbegin();

    ASSERT_NE(reverse, view.rend());
    EXPECT_EQ(reverse->val, 7);
    EXPECT_EQ(reverse->key, 40);
    reverse->val = 70;
    EXPECT_EQ(values[7], 70);
}

/**
 * @brief REnd terminates reverse traversal before the first paired entry.
 * @see   lgz::DFViewIndexed::rend
 */
TEST_F(ViewIndexedTest, REnd)
{
    auto view = Field();
    EXPECT_EQ(view.rend() - view.rbegin(), 4);
    EXPECT_EQ((view.rend() - 1)->val, 1);
    EXPECT_EQ((view.rend() - 1)->key, 10);

    auto empty = View();
    EXPECT_EQ(empty.rbegin(), empty.rend());
}

/**
 * @brief CRBegin starts const reverse traversal at the final paired entry.
 * @see   lgz::DFViewIndexed::crbegin
 */
TEST_F(ViewIndexedTest, CRBegin)
{
    auto const view = ConstField();
    auto reverse = view.crbegin();

    static_assert(std::is_const_v<std::remove_reference_t<decltype(reverse->val)>>);
    ASSERT_NE(reverse, view.crend());
    EXPECT_EQ(reverse->val, 7);
    EXPECT_EQ(reverse->key, 40);
}

/**
 * @brief CREnd terminates const reverse traversal before the first paired entry.
 * @see   lgz::DFViewIndexed::crend
 */
TEST_F(ViewIndexedTest, CREnd)
{
    auto const view = ConstField();
    EXPECT_EQ(view.crend() - view.crbegin(), 4);
    EXPECT_EQ((view.crend() - 1)->val, 1);
    EXPECT_EQ((view.crend() - 1)->key, 10);
}

/**
 * @brief Contains performs read-only keyed lookup on a const view object.
 * @see   lgz::DFViewIndexed::Contains
 */
TEST_F(ViewIndexedTest, Contains)
{
    auto const view = Field();
    static_assert(noexcept(view.Contains(10)));

    EXPECT_TRUE(view.Contains(10));
    EXPECT_TRUE(view.Contains(40));
    EXPECT_FALSE(view.Contains(50));
}

/**
 * @brief At returns a pointer whose constness follows the element type.
 * @see   lgz::DFViewIndexed::At
 */
TEST_F(ViewIndexedTest, At)
{
    auto const view = Field();
    auto* value = view.At(30);
    static_assert(not std::is_const_v<std::remove_pointer_t<decltype(value)>>);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 5);
    *value = 50;
    EXPECT_EQ(values[5], 50);
    EXPECT_EQ(view.At(50), nullptr);

    [[maybe_unused]] auto const constView = ConstField();
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(constView.At(10))>>);
}

/**
 * @brief Indexed iterators keep values and indices synchronized during random access.
 * @see   lgz::DFViewIndexed::Iterator
 */
TEST_F(ViewIndexedTest, IteratorRandomAccess)
{
    auto view = Field();
    using Iterator = decltype(view.begin());
    static_assert(std::random_access_iterator<Iterator>);
    static_assert(std::ranges::random_access_range<View>);

    auto const begin = view.begin();
    auto const end   = view.end();
    EXPECT_EQ(end - begin, 4);
    EXPECT_EQ(begin - end, -4);
    EXPECT_EQ((begin + 2)->val, 5);
    EXPECT_EQ((2 + begin)->key, 30);
    EXPECT_EQ((end - 1)->val, 7);
    EXPECT_LT(begin, end);
    EXPECT_LE(begin, end);
    EXPECT_GT(end, begin);
    EXPECT_GE(end, begin);

    auto iterator = begin;
    EXPECT_EQ((iterator++)->key, 10);
    EXPECT_EQ(iterator->key, 20);
    EXPECT_EQ((++iterator)->key, 30);
    EXPECT_EQ((iterator--)->key, 30);
    EXPECT_EQ((--iterator)->key, 10);
    iterator += 3;
    EXPECT_EQ(iterator->key, 40);
    iterator -= 2;
    EXPECT_EQ(iterator->key, 20);
    EXPECT_EQ(iterator[1].key, 30);
}

/**
 * @brief Iterators of an empty indexed view form a valid empty range.
 * @see   lgz::DFViewIndexed::begin
 * @see   lgz::DFViewIndexed::end
 */
TEST_F(ViewIndexedTest, EmptyIterator)
{
    auto const view = View();
    EXPECT_EQ(view.begin(), view.end());
    EXPECT_EQ(view.end() - view.begin(), 0);
    EXPECT_EQ(std::ranges::distance(view), 0);
}

/**
 * @brief Entries from a generated range index retain independent index values.
 * @see   lgz::DFViewIndexed::Entry
 */
TEST(ViewIndexed, RangeIndexEntry)
{
    using RangeIndex = lgz::DFRangeIndex<int>;
    using RangeView  = lgz::DFViewIndexed<int, RangeIndex>;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    auto matrix = Matrix(values.data(), 4, 2);
    auto records = RangeIndex(10, 18, 2);
    auto view = RangeView::FieldView(matrix, &records, 0, records.Keys());

    auto const first  = view[0];
    auto const second = view[1];
    EXPECT_EQ(first.key, 10);
    EXPECT_EQ(second.key, 12);
    EXPECT_EQ(first.key, 10);
}
