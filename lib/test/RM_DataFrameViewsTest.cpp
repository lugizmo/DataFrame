// Filename: RM_DataframeViewsTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions viewing dataframe data. The value/sequence overloads (requires DFValIndex /
// DFSeqIndex) are selected by the index type. Cell (field f, record r) holds `r * FLD_COUNT + f`.
//
// Typed across every index configuration (suite RM_DataframeViews):
//
// ✅ ViewField           - ViewField(field) [const]              -> DFView<T[ const], RecI>
// ✅ ViewRecord          - ViewRecord(record) [const]            -> DFView<T[ const], FldI>
// ✅ ViewFieldIndexed    - ViewFieldIndexed(field) [const]       -> DFViewIndexed<T[ const], RecI const>
// ✅ ViewRecordIndexed   - ViewRecordIndexed(record) [const]     -> DFViewIndexed<T[ const], FldI const>
// ✅ Contains            - DFView::Contains(key) const
// ✅ IndexOperator       - DFView::operator[]
// ✅ CheckedIndexOperator- DFView::operator()
// ✅ At                  - DFView::At
// ✅ TryAt               - DFView::TryAt
// ✅ IteratorConstness   - DFView::begin element access
// ✅ Front               - DFView::Front
// ✅ Back                - DFView::Back
// ✅ RBegin              - DFView::rbegin
// ✅ REnd                - DFView::rend
// ✅ CRBegin             - DFView::crbegin
// ✅ CREnd               - DFView::crend
// ✅ IndexedIterator     - DFViewIndexed::IteratorIdx
// ✅ IndexedLookup       - DFViewIndexed::Contains / At
// ✅ SelectField         - operator|(SelectField<F>) [const]     -> DFView<T[ const], RecI>
// ✅ SelectRecord        - operator|(SelectRecord<R>) [const]    -> DFView<T[ const], FldI>
// ✅ SelectFieldIndexed  - operator|(SelectFieldIndexed<F>) [const]  -> DFViewIndexed<...>
// ✅ SelectRecordIndexed - operator|(SelectRecordIndexed<R>) [const] -> DFViewIndexed<...>
// ✅ Fields              - Fields() const -> Flds
// ✅ Records             - Records() const -> Recs
//

#include "gtest/gtest.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

#include "RM_HelperDFTestConfigs.h"

using namespace lugizmo;
using namespace lugizmo::test;

namespace {

    // Builds a populated frame whose cell (field f, record r) holds `r * FLD_COUNT + f`.
    template<typename Cfg>
    auto BuildFilled() -> Cfg::DF
    {
        auto df = Cfg::Build();
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
            {
                df.AssignValue(Cfg::FieldKey(f), Cfg::RecordKey(r), static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
        return df;
    }

} // namespace

template<typename>
class RM_DataframeViews: public testing::Test // NOLINT(readability-identifier-naming)
{
};

TYPED_TEST_SUITE(RM_DataframeViews, IndexConfigs);

/**
 *  @brief ViewField spans a column over the records (const + mutable).
 *  @see   lugizmo::DataFrame.ViewField(field)
 */
TYPED_TEST(RM_DataframeViews, ViewField)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // a field view spans its column in record order
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            auto const view = std::as_const(df).ViewField(Cfg::FieldKey(f));
            ASSERT_EQ(view.Size(), Cfg::REC_COUNT);
            EXPECT_FALSE(view.Empty());

            std::size_t r = 0;
            for (auto const value : view) EXPECT_EQ(value, static_cast<int>(r++ * Cfg::FLD_COUNT + f));
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // At resolves a record key; a const view gives const access
        auto const view  = std::as_const(df).ViewField(Cfg::FieldKey(1));
        auto const* cell = view.At(Cfg::RecordKey(2));
        ASSERT_NE(cell, nullptr);
        EXPECT_EQ(*cell, static_cast<int>(2 * Cfg::FLD_COUNT + 1));
        static_assert(std::is_const_v<std::remove_pointer_t<decltype(view.At(Cfg::RecordKey(0)))>>);
    }

    {
        // a mutable view writes through to the frame
        auto view = df.ViewField(Cfg::FieldKey(0));
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(view.At(Cfg::RecordKey(0)))>>);

        auto* cell = view.At(Cfg::RecordKey(0));
        ASSERT_NE(cell, nullptr);
        *cell = 999;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 999);
    }
}

/**
 *  @brief ViewRecord spans a row over the fields (const + mutable).
 *  @see   lugizmo::DataFrame.ViewRecord(record)
 */
