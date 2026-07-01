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
// ✅ SelectField         - operator|(SelectField<F>) [const]     -> DFView<T[ const], RecI>
// ✅ SelectRecord        - operator|(SelectRecord<R>) [const]    -> DFView<T[ const], FldI>
// ✅ SelectFieldIndexed  - operator|(SelectFieldIndexed<F>) [const]  -> DFViewIndexed<...>
// ✅ SelectRecordIndexed - operator|(SelectRecordIndexed<R>) [const] -> DFViewIndexed<...>
// ✅ Fields              - Fields() const -> Flds
// ✅ Records             - Records() const -> Recs
//

#include "gtest/gtest.h"

#include <cstddef>
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
 *  @brief ViewFieldIndexed yields (value, record index) per entry (const + mutable).
 *  @see   lugizmo::DataFrame.ViewFieldIndexed(field)
 */
TYPED_TEST(RM_DataframeViews, ViewFieldIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // each entry pairs the column value with its record index
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            auto const view = std::as_const(df).ViewFieldIndexed(Cfg::FieldKey(f));
            ASSERT_EQ(view.Size(), Cfg::REC_COUNT);

            std::size_t r = 0;
            for (auto [value, index] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(index, Cfg::RecordKey(r));
                ++r;
            }
            EXPECT_EQ(r, Cfg::REC_COUNT);
        }
    }

    {
        // a mutable indexed view writes through
        auto view = df.ViewFieldIndexed(Cfg::FieldKey(0));
        for (auto [value, index] : view)
        {
            static_cast<void>(index);
            value = 7;
        }

        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(r)), 7);
        }
    }
}

/**
 *  @brief ViewRecordIndexed yields (value, field index) per entry (const + mutable).
 *  @see   lugizmo::DataFrame.ViewRecordIndexed(record)
 */
TYPED_TEST(RM_DataframeViews, ViewRecordIndexed)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // each entry pairs the row value with its field index
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            auto const view = std::as_const(df).ViewRecordIndexed(Cfg::RecordKey(r));
            ASSERT_EQ(view.Size(), Cfg::FLD_COUNT);

            std::size_t f = 0;
            for (auto [value, index] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(index, Cfg::FieldKey(f));
                ++f;
            }
            EXPECT_EQ(f, Cfg::FLD_COUNT);
        }
    }

    {
        // a mutable indexed view writes through
        auto view = df.ViewRecordIndexed(Cfg::RecordKey(0));
        for (auto [value, index] : view)
        {
            static_cast<void>(index);
            value = 7;
        }

        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(f), Cfg::RecordKey(0)), 7);
        }
    }
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
            for (auto [value, index] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(index, Cfg::RecordKey(r));
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
            for (auto [value, index] : view)
            {
                EXPECT_EQ(value, static_cast<int>(r * Cfg::FLD_COUNT + f));
                EXPECT_EQ(index, Cfg::FieldKey(f));
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
