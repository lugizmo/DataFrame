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
// ✅ ViewRecord          - ViewRecord(record) [const]            -> DFSpan<T[ const], FldI>
// ✅ ViewFieldIndexed    - ViewFieldIndexed(field) [const]       -> DFViewIndexed<T[ const], RecI const>
// ✅ ViewRecordIndexed   - ViewRecordIndexed(record) [const]     -> DFViewIndexed<T[ const], FldI const>
// ✅ Slice               - Slice() / Slice(fields, records)
// ✅ SliceFields         - SliceFields(fields)
// ✅ SliceRecords        - SliceRecords(records)
// ✅ SliceRangeFields    - SliceFields(DFRangeIndexBounds)
// ✅ SliceRangeRecords   - SliceRecords(DFRangeIndexBounds)
// ✅ SliceRangeMixed     - Slice(bounds, keys) / Slice(keys, bounds)
// ✅ SelectField         - operator|(SelectField<F>) [const]     -> DFView<T[ const], RecI>
// ✅ SelectRecord        - operator|(SelectRecord<R>) [const]    -> DFSpan<T[ const], FldI>
// ✅ SelectFieldIndexed  - operator|(SelectFieldIndexed<F>) [const]  -> DFViewIndexed<...>
// ✅ SelectRecordIndexed - operator|(SelectRecordIndexed<R>) [const] -> DFViewIndexed<...>
// ✅ Fields              - Fields() const -> Flds
// ✅ Records             - Records() const -> Recs
//

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

#include "RM_HelperDFTestConfigs.h"

using namespace lgz;
using namespace lgz::test;

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
 *  @see   lgz::DataFrame.ViewField(field)
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
            static_assert(!std::ranges::contiguous_range<decltype(view)>);
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
 *  @see   lgz::DataFrame.ViewRecord(record)
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
            static_assert(std::ranges::contiguous_range<decltype(view)>);
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
 *  @brief ViewFieldIndexed yields (record key, value) per entry (const + mutable).
 *  @see   lgz::DataFrame.ViewFieldIndexed(field)
 */
TYPED_TEST(RM_DataframeViews, ViewFieldIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // each entry pairs the record key with its column value
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            auto const view = std::as_const(df).ViewFieldIndexed(Cfg::FieldKey(f));
            static_assert(!std::ranges::contiguous_range<decltype(view)>);
            static_assert(!std::ranges::contiguous_range<decltype(view.Values())>);
            ASSERT_EQ(view.Size(), Cfg::REC_COUNT);

            std::size_t r = 0;
            for (auto [key, value] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(key, Cfg::RecordKey(r));
                ++r;
            }
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // a mutable indexed view writes through
        auto view = df.ViewFieldIndexed(Cfg::FieldKey(0));
        for (auto [key, value] : view)
        {
            static_cast<void>(key);
            value = 7;
        }

        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(r)), 7);
        }
    }
}

/**
 *  @brief ViewRecordIndexed yields (field key, value) per entry (const + mutable).
 *  @see   lgz::DataFrame.ViewRecordIndexed(record)
 */
TYPED_TEST(RM_DataframeViews, ViewRecordIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // each entry pairs the field key with its row value
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            auto const view = std::as_const(df).ViewRecordIndexed(Cfg::RecordKey(r));
            static_assert(!std::ranges::contiguous_range<decltype(view)>);
            static_assert(std::ranges::contiguous_range<decltype(view.Values())>);
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);

            std::size_t f = 0;
            for (auto [key, value] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(key, Cfg::FieldKey(f));
                ++f;
            }
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // a mutable indexed view writes through
        auto view = df.ViewRecordIndexed(Cfg::RecordKey(0));
        for (auto [key, value] : view)
        {
            static_cast<void>(key);
            value = 7;
        }

        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(f), Cfg::RecordKey(0)), 7);
        }
    }
}

/**
 * @brief Slice() exposes the complete dataframe as one contiguous storage-order range.
 * @see   lgz::DataFrame.Slice()
 */
TYPED_TEST(RM_DataframeViews, SliceAll)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const slice = std::as_const(df).Slice();
    static_assert(decltype(slice)::Contiguous);
    static_assert(std::ranges::contiguous_range<decltype(slice)>);
    EXPECT_TRUE(slice.IsContiguous());
    EXPECT_EQ(slice.FieldSize(), Cfg::FLD_COUNT);
    EXPECT_EQ(slice.RecordSize(), Cfg::REC_COUNT);
    EXPECT_TRUE(std::ranges::equal(slice, std::array{0, 1, 2, 3, 4, 5, 6, 7, 8}));
}

/**
 * @brief SliceFields selects consecutive fields across every record without copying.
 * @see   lgz::DataFrame.SliceFields(firstField, fieldCount)
 */
