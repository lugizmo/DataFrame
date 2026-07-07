// Filename: DataFramePropertiesTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions reporting dataframe properties, across every index configuration via TYPED_TEST.
// The following functions are tested here (with names of tests):
//
// ✅ HasField
// - HasField<C>(C const& field) const -> bool
//
// ✅ HasRecord
// - HasRecord<C>(C const& record) const -> bool
//
// ✅ Size
// - Size() const -> size_t
//
// ✅ FieldSize
// - FieldSize() const -> size_t
//
// ✅ RecordSize
// - RecordSize() const -> size_t
//
// ✅ Empty
// - Empty() const -> bool
//

#include "gtest/gtest.h"

#include <cstddef>

#include "HelperDFTestConfigs.h"

using namespace lgz::test;

template<typename>
class RM_DataframeProperties: public testing::Test // NOLINT(readability-identifier-naming)
{
};

TYPED_TEST_SUITE(RM_DataframeProperties, IndexConfigs);

/**
 *  @brief HasField reports membership of a field key.
 *  @see   lgz::DataFrame.HasField<C>(C const& field) const -> bool
 */
TYPED_TEST(RM_DataframeProperties, HasField)
{
    using Cfg = TypeParam;
    auto const df = Cfg::Build();

    {
        // every present field key is found
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f) EXPECT_TRUE(df.HasField(Cfg::FieldKey(f)));
    }

    {
        // a non-member field key is not found
        EXPECT_FALSE(df.HasField(Cfg::MissingField()));
    }

    {
        // a comparable key of a different type resolves the same membership
        // (heterogeneous comparison for range, convertible/transparent lookup for unique)
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f) EXPECT_TRUE(df.HasField(static_cast<short>(Cfg::FieldKey(f))));
        EXPECT_FALSE(df.HasField(static_cast<short>(Cfg::MissingField())));
    }
}

/**
 *  @brief HasRecord reports membership of a record key.
 *  @see   lgz::DataFrame.HasRecord<C>(C const& record) const -> bool
 */
TYPED_TEST(RM_DataframeProperties, HasRecord)
{
    using Cfg = TypeParam;
    auto const df = Cfg::Build();

    {
        // every present record key is found
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r) EXPECT_TRUE(df.HasRecord(Cfg::RecordKey(r)));
    }

    {
        // a non-member record key (off-grid for step != 1) is not found
        EXPECT_FALSE(df.HasRecord(Cfg::MissingRecord()));
    }

    {
        // a comparable key of a different type resolves the same membership
        // (heterogeneous comparison for range, convertible/transparent lookup for unique)
        for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r) EXPECT_TRUE(df.HasRecord(static_cast<short>(Cfg::RecordKey(r))));
        EXPECT_FALSE(df.HasRecord(static_cast<short>(Cfg::MissingRecord())));
    }
}

/**
 *  @brief Size reports the total number of values (fields * records).
 *  @see   lgz::DataFrame.Size() const -> size_t
 */
TYPED_TEST(RM_DataframeProperties, Size)
{
    using Cfg = TypeParam;

    {
        // a populated frame reports fields * records
        auto const df = Cfg::Build();
        EXPECT_EQ(df.Size(), Cfg::FLD_COUNT * Cfg::REC_COUNT);
    }

    {
        // an empty frame reports 0
        typename Cfg::DF const empty;
        EXPECT_EQ(empty.Size(), 0);
    }
}

/**
 *  @brief FieldSize reports the number of fields.
 *  @see   lgz::DataFrame.FieldSize() const -> size_t
 */
TYPED_TEST(RM_DataframeProperties, FieldSize)
{
    using Cfg = TypeParam;

    {
        auto const df = Cfg::Build();
        EXPECT_EQ(df.FieldSize(), Cfg::FLD_COUNT);
    }

    {
        typename Cfg::DF const empty;
        EXPECT_EQ(empty.FieldSize(), 0);
    }
}

/**
 *  @brief RecordSize reports the number of records.
 *  @see   lgz::DataFrame.RecordSize() const -> size_t
 */
TYPED_TEST(RM_DataframeProperties, RecordSize)
{
    using Cfg = TypeParam;

    {
        auto const df = Cfg::Build();
        EXPECT_EQ(df.RecordSize(), Cfg::REC_COUNT);
    }

    {
        typename Cfg::DF const empty;
        EXPECT_EQ(empty.RecordSize(), 0);
    }
}

/**
 *  @brief Empty reports whether the frame holds any values.
 *  @see   lgz::DataFrame.Empty() const -> bool
 */
TYPED_TEST(RM_DataframeProperties, Empty)
{
    using Cfg = TypeParam;

    {
        // a populated frame is not empty
        auto const df = Cfg::Build();
        EXPECT_FALSE(df.Empty());
    }

    {
        // a default-constructed frame is empty
        typename Cfg::DF const empty;
        EXPECT_TRUE(empty.Empty());
    }
}