TYPED_TEST(RM_DataframeViews, ViewRecord)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // a record view spans its row in field order
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            auto const view = std::as_const(df).ViewRecord(Cfg::RecordKey(r));
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);
            EXPECT_FALSE(view.Empty());

            std::size_t f = 0;
            for (auto const value : view) EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f++));
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // At resolves a field key; a const view gives const access
        auto const view  = std::as_const(df).ViewRecord(Cfg::RecordKey(1));
        auto const* cell = view.At(Cfg::FieldKey(2));
        ASSERT_NE(cell, nullptr);
        EXPECT_EQ(*cell, static_cast<int>(1 * Cfg::FLD_COUNT + 2));
        static_assert(std::is_const_v<std::remove_pointer_t<decltype(view.At(Cfg::FieldKey(0)))>>);
    }

    {
        // a mutable view writes through to the frame
        auto view = df.ViewRecord(Cfg::RecordKey(0));
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(view.At(Cfg::FieldKey(0)))>>);

        auto* cell = view.At(Cfg::FieldKey(0));
        ASSERT_NE(cell, nullptr);
        *cell = 999;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 999);
    }
}

/**
 * @brief Contains performs a read-only lookup through the opposite-axis index.
 * @see   lugizmo::DFView.Contains
 */
TYPED_TEST(RM_DataframeViews, Contains)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // A const field view resolves record keys without changing the view.
        auto const view = std::as_const(df).ViewField(Cfg::FieldKey(0));
        auto const firstRecord = Cfg::RecordKey(0);
        static_assert(noexcept(view.Contains(firstRecord)));

        EXPECT_TRUE(view.Contains(firstRecord));
        EXPECT_TRUE(view.Contains(Cfg::RecordKey(Cfg::REC_COUNT - 1)));
        EXPECT_FALSE(view.Contains(Cfg::MissingRecord()));
    }

    {
        // A const view object created from a mutable frame supports the same lookup.
        auto const view = df.ViewRecord(Cfg::RecordKey(0));

        EXPECT_TRUE(view.Contains(Cfg::FieldKey(0)));
        EXPECT_TRUE(view.Contains(Cfg::FieldKey(Cfg::FLD_COUNT - 1)));
        EXPECT_FALSE(view.Contains(Cfg::MissingField()));
    }

    {
        // The empty view returned for a missing selection contains no keys.
        auto const empty = std::as_const(df).ViewField(Cfg::MissingField());
        EXPECT_TRUE(empty.Empty());
        EXPECT_FALSE(empty.Contains(Cfg::RecordKey(0)));
        EXPECT_FALSE(empty.Contains(Cfg::MissingRecord()));
    }
}

/**
 * @brief Positional indexing follows span-like element constness.
 * @see   lugizmo::DFView.operator[]
 */
TYPED_TEST(RM_DataframeViews, IndexOperator)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        auto const view = df.ViewField(Cfg::FieldKey(0));
        static_assert(!std::is_const_v<std::remove_reference_t<decltype(view[0])>>);

        view[0] = 101;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 101);
    }

    {
        auto const view = std::as_const(df).ViewField(Cfg::FieldKey(0));
        static_assert(std::is_const_v<std::remove_reference_t<decltype(view[0])>>);
        EXPECT_EQ(view[0], 101);
    }
}

/**
 * @brief Checked positional access preserves element constness and reports misses.
 * @see   lugizmo::DFView.operator()
 */
TYPED_TEST(RM_DataframeViews, CheckedIndexOperator)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        auto const view = df.ViewField(Cfg::FieldKey(0));
        auto value      = view(0);
        static_assert(!std::is_const_v<std::remove_reference_t<decltype(value.Value())>>);

        ASSERT_TRUE(value.HasValue());
        value.Value() = 102;
        EXPECT_FALSE(view(Cfg::REC_COUNT).HasValue());
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 102);
    }

    {
        auto const view = std::as_const(df).ViewField(Cfg::FieldKey(0));
        auto value      = view(0);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(value.Value())>>);
        ASSERT_TRUE(value.HasValue());
        EXPECT_EQ(value.Value(), 102);
    }
}

/**
 * @brief Key lookup returns pointers whose constness follows the element type.
 * @see   lugizmo::DFView.At
 */