TYPED_TEST(RM_DataframeViews, SliceFields)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const selectedFields = std::array{Cfg::FieldKey(2), Cfg::FieldKey(0)};
    auto slice = df.SliceFields(selectedFields);
    static_assert(!decltype(slice)::Contiguous);
    static_assert(!std::ranges::contiguous_range<decltype(slice)>);
    EXPECT_FALSE(slice.IsContiguous());
    EXPECT_TRUE(std::ranges::equal(slice, std::array{0, 2, 3, 5, 6, 8}));

    slice[0] = 91;
    EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 91);
    EXPECT_TRUE(df.SliceFields(std::array{Cfg::MissingField()}).Empty());

    auto const initializerSlice = std::as_const(df).SliceFields({Cfg::FieldKey(2), Cfg::FieldKey(0)});
    EXPECT_EQ(initializerSlice.Size(), 2 * Cfg::REC_COUNT);
}

/**
 * @brief SliceRecords selects records by key and recognizes adjacent physical storage at runtime.
 * @see   lgz::DataFrame.SliceRecords(records)
 */
TYPED_TEST(RM_DataframeViews, SliceRecords)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const selectedRecords = std::array{Cfg::RecordKey(2), Cfg::RecordKey(1)};
    auto const slice = std::as_const(df).SliceRecords(selectedRecords);
    static_assert(!decltype(slice)::Contiguous);
    static_assert(!std::ranges::contiguous_range<decltype(slice)>);
    EXPECT_TRUE(slice.IsContiguous());
    EXPECT_TRUE(std::ranges::equal(slice, std::array{3, 4, 5, 6, 7, 8}));
    EXPECT_TRUE(std::as_const(df).SliceRecords(std::array{Cfg::MissingRecord()}).Empty());
}

/**
 * @brief Slice(fields, records) selects a keyed Cartesian product in physical storage order.
 * @see   lgz::DataFrame.Slice(fields, records)
 */
TYPED_TEST(RM_DataframeViews, SliceRectangle)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const selectedFields  = std::array{Cfg::FieldKey(2), Cfg::FieldKey(0)};
    auto const selectedRecords = std::array{Cfg::RecordKey(2), Cfg::RecordKey(0)};
    auto slice = df.Slice(selectedFields, selectedRecords);
    static_assert(!decltype(slice)::Contiguous);
    EXPECT_FALSE(slice.IsContiguous());
    EXPECT_TRUE(std::ranges::equal(slice, std::array{0, 2, 6, 8}));
    EXPECT_TRUE(slice.Contains(Cfg::FieldKey(2), Cfg::RecordKey(2)));
    EXPECT_FALSE(slice.Contains(Cfg::FieldKey(1), Cfg::RecordKey(2)));
    ASSERT_NE(slice.At(Cfg::FieldKey(2), Cfg::RecordKey(2)), nullptr);
    EXPECT_EQ(*slice.At(Cfg::FieldKey(2), Cfg::RecordKey(2)), 8);

    EXPECT_TRUE(df.Slice(std::array{Cfg::MissingField()}, std::array{Cfg::RecordKey(0)}).Empty());
    EXPECT_TRUE(df.Slice(std::array{Cfg::FieldKey(0)}, std::array{Cfg::MissingRecord()}).Empty());
}

/**
 * @brief Range-index field bounds use a calculated position stride without materializing positions.
 * @see   lgz::DataFrame.SliceFields(DFRangeIndexBounds)
 */
TEST(RM_DataframeViewsRange, SliceFields)
{
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

    auto df = DF();
    df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
    df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 4});

    for(int record = 0; record < 4; ++record)
        for(int field = 0; field < 4; ++field) df.AssignValue(field, record, record * 4 + field);

    auto unit = df.SliceFields(DFRangeIndexBounds{.lower = 1, .upper = 4});
    EXPECT_EQ(unit.FieldStride(), 1);
    EXPECT_FALSE(unit.IsContiguous());
    EXPECT_TRUE(std::ranges::equal(unit, std::array{1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15}));

    auto strided = df.SliceFields(DFRangeIndexBounds{.lower = 0, .upper = 4, .step = 2});
    EXPECT_EQ(strided.FieldStride(), 2);
    EXPECT_TRUE(std::ranges::equal(strided, std::array{0, 2, 4, 6, 8, 10, 12, 14}));
    EXPECT_FALSE(strided.Contains(1, 0));

    auto steppedIndex = DF();
    steppedIndex.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 8, .step = 2});
    steppedIndex.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 2});
    for(int record = 0; record < 2; ++record)
        for(int field = 0; field < 4; ++field) steppedIndex.AssignValue(field * 2, record, record * 4 + field);

    auto consecutivePositions = steppedIndex.SliceFields(DFRangeIndexBounds{.lower = 2, .upper = 8, .step = 2});
    EXPECT_EQ(consecutivePositions.FieldStride(), 1);
    EXPECT_TRUE(std::ranges::equal(consecutivePositions, std::array{1, 2, 3, 5, 6, 7}));
}

