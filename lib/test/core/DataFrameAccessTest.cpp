// Filename: DataFrameAccessTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions accessing dataframe values, across every index configuration
// (unique, range step==1, range step!=1) via TYPED_TEST.
// The following functions are tested here (with names of tests):
//
// ✅ GetValue
// - GetValue(FldT const& field, RecT const& record) const -> T const*
// - GetValue(FldT const& field, RecT const& record) -> T*
//
// ✅ IndexOperator
// - operator[](FldT const& field, RecT const& record) -> T&
// - operator[](FldT const& field, RecT const& record) const -> T const&
//
// ✅ Data
// - Data() const -> T const*
//
// ✅ MDSpan
// - MDSpan() const -> RecsData<T const>
//
// ✅ Values
// - Values(this auto& self) -> std::span<Value<Self>>
//

#include "gtest/gtest.h"

#include <cstddef>
#include <type_traits>
#include <utility>

#include "HelperDFTestConfigs.h"

using namespace lgz::test;

template<typename>
class RM_DataframeAccess: public testing::Test // NOLINT(readability-identifier-naming)
{
};

TYPED_TEST_SUITE(RM_DataframeAccess, IndexConfigs);

namespace {

    // Fills cell (field f, record r) with `r * FLD_COUNT + f`, so the row-major buffer is iota.
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

/**
 *  @brief GetValue returns a pointer to the stored value, or null for missing keys.
 *  @see   lgz::DataFrame.GetValue(...)
 */
TYPED_TEST(RM_DataframeAccess, GetValue)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const lookup returns the stored value at each cell
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
            {
                auto const val = std::as_const(df).GetValue(Cfg::FieldKey(f), Cfg::RecordKey(r));
                ASSERT_NE(val, nullptr);
                EXPECT_EQ(*val, static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
    }

    {
        // a missing field or record key resolves to nothing
        EXPECT_EQ(std::as_const(df).GetValue(Cfg::MissingField(), Cfg::RecordKey(0)), nullptr);
        EXPECT_EQ(std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::MissingRecord()), nullptr);
    }

    {
        // the mutable overload yields a non-const reference and writes through
        auto val = df.GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0));
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(val)>>);
        ASSERT_NE(val, nullptr);

        *val = 123;
        EXPECT_EQ(*std::as_const(df).GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0)), 123);
    }
}

/**
 *  @brief operator[] returns the stored value (unchecked), mutable overload writes through.
 *  @see   lgz::DataFrame.operator[](...)
 */
TYPED_TEST(RM_DataframeAccess, IndexOperator)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const operator[] returns the stored value at each cell
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
            {
                EXPECT_EQ((std::as_const(df)[Cfg::FieldKey(f), Cfg::RecordKey(r)]), static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
    }

    {
        // mutable operator[] yields a non-const reference and writes through
        auto& ref = df[Cfg::FieldKey(0), Cfg::RecordKey(0)];
        static_assert(not std::is_const_v<std::remove_reference_t<decltype(ref)>>);

        ref = 123;
        EXPECT_EQ((std::as_const(df)[Cfg::FieldKey(0), Cfg::RecordKey(0)]), 123);
    }
}

/**
 *  @brief Data() exposes the dense row-major buffer in position order.
 *  @see   lgz::DataFrame.Data() const
 */
TYPED_TEST(RM_DataframeAccess, Data)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // the buffer is dense and in row-major position order (iota by construction)
        auto const* data = std::as_const(df).Data();
        ASSERT_NE(data, nullptr);

        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
            {
                EXPECT_EQ(data[r * Cfg::FLD_COUNT + f], static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
    }
}

/**
 *  @brief MDSpan() exposes the buffer as a (records x fields) mdspan.
 *  @see   lgz::DataFrame.MDSpan() const
 */
TYPED_TEST(RM_DataframeAccess, MDSpan)
{
    using Cfg = TypeParam;

    {
        // an empty frame yields an empty span
        typename Cfg::DF const empty;
        auto const md = empty.MDSpan();
        EXPECT_TRUE(md.empty());
        EXPECT_EQ(md.extent(0), 0);
        EXPECT_EQ(md.extent(1), 0);
    }

    auto df = BuildFilled<Cfg>();

    {
        // extents match record x field counts; handle aliases Data()
        auto const md = std::as_const(df).MDSpan();
        ASSERT_EQ(md.extent(0), Cfg::REC_COUNT);
        ASSERT_EQ(md.extent(1), Cfg::FLD_COUNT);
        ASSERT_EQ(md.size(), Cfg::REC_COUNT * Cfg::FLD_COUNT);
        EXPECT_EQ(md.data_handle(), std::as_const(df).Data());

        // every element is addressed by [record, field]
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
        {
            for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
            {
                EXPECT_EQ((md[r, f]), static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
    }
}

/**
 *  @brief Values() exposes the whole value buffer as a flat span (const + mutable).
 *  @see   lgz::DataFrame.Values(this auto& self)
 */
TYPED_TEST(RM_DataframeAccess, Values)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    {
        // const view spans every value in row-major position order
        auto const values = std::as_const(df).Values();
        ASSERT_EQ(values.size(), Cfg::FLD_COUNT * Cfg::REC_COUNT);
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            EXPECT_EQ(values[i], static_cast<int>(i));
        }
    }

    {
        // mutable view yields non-const elements and writes through to the buffer
        auto values = df.Values();
        static_assert(not std::is_const_v<std::remove_reference_t<decltype(values[0])>>);

        values[0] = 123;
        EXPECT_EQ(std::as_const(df).Data()[0], 123);
    }
}