TYPED_TEST(RM_DataframeViews, At)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        auto const view = df.ViewRecord(Cfg::RecordKey(0));
        auto* value     = view.At(Cfg::FieldKey(0));
        static_assert(!std::is_const_v<std::remove_pointer_t<decltype(value)>>);

        ASSERT_NE(value, nullptr);
        *value = 103;
        EXPECT_EQ(view.At(Cfg::MissingField()), nullptr);
    }

    {
        auto const view = std::as_const(df).ViewRecord(Cfg::RecordKey(0));
        auto* value     = view.At(Cfg::FieldKey(0));
        static_assert(std::is_const_v<std::remove_pointer_t<decltype(value)>>);

        ASSERT_NE(value, nullptr);
        EXPECT_EQ(*value, 103);
        EXPECT_EQ(view.At(Cfg::MissingField()), nullptr);
    }
}

/**
 * @brief TryAt returns an unqualified value copy for mutable and const-element views.
 * @see   lugizmo::DFView.TryAt
 */
TYPED_TEST(RM_DataframeViews, TryAt)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const mutableView = df.ViewField(Cfg::FieldKey(0));
    auto mutableCopy       = mutableView.TryAt(Cfg::RecordKey(1));
    static_assert(std::same_as<decltype(mutableCopy), std::optional<int>>);
    ASSERT_TRUE(mutableCopy.has_value());
    EXPECT_EQ(*mutableCopy, static_cast<int>(Cfg::FLD_COUNT));

    auto const constView = std::as_const(df).ViewField(Cfg::FieldKey(0));
    auto constCopy       = constView.TryAt(Cfg::RecordKey(1));
    static_assert(std::same_as<decltype(constCopy), std::optional<int>>);
    ASSERT_TRUE(constCopy.has_value());
    EXPECT_EQ(*constCopy, static_cast<int>(Cfg::FLD_COUNT));
    EXPECT_FALSE(constView.TryAt(Cfg::MissingRecord()).has_value());
}

/**
 * @brief Iteration uses the same span-like element constness as direct access.
 * @see   lugizmo::DFView.begin
 */
TYPED_TEST(RM_DataframeViews, IteratorConstness)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const mutableView = df.ViewField(Cfg::FieldKey(0));
    static_assert(!std::is_const_v<std::remove_reference_t<decltype(*mutableView.begin())>>);
    *mutableView.begin() = 104;

    auto const constView = std::as_const(df).ViewField(Cfg::FieldKey(0));
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*constView.begin())>>);
    EXPECT_EQ(*constView.begin(), 104);
}

/**
 * @brief Front returns the first value with element-based constness.
 * @see   lugizmo::DFView.Front
 */
TYPED_TEST(RM_DataframeViews, Front)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const mutableView = df.ViewField(Cfg::FieldKey(0));
    auto* mutableFront     = mutableView.Front();
    static_assert(!std::is_const_v<std::remove_pointer_t<decltype(mutableFront)>>);
    ASSERT_NE(mutableFront, nullptr);
    *mutableFront = 105;

    auto const constView = std::as_const(df).ViewField(Cfg::FieldKey(0));
    auto* constFront     = constView.Front();
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(constFront)>>);
    ASSERT_NE(constFront, nullptr);
    EXPECT_EQ(*constFront, 105);

    auto const empty = df.ViewField(Cfg::MissingField());
    EXPECT_EQ(empty.Front(), nullptr);
}

/**
 * @brief Back returns the final value with element-based constness.
 * @see   lugizmo::DFView.Back
 */
TYPED_TEST(RM_DataframeViews, Back)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const mutableView = df.ViewField(Cfg::FieldKey(0));
    auto* mutableBack      = mutableView.Back();
    static_assert(!std::is_const_v<std::remove_pointer_t<decltype(mutableBack)>>);
    ASSERT_NE(mutableBack, nullptr);
    *mutableBack = 106;

    auto const constView = std::as_const(df).ViewField(Cfg::FieldKey(0));
    auto* constBack      = constView.Back();
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(constBack)>>);
    ASSERT_NE(constBack, nullptr);
    EXPECT_EQ(*constBack, 106);

    auto const empty = df.ViewField(Cfg::MissingField());
    EXPECT_EQ(empty.Back(), nullptr);
}

/**
 * @brief rbegin starts at the final value and preserves element constness.
 * @see   lugizmo::DFView.rbegin
 */
