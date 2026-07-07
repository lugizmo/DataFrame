// Filename: DataFrameMutatingTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions mutating dataframe values.
// The following functions are tested here (with names of tests):
//
// ✅ AssignValue        (typed, every index configuration)
// - AssignValue(FldT const& field, RecT const& record, U&& value) -> bool
//
// ✅ UpsertValue        (value-index only)
// - UpsertValue(FldT const& field, RecT const& record, U&& value) requires DFUnqIndices<FldI, RecI>
//
// ✅ AssignFieldValues
// - AssignFieldValues(FldT const& field, std::span<U const> values) -> bool
//
// ✅ AssignRecordValues
// - AssignRecordValues(RecT const& record, std::span<U const> values) -> bool
//

#include "gtest/gtest.h"

#include <array>
#include <span>
#include <string>

#include "HelperDFTestConfigs.h"

using namespace lgz;
using namespace lgz::test;

namespace {

    // A value type whose constructor is explicit, to verify U&& forwarding constructs T correctly.
    struct ExplicitIntValue
    {
        int value = 0;

        ExplicitIntValue() = default;
        explicit ExplicitIntValue(int const v) : value(v) {}
    };

} // namespace

// ======= AssignValue (index-agnostic) ===========================================================

template<typename>
class RM_DataframeMutating: public testing::Test // NOLINT(readability-identifier-naming)
{
};

TYPED_TEST_SUITE(RM_DataframeMutating, IndexConfigs);

/**
 *  @brief AssignValue writes through at a present cell and fails (without adding indices) otherwise.
 *  @see   lgz::DataFrame.AssignValue(FldT const& field, RecT const& record, U&& value) -> bool
 */
TYPED_TEST(RM_DataframeMutating, AssignValue)
{
    using Cfg = TypeParam;
    auto df   = Cfg::Build();

    {
        // assigning at a present cell succeeds and writes through
        ASSERT_TRUE(df.AssignValue(Cfg::FieldKey(1), Cfg::RecordKey(2), 42));

        auto const val = df.GetValue(Cfg::FieldKey(1), Cfg::RecordKey(2));
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, 42);
    }

    {
        // a missing field or record (off-grid for step != 1) fails and adds no indices
        auto const fields  = df.FieldSize();
        auto const records = df.RecordSize();

        EXPECT_FALSE(df.AssignValue(Cfg::MissingField(), Cfg::RecordKey(0), 1));
        EXPECT_FALSE(df.AssignValue(Cfg::FieldKey(0), Cfg::MissingRecord(), 1));

        EXPECT_EQ(df.FieldSize(), fields);
        EXPECT_EQ(df.RecordSize(), records);
    }
}

// ======= UpsertValue / AssignFieldValues / AssignRecordValues (value-index, with forwarding) =====

/**
 *  @brief UpsertValue inserts a missing field and/or record then assigns; replaces when both exist.
 *  @see   lgz::DataFrame.UpsertValue(FldT const& field, RecT const& record, U&& value) requires DFUnqIndices<FldI, RecI>
 */
TEST(RM_DataframeMutatingValue, UpsertValue)
{
    {
        // upsert inserts the missing side(s), then replaces once both are present
        auto df = DataFrame<int, std::string, std::string>();

        df.UpsertValue("field1", "record1", 1); // inserts field + record
        EXPECT_EQ(df.FieldSize(), 1);
        EXPECT_EQ(df.RecordSize(), 1);
        ASSERT_NE(df.GetValue("field1", "record1"), nullptr);
        EXPECT_EQ(*df.GetValue("field1", "record1"), 1);

        df.UpsertValue("field1", "record2", 2); // inserts record only
        EXPECT_EQ(df.RecordSize(), 2);
        EXPECT_EQ(*df.GetValue("field1", "record2"), 2);

        df.UpsertValue("field2", "record1", 3); // inserts field only
        EXPECT_EQ(df.FieldSize(), 2);
        EXPECT_EQ(*df.GetValue("field2", "record1"), 3);

        df.UpsertValue("field1", "record1", 4); // both present -> replace
        EXPECT_EQ(*df.GetValue("field1", "record1"), 4);
    }

    {
        // forwards a value type with an explicit constructor (insert and replace)
        auto df = DataFrame<ExplicitIntValue, std::string, std::string>();

        df.UpsertValue("field1", "record1", 5);
        ASSERT_NE(df.GetValue("field1", "record1"), nullptr);
        EXPECT_EQ(df.GetValue("field1", "record1")->value, 5);

        df.UpsertValue("field1", "record1", 11);
        EXPECT_EQ(df.GetValue("field1", "record1")->value, 11);
    }
}