/**
 * @brief Range-index record bounds recognize complete adjacent rows and preserve larger strides.
 * @see   lgz::DataFrame.SliceRecords(DFRangeIndexBounds)
 */
TEST(RM_DataframeViewsRange, SliceRecords)
{
    using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;
    auto df = DF();
    df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
    df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
    for(int record = 0; record < 4; ++record)
        for(int field = 0; field < 4; ++field) df.AssignValue(field, record, record * 4 + field);

    auto unit = df.SliceRecords(DFRangeIndexBounds{.lower = 1, .upper = 3});
    EXPECT_EQ(unit.RecordStride(), 4);
    EXPECT_TRUE(unit.IsContiguous());
    EXPECT_TRUE(std::ranges::equal(unit, std::array{4, 5, 6, 7, 8, 9, 10, 11}));

    auto strided = df.SliceRecords(DFRangeIndexBounds{.lower = 0, .upper = 4, .step = 2});
    EXPECT_EQ(strided.RecordStride(), 8);
    EXPECT_FALSE(strided.IsContiguous());
    EXPECT_TRUE(std::ranges::equal(strided, std::array{0, 1, 2, 3, 8, 9, 10, 11}));
}

/**
 * @brief Range bounds compose with key selections for both mixed-index orientations.
 * @see   lgz::DataFrame.Slice(fields, records)
 */
TEST(RM_DataframeViewsRange, SliceMixed)
{
    {
        using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;
        auto df = DF();
        df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
        df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
        for(int record = 0; record < 4; ++record)
            for(int field = 0; field < 4; ++field) df.AssignValue(field, record, record * 4 + field);

        auto slice = df.Slice(DFRangeIndexBounds{.lower = 0, .upper = 4, .step = 2},
                              DFRangeIndexBounds{.lower = 0, .upper = 4, .step = 2});
        EXPECT_EQ(slice.FieldStride(), 2);
        EXPECT_EQ(slice.RecordStride(), 8);
        EXPECT_TRUE(std::ranges::equal(slice, std::array{0, 2, 8, 10}));
    }

    {
        using DF = DataFrame<int, int, DFRangeIndex<int>>;
        auto df = DF();
        df.AddFields(std::array{10, 20, 30});
        df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
        for(int record = 0; record < 4; ++record)
            for(int field = 0; field < 3; ++field) df.AssignValue((field + 1) * 10, record, record * 3 + field);

        auto slice = df.Slice(std::array{30, 10}, DFRangeIndexBounds{.lower = 1, .upper = 4, .step = 2});
        EXPECT_EQ(slice.RecordStride(), 6);
        EXPECT_TRUE(std::ranges::equal(slice, std::array{3, 5, 9, 11}));
    }

    {
        using DF = DataFrame<int, DFRangeIndex<int>, int>;
        auto df = DF();
        df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 4});
        df.AddRecords(std::array{10, 20, 30, 40});
        for(int record = 0; record < 4; ++record)
            for(int field = 0; field < 4; ++field) df.AssignValue(field, (record + 1) * 10, record * 4 + field);

        auto slice = df.Slice(DFRangeIndexBounds{.lower = 0, .upper = 4, .step = 2}, std::array{40, 20});
        EXPECT_EQ(slice.FieldStride(), 2);
        EXPECT_TRUE(std::ranges::equal(slice, std::array{4, 6, 12, 14}));
    }
}

/**
 *  @brief df | SelectField(field) yields the same view as ViewField.
 *  @see   lgz::DataFrame.operator|(SelectField<F>)
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
            auto const fieldKey = Cfg::FieldKey(f);
            auto const selector = SelectField{fieldKey};
            auto const view = constDf | selector;
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
 *  @see   lgz::DataFrame.operator|(SelectRecord<R>)
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
            static_assert(std::ranges::contiguous_range<decltype(view)>);
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
 *  @see   lgz::DataFrame.operator|(SelectFieldIndexed<F>)
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
            for (auto [key, value] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(key, Cfg::RecordKey(r));
                ++r;
            }
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // mutable indexed selection writes through to the dataframe
        auto view = df | SelectFieldIndexed(Cfg::FieldKey(0));
        view[0].val = 103;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 103);
    }
}

/**
 *  @brief df | SelectRecordIndexed(record) yields the same view as ViewRecordIndexed.
 *  @see   lgz::DataFrame.operator|(SelectRecordIndexed<R>)
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
            static_assert(std::ranges::contiguous_range<decltype(view.Values())>);
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);

            std::size_t f = 0;
            for (auto [key, value] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(key, Cfg::FieldKey(f));
                ++f;
            }
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // mutable indexed selection writes through to the dataframe
        auto view = df | SelectRecordIndexed(Cfg::RecordKey(0));
        view[0].val = 104;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 104);
    }
}

/**
 *  @brief Fields() exposes the field keys in order.
 *  @see   lgz::DataFrame.Fields() const -> Flds
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
 *  @see   lgz::DataFrame.Records() const -> Recs
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