TYPED_TEST(RM_DataframeViews, RBegin)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const mutableView = df.ViewField(Cfg::FieldKey(0));
    auto reverse           = mutableView.rbegin();
    static_assert(!std::is_const_v<std::remove_reference_t<decltype(*reverse)>>);
    ASSERT_NE(reverse, mutableView.rend());
    EXPECT_EQ(*reverse, static_cast<int>((Cfg::REC_COUNT - 1) * Cfg::FLD_COUNT));
    *reverse = 201;

    auto const constView = std::as_const(df).ViewField(Cfg::FieldKey(0));
    auto constReverse    = constView.rbegin();
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*constReverse)>>);
    EXPECT_EQ(*constReverse, 201);
}

/**
 * @brief rend is one past the reversed sequence and its predecessor is the first value.
 * @see   lugizmo::DFView.rend
 */
TYPED_TEST(RM_DataframeViews, REnd)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();
    auto view = df.ViewField(Cfg::FieldKey(0));

    EXPECT_EQ(view.rend() - view.rbegin(), static_cast<std::ptrdiff_t>(Cfg::REC_COUNT));
    EXPECT_EQ(*(view.rend() - 1), 0);

    auto const empty = df.ViewField(Cfg::MissingField());
    EXPECT_EQ(empty.rbegin(), empty.rend());
}

/**
 * @brief crbegin starts at the final value of a const-element view.
 * @see   lugizmo::DFView.crbegin
 */
TYPED_TEST(RM_DataframeViews, CRBegin)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();
    auto const view = std::as_const(df).ViewField(Cfg::FieldKey(0));

    auto reverse = view.crbegin();
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*reverse)>>);
    ASSERT_NE(reverse, view.crend());
    EXPECT_EQ(*reverse, static_cast<int>((Cfg::REC_COUNT - 1) * Cfg::FLD_COUNT));
}

/**
 * @brief crend terminates const reverse traversal after every value.
 * @see   lugizmo::DFView.crend
 */
TYPED_TEST(RM_DataframeViews, CREnd)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();
    auto const view = std::as_const(df).ViewField(Cfg::FieldKey(0));

    EXPECT_EQ(view.crend() - view.crbegin(), static_cast<std::ptrdiff_t>(Cfg::REC_COUNT));
    EXPECT_EQ(*(view.crend() - 1), 0);
}

/**
 *  @brief ViewFieldIndexed yields (value, record-key) per entry (const + mutable).
 *  @see   lugizmo::DataFrame.ViewFieldIndexed(field)
 */
TYPED_TEST(RM_DataframeViews, ViewFieldIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // each entry pairs the column value with its record key
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            auto const view = std::as_const(df).ViewFieldIndexed(Cfg::FieldKey(f));
            ASSERT_EQ(view.Size(), Cfg::REC_COUNT);

            std::size_t r = 0;
            for (auto const& entry : view)
            {
                EXPECT_EQ(*entry.val, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(*entry.idx, Cfg::RecordKey(r));
                ++r;
            }
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // a mutable indexed view writes through
        auto view = df.ViewFieldIndexed(Cfg::FieldKey(0));
        for (auto& entry : view) *entry.val = 7;

        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(r)), 7);
        }
    }
}

/**
 *  @brief ViewRecordIndexed yields (value, field-key) per entry (const + mutable).
 *  @see   lugizmo::DataFrame.ViewRecordIndexed(record)
 */
TYPED_TEST(RM_DataframeViews, ViewRecordIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // each entry pairs the row value with its field key
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            auto const view = std::as_const(df).ViewRecordIndexed(Cfg::RecordKey(r));
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);

            std::size_t f = 0;
            for (auto const& entry : view)
            {
                EXPECT_EQ(*entry.val, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(*entry.idx, Cfg::FieldKey(f));
                ++f;
            }
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // a mutable indexed view writes through
        auto view = df.ViewRecordIndexed(Cfg::RecordKey(0));
        for (auto& entry : view) *entry.val = 7;

        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(f), Cfg::RecordKey(0)), 7);
        }
    }
}

/**
 * @brief Indexed views resolve keys through the dataframe index for mutable and const access.
 * @see   lugizmo::DFViewIndexed.Contains
 * @see   lugizmo::DFViewIndexed.At
 */
TYPED_TEST(RM_DataframeViews, IndexedLookup)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        auto view = df.ViewFieldIndexed(Cfg::FieldKey(1));

        EXPECT_TRUE(view.Contains(Cfg::RecordKey(2)));
        EXPECT_FALSE(view.Contains(Cfg::MissingRecord()));

        auto* value = view.At(Cfg::RecordKey(2));
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(*value, static_cast<int>(2 * Cfg::FLD_COUNT + 1));
        EXPECT_EQ(view.At(Cfg::MissingRecord()), nullptr);

        *value = 77;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(1), Cfg::RecordKey(2)), 77);
    }

    {
        auto const view = std::as_const(df).ViewRecordIndexed(Cfg::RecordKey(0));
        auto const* value = view.At(Cfg::FieldKey(1));
        static_assert(std::is_const_v<std::remove_pointer_t<decltype(value)>>);

        ASSERT_NE(value, nullptr);
        EXPECT_EQ(*value, 1);
        EXPECT_EQ(view.At(Cfg::MissingField()), nullptr);
    }
}

