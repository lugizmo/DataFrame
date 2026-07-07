// Filename: DataFrameConstructTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test constructing a dataframe from constructors and the FromFields* helpers.
// The following functions are tested here (with names of tests):
//
// Typed across every index configuration (suite RM_DataFrameConstruct):
//
// ✅ DefaultConstructor  - DataFrame(MemRsc res)
// ✅ ReservedConstructor - DataFrame(size_t reservedValues, MemRsc res)
//
// Value-index family -- FromFields* take spans/initializer-lists (suite RM_DataFrameConstructValue):
//
// ✅ DefaultLayout       - DataFrame::Layout is DFRowMajor
// ✅ FromFields          - FromFields(span [, reservedValues])
// ✅ FromFieldsAndRecord - FromFieldsAndRecord(fields, records [, recValues])
// ✅ FromFieldsAndRecords- FromFieldsAndRecords(fields, records, iterable / initializer_list)
//
// (Copy/move construction is covered in RM_DataFrameTorsTest; range-index construction via
//  DFRangeIndexBounds is exercised in the RangeIndices examples.)
//

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <string>
#include <type_traits>

#include "HelperDFTestConfigs.h"

using namespace lgz;
using namespace lgz::test;

// ======= Constructors (index-agnostic) ==========================================================

template<typename>
class RM_DataFrameConstruct: public testing::Test // NOLINT(readability-identifier-naming)
{
};

TYPED_TEST_SUITE(RM_DataFrameConstruct, IndexConfigs);

/**
 *  @brief A default-constructed frame is empty and allocates nothing.
 *  @see   lgz::DataFrame::DataFrame(MemRsc res)
 */
TYPED_TEST(RM_DataFrameConstruct, DefaultConstructor)
{
    using Cfg = TypeParam;
    typename Cfg::DF const df;

    EXPECT_TRUE(df.Empty());
    EXPECT_EQ(df.Size(), 0);
    EXPECT_EQ(df.FieldSize(), 0);
    EXPECT_EQ(df.RecordSize(), 0);
    EXPECT_EQ(df.Data(), nullptr); // no reservation -> no allocation
}

/**
 *  @brief Reserving up front allocates the value buffer but leaves the frame empty.
 *  @see   lgz::DataFrame::DataFrame(size_t reservedValues, MemRsc res)
 */
TYPED_TEST(RM_DataFrameConstruct, ReservedConstructor)
{
    using Cfg = TypeParam;
    typename Cfg::DF const df{50};

    EXPECT_TRUE(df.Empty());
    EXPECT_EQ(df.Size(), 0);
    EXPECT_EQ(df.FieldSize(), 0);
    EXPECT_EQ(df.RecordSize(), 0);
    EXPECT_NE(df.Data(), nullptr); // reserved -> allocated
}

// ======= FromFields* helpers (value-index family) ===============================================

/**
 *  @brief The default layout is row-major (DFRowMajor / std::layout_right).
 */
TEST(RM_DataFrameConstructValue, DefaultLayout)
{
    static_assert(std::is_same_v<DataFrame<int, int, int>::Layout, DFRowMajor<int>>);
}

/**
 *  @brief FromFields creates a frame with the given fields and no records.
 *  @see   lgz::DataFrame::FromFields(fields, reservedValues, res)
 */
TEST(RM_DataFrameConstructValue, FromFields)
{
    using DF              = DataFrame<int, int, int>;
    constexpr auto fields = std::array{1, 2, 3, 4};

    {
        // without reservation: the fields are present, no values yet
        auto const df = DF::FromFields(fields);
        EXPECT_TRUE(df.Empty());
        EXPECT_EQ(df.Size(), 0);
        EXPECT_EQ(df.FieldSize(), fields.size());
        EXPECT_EQ(df.RecordSize(), 0);

        std::size_t i = 0;
        for (auto const fld : df.Fields()) EXPECT_EQ(fld, fields.at(i++));
        EXPECT_EQ(i, fields.size());
    }

    {
        // with reservation: the value buffer is allocated up front
        auto const df = DF::FromFields(fields, 5);
        EXPECT_EQ(df.FieldSize(), fields.size());
        EXPECT_NE(df.Data(), nullptr);
    }
}

/**
 *  @brief FromFieldsAndRecord builds a populated frame; one value row is reused for every record.
 *  @see   lgz::DataFrame::FromFieldsAndRecord(fields, records, recValues)
 */
TEST(RM_DataFrameConstructValue, FromFieldsAndRecord)
{
    using DF        = DataFrame<float, std::string, int>;
    auto const flds = std::array{std::string("0"), std::string("1"), std::string("2")};
    auto const recs = std::array{0, 1, 2};

    {
        // defaulted values fill every cell with T{}
        auto const df = DF::FromFieldsAndRecord(flds, recs);
        ASSERT_EQ(df.FieldSize(), 3);
        ASSERT_EQ(df.RecordSize(), 3);
        EXPECT_EQ(df.Size(), 9);

        for (auto const value : df.Values()) EXPECT_EQ(value, float());
    }

    {
        // a per-record value row is reused for each record
        auto const data = std::array{1.f, 2.f, 3.f};
        auto const df   = DF::FromFieldsAndRecord(flds, recs, data);
        ASSERT_EQ(df.Size(), 9);

        for (auto const record : df.Records())
        {
            std::size_t f = 0;
            for (auto const value : df.ViewRecord(record)) EXPECT_EQ(value, data.at(f++));
        }
    }
}

/**
 *  @brief FromFieldsAndRecords initializes every value explicitly (iterable or initializer-list).
 *  @see   lgz::DataFrame::FromFieldsAndRecords(fields, records, recValues)
 */
TEST(RM_DataFrameConstructValue, FromFieldsAndRecords)
{
    using DF        = DataFrame<float, std::string, int>;
    auto const flds = std::array{std::string("0"), std::string("1"), std::string("2")};
    auto const recs = std::array{0, 1, 2};
    auto const data = std::array{std::array{1.f, 2.f, 3.f}, std::array{4.f, 5.f, 6.f}, std::array{7.f, 8.f, 9.f}};

    {
        // from an iterable of value rows
        auto const df = DF::FromFieldsAndRecords(flds, recs, data);
        ASSERT_EQ(df.FieldSize(), 3);
        ASSERT_EQ(df.RecordSize(), 3);

        for (std::size_t r = 0; r < 3; ++r)
        {
            for (std::size_t f = 0; f < 3; ++f)
            {
                auto const value = df.GetValue(flds.at(f), recs.at(r));
                ASSERT_NE(value, nullptr);
                EXPECT_EQ(*value, data.at(r).at(f));
            }
        }
    }

    {
        // from initializer lists (fields, records, and value rows)
        auto const df = DF::FromFieldsAndRecords({"0", "1", "2"}, {0, 1, 2},
                                                 {{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}, {7.f, 8.f, 9.f}});
        ASSERT_EQ(df.FieldSize(), 3);
        ASSERT_EQ(df.RecordSize(), 3);

        for (std::size_t r = 0; r < 3; ++r)
        {
            for (std::size_t f = 0; f < 3; ++f)
            {
                auto const value = df.GetValue(flds.at(f), recs.at(r));
                ASSERT_NE(value, nullptr);
                EXPECT_EQ(*value, data.at(r).at(f));
            }
        }
    }
}