/**
 *  @brief AssignFieldValues replaces a whole column when the size matches; rejects otherwise.
 *  @see   lgz::DataFrame.AssignFieldValues(FldT const& field, std::span<U const> values) -> bool
 */
TEST(RM_DataframeMutatingValue, AssignFieldValues)
{
    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"field1", "field2"}), 2);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 3>{"record1", "record2", "record3"}), 3);

    {
        // a matching-size span replaces the whole column
        std::array<int, 3> values = {11, 12, 13};
        ASSERT_TRUE(df.AssignFieldValues("field1", std::span{values}));

        EXPECT_EQ(*df.GetValue("field1", "record1"), 11);
        EXPECT_EQ(*df.GetValue("field1", "record2"), 12);
        EXPECT_EQ(*df.GetValue("field1", "record3"), 13);
    }

    {
        // a wrong-size span or a missing field is rejected and changes nothing
        std::array<int, 2> wrongSize = {101, 102};
        EXPECT_FALSE(df.AssignFieldValues("field1", std::span{wrongSize}));
        EXPECT_EQ(*df.GetValue("field1", "record1"), 11);

        std::array<int, 3> values = {1, 2, 3};
        EXPECT_FALSE(df.AssignFieldValues("missing", std::span{values}));
        EXPECT_EQ(df.FieldSize(), 2);
    }

    {
        // forwards an explicit-constructor value type
        auto dfx = DataFrame<ExplicitIntValue, std::string, std::string>();
        ASSERT_EQ(dfx.AddFields(std::array<std::string, 1>{"field1"}), 1);
        ASSERT_EQ(dfx.AddRecords(std::array<std::string, 2>{"record1", "record2"}), 2);

        std::array<int, 2> values = {31, 32};
        ASSERT_TRUE(dfx.AssignFieldValues("field1", std::span{values}));
        EXPECT_EQ(dfx.GetValue("field1", "record1")->value, 31);
        EXPECT_EQ(dfx.GetValue("field1", "record2")->value, 32);
    }
}

/**
 *  @brief AssignRecordValues replaces a whole row when the size matches; rejects otherwise.
 *  @see   lgz::DataFrame.AssignRecordValues(RecT const& record, std::span<U const> values) -> bool
 */
TEST(RM_DataframeMutatingValue, AssignRecordValues)
{
    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 3>{"field1", "field2", "field3"}), 3);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"record1", "record2"}), 2);

    {
        // a matching-size span replaces the whole row
        std::array<int, 3> values = {41, 42, 43};
        ASSERT_TRUE(df.AssignRecordValues("record2", std::span{values}));

        EXPECT_EQ(*df.GetValue("field1", "record2"), 41);
        EXPECT_EQ(*df.GetValue("field2", "record2"), 42);
        EXPECT_EQ(*df.GetValue("field3", "record2"), 43);
    }

    {
        // a wrong-size span or a missing record is rejected and changes nothing
        std::array<int, 2> wrongSize = {71, 72};
        EXPECT_FALSE(df.AssignRecordValues("record2", std::span{wrongSize}));
        EXPECT_EQ(*df.GetValue("field1", "record2"), 41);

        std::array<int, 3> values = {1, 2, 3};
        EXPECT_FALSE(df.AssignRecordValues("missing", std::span{values}));
        EXPECT_EQ(df.RecordSize(), 2);
    }

    {
        // forwards an explicit-constructor value type
        auto dfx = DataFrame<ExplicitIntValue, std::string, std::string>();
        ASSERT_EQ(dfx.AddFields(std::array<std::string, 2>{"field1", "field2"}), 2);
        ASSERT_EQ(dfx.AddRecords(std::array<std::string, 1>{"record1"}), 1);

        std::array<int, 2> values = {81, 82};
        ASSERT_TRUE(dfx.AssignRecordValues("record1", std::span{values}));
        EXPECT_EQ(dfx.GetValue("field1", "record1")->value, 81);
        EXPECT_EQ(dfx.GetValue("field2", "record1")->value, 82);
    }
}