/**
 *  @brief Indexed view iterators keep value and key positions synchronized for every
 *         random-access operation.
 */
TYPED_TEST(RM_DataframeViews, IndexedIterator)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();
    auto view = df.ViewFieldIndexed(Cfg::FieldKey(0));

    using Iterator = decltype(view.begin());
    static_assert(std::random_access_iterator<Iterator>);
    static_assert(std::ranges::random_access_range<decltype(view)>);

    auto const begin = view.begin();
    auto const end   = view.end();

    EXPECT_EQ(end - begin, static_cast<std::ptrdiff_t>(Cfg::REC_COUNT));
    EXPECT_EQ(std::ranges::distance(view), static_cast<std::ptrdiff_t>(Cfg::REC_COUNT));

    auto second = begin + 1;
    EXPECT_EQ(second - begin, 1);
    EXPECT_EQ(*second->val, static_cast<int>(Cfg::FLD_COUNT));
    EXPECT_EQ(*second->idx, Cfg::RecordKey(1));

    auto first = second - 1;
    EXPECT_EQ(*first->val, 0);
    EXPECT_EQ(*first->idx, Cfg::RecordKey(0));

    auto symmetric = 2 + begin;
    EXPECT_EQ(*symmetric->val, static_cast<int>(2 * Cfg::FLD_COUNT));
    EXPECT_EQ(*symmetric->idx, Cfg::RecordKey(2));

    auto advanced = begin;
    advanced += 2;
    EXPECT_EQ(*advanced->val, static_cast<int>(2 * Cfg::FLD_COUNT));
    EXPECT_EQ(*advanced->idx, Cfg::RecordKey(2));
    advanced -= 1;
    EXPECT_EQ(*advanced->val, static_cast<int>(Cfg::FLD_COUNT));
    EXPECT_EQ(*advanced->idx, Cfg::RecordKey(1));

    auto const& indexed = begin[2];
    EXPECT_EQ(*indexed.val, static_cast<int>(2 * Cfg::FLD_COUNT));
    EXPECT_EQ(*indexed.idx, Cfg::RecordKey(2));

    // Direct-access results retain their own index position, including generated range keys.
    auto const directFirst  = view[0];
    auto const directSecond = view[1];
    EXPECT_EQ(*directFirst.idx, Cfg::RecordKey(0));
    EXPECT_EQ(*directSecond.idx, Cfg::RecordKey(1));

    auto const checked = view(2);
    ASSERT_TRUE(checked.has_value());
    EXPECT_EQ(*checked->val, static_cast<int>(2 * Cfg::FLD_COUNT));
    EXPECT_EQ(*checked->idx, Cfg::RecordKey(2));
    EXPECT_FALSE(view(Cfg::REC_COUNT).has_value());

    auto incremented = begin;
    auto previous    = incremented++;
    EXPECT_EQ(*previous->idx, Cfg::RecordKey(0));
    EXPECT_EQ(*incremented->idx, Cfg::RecordKey(1));
    previous = incremented--;
    EXPECT_EQ(*previous->idx, Cfg::RecordKey(1));
    EXPECT_EQ(*incremented->idx, Cfg::RecordKey(0));

    EXPECT_LT(begin, end);
    EXPECT_LE(begin, end);
    EXPECT_GT(end, begin);
    EXPECT_GE(end, begin);

    Iterator copied = begin;
    Iterator moved  = std::move(copied);
    EXPECT_EQ(*moved->idx, Cfg::RecordKey(0));

    Iterator copyAssigned;
    copyAssigned = begin;
    Iterator moveAssigned;
    moveAssigned = std::move(copyAssigned);
    EXPECT_EQ(*moveAssigned->idx, Cfg::RecordKey(0));

    auto constView  = std::as_const(df).ViewFieldIndexed(Cfg::FieldKey(0));
    auto constBegin = constView.begin();
    static_assert(std::is_const_v<std::remove_pointer_t<decltype(constBegin->First())>>);
    EXPECT_EQ(*constBegin[1].val, static_cast<int>(Cfg::FLD_COUNT));
    EXPECT_EQ(*constBegin[1].idx, Cfg::RecordKey(1));
}

