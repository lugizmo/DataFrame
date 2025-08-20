// Filename: RM_DataframeAccessTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions accessing dataframe values.
// The following functions are tested here (with names of tests):
//
// ✅ get_val
// - GetValue(FldT const& field, RecT const& record) const -> std::optional<std::reference_wrapper<T const>>
// - GetValue(FldT const& field, RecT const& record) -> std::optional<std::reference_wrapper<T>>
//
// ✅ get_val_op
// - operator[](FldT const& field, RecT const& record) -> T&
// - operator[](FldT const& field, RecT const& record) const -> T const&
//
// ✅ get_raw_mem
// - Data() const -> T const*

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include "RM_DataframeTestData.h"

/**
 *  @brief Getting individual values from the dataframe.
 *  @see   lugizmo::Dataframe.GetValue(FldT const& field, RecT const& record) const -> std::optional<std::reference_wrapper<T const>>
 *         lugizmo::Dataframe.GetValue(FldT const& field, RecT const& record) -> std::optional<std::reference_wrapper<T>>
 */
TEST(lugizmo_dataframe_access_row_major, get_val)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const version
        auto const df   = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const recName : DFRecords)
            {
                auto const val = df.GetValue(fldName, recName);
                ASSERT_TRUE(val.has_value());
                EXPECT_EQ(*val, DFData[recCount][fldCount]);
                recCount++;
            }
            fldCount++;
        }

        // not present
        auto const valMiss = df.GetValue(1000, 1000);
        ASSERT_FALSE(valMiss.has_value());

        // mutable version
        [[maybe_unused]] auto dfM = DefaultDataframe();
        [[maybe_unused]] auto val = dfM.GetValue(DFFields[0], DFRecords[0]);
        static_assert(not std::is_const_v<decltype(val)>);
        static_assert(not std::is_const_v<decltype(val.value())>);
    }

    {
        using namespace lugizmo::test::str;

        auto const df   = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const& recName : DFRecords)
            {
                auto const val = df.GetValue(fldName, recName);
                ASSERT_TRUE(val.has_value());
                EXPECT_EQ(*val, DFData[recCount][fldCount]);
                recCount++;
            }
            fldCount++;
        }

        // not present
        auto const valMiss = df.GetValue("1000", "1000");
        ASSERT_FALSE(valMiss.has_value());

        // mutable version
        [[maybe_unused]] auto dfM = DefaultDataframe();
        [[maybe_unused]] auto val = dfM.GetValue(DFFields[0], DFRecords[0]);
        static_assert(not std::is_const_v<decltype(val)>);
        static_assert(not std::is_const_v<decltype(val.value())>);
    }
}

/**
 *  @brief Getting individual values from the dataframe using unsafe operator[].
 *  @see   lugizmo::Dataframe.operator[](FldT const& field, RecT const& record) -> T&
 *         lugizmo::Dataframe.operator[](FldT const& field, RecT const& record) const -> T const&
 */
TEST(lugizmo_dataframe_access_row_major, get_val_op)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const version
        auto const df   = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const recName : DFRecords)
            {
                auto const val = df[fldName, recName];
                EXPECT_EQ(val, DFData[recCount][fldCount]);
                recCount++;
            }
            fldCount++;
        }

        // mutable version
        [[maybe_unused]] auto dfM  = DefaultDataframe();
        [[maybe_unused]] auto& val = dfM[DFFields[0], DFRecords[0]];
        static_assert(not std::is_const_v<decltype(val)>);
    }

    {
        using namespace lugizmo::test::str;

        auto const df   = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            size_t recCount = 0;
            for(auto const& recName : DFRecords)
            {
                auto const val = df[fldName, recName];
                EXPECT_EQ(val, DFData[recCount][fldCount]);
                recCount++;
            }
            fldCount++;
        }

        // mutable version
        [[maybe_unused]] auto  dfM = DefaultDataframe();
        [[maybe_unused]] auto& val = dfM[DFFields[0], DFRecords[0]];
        static_assert(not std::is_const_v<decltype(val)>);
    }
}

/**
 *  @brief Getting raw underlying memory of dataframe.
 *  @see   lugizmo::Dataframe.Data() const -> T const*
 */
TEST(lugizmo_dataframe_access_row_major, get_raw_mem)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;
    using namespace lugizmo::test::integer;

    // const version
    auto const df   = DefaultDataframe();
    auto const data = df.Data();

    for(size_t rec = 0; rec < DFRecCount; rec++)
    {
        for(size_t fld = 0; fld < DFFldCount; fld++)
        {
            ASSERT_EQ(data[rec * DFFldCount + fld], DFData[rec][fld]);
        }
    }
}