/**
 *  @brief df | SelectField(field) yields the same view as ViewField.
 *  @see   lugizmo::DataFrame.operator|(SelectField<F>)
 */
TYPED_TEST(RM_DataframeViews, SelectField)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const selection traverses the requested field
        auto const& constDf = std::as_const(df);
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            auto const view = constDf | SelectField(Cfg::FieldKey(f));
            ASSERT_EQ(view.Size(), Cfg::REC_COUNT);

            std::size_t r = 0;
            for (auto const value : view) EXPECT_EQ(value, static_cast<int>(r++ * Cfg::FLD_COUNT + f));
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // mutable selection writes through to the dataframe
        auto view = df | SelectField(Cfg::FieldKey(0));
        view[0] = 101;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 101);
    }
}

/**
 *  @brief df | SelectRecord(record) yields the same view as ViewRecord.
 *  @see   lugizmo::DataFrame.operator|(SelectRecord<R>)
 */
TYPED_TEST(RM_DataframeViews, SelectRecord)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const selection traverses the requested record
        auto const& constDf = std::as_const(df);
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            auto const view = constDf | SelectRecord(Cfg::RecordKey(r));
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);

            std::size_t f = 0;
            for (auto const value : view) EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f++));
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // mutable selection writes through to the dataframe
        auto view = df | SelectRecord(Cfg::RecordKey(0));
        view[0] = 102;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 102);
    }
}

/**
 *  @brief df | SelectFieldIndexed(field) yields the same view as ViewFieldIndexed.
 *  @see   lugizmo::DataFrame.operator|(SelectFieldIndexed<F>)
 */
TYPED_TEST(RM_DataframeViews, SelectFieldIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const selection pairs values with record keys
        auto const& constDf = std::as_const(df);
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            auto const view = constDf | SelectFieldIndexed(Cfg::FieldKey(f));
            ASSERT_EQ(view.Size(), Cfg::REC_COUNT);

            std::size_t r = 0;
            for (auto const& entry : view)
            {
                EXPECT_EQ(*entry.val, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(*entry.idx, Cfg::RecordKey(r));
                ++r;
            }
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // mutable indexed selection writes through to the dataframe
        auto view = df | SelectFieldIndexed(Cfg::FieldKey(0));
        *view[0].val = 103;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 103);
    }
}

/**
 *  @brief df | SelectRecordIndexed(record) yields the same view as ViewRecordIndexed.
 *  @see   lugizmo::DataFrame.operator|(SelectRecordIndexed<R>)
 */
TYPED_TEST(RM_DataframeViews, SelectRecordIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const selection pairs values with field keys
        auto const& constDf = std::as_const(df);
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            auto const view = constDf | SelectRecordIndexed(Cfg::RecordKey(r));
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);

            std::size_t f = 0;
            for (auto const& entry : view)
            {
                EXPECT_EQ(*entry.val, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(*entry.idx, Cfg::FieldKey(f));
                ++f;
            }
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // mutable indexed selection writes through to the dataframe
        auto view = df | SelectRecordIndexed(Cfg::RecordKey(0));
        *view[0].val = 104;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 104);
    }
}

/**
 *  @brief Fields() exposes the field keys in order.
 *  @see   lugizmo::DataFrame.Fields() const -> Flds
 */
TYPED_TEST(RM_DataframeViews, Fields)
{
    using Cfg = TypeParam;
    auto const df = Cfg::Build();

    auto const flds = df.Fields();
    EXPECT_EQ(flds.size(), Cfg::FLD_COUNT);

    std::size_t f = 0;
    for (auto const key : flds) EXPECT_EQ(key, Cfg::FieldKey(f++));
    EXPECT_EQ(f, Cfg::FLD_COUNT);
}

/**
 *  @brief Records() exposes the record keys in order.
 *  @see   lugizmo::DataFrame.Records() const -> Recs
 */
TYPED_TEST(RM_DataframeViews, Records)
{
    using Cfg = TypeParam;
    auto const df = Cfg::Build();

    auto const recs = df.Records();
    EXPECT_EQ(recs.size(), Cfg::REC_COUNT);

    std::size_t r = 0;
    for (auto const key : recs) EXPECT_EQ(key, Cfg::RecordKey(r++));
    EXPECT_EQ(r, Cfg::REC_COUNT);
}
