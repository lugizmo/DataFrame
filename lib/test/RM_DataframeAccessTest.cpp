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
// - GetValue(FldT const& field, RecT const& record) const -> OptionalRef<T const>
// - GetValue(FldT const& field, RecT const& record) -> OptionalRef<T>
//
// ✅ get_val_op
// - operator[](FldT const& field, RecT const& record) -> T&
// - operator[](FldT const& field, RecT const& record) const -> T const&
//
// ✅ get_raw_mem
// - Data() const -> T const*
//
// ✅ get_mdspan
// - MDSpan() const -> RecsData<T const>

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include "RM_DataframeTestData.h"

/**
 *  @brief Getting individual values from the dataframe.
 *  @see   lugizmo::Dataframe.GetValue(FldT const& field, RecT const& record) const -> OptionalRef<T const>
 *         lugizmo::Dataframe.GetValue(FldT const& field, RecT const& record) -> OptionalRef<T>
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
                ASSERT_TRUE(val.HasValue());
                EXPECT_EQ(*val, DFData[recCount][fldCount]);
                recCount++;
            }
            fldCount++;
        }

        // not present
        auto const valMiss = df.GetValue(1000, 1000);
        ASSERT_FALSE(valMiss.HasValue());

        // mutable version
        [[maybe_unused]] auto dfM = DefaultDataframe();
        [[maybe_unused]] auto val = dfM.GetValue(DFFields[0], DFRecords[0]);
        static_assert(not std::is_const_v<decltype(val)>);
        static_assert(not std::is_const_v<decltype(val.Value())>);
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
                ASSERT_TRUE(val.HasValue());
                EXPECT_EQ(*val, DFData[recCount][fldCount]);
                recCount++;
            }
            fldCount++;
        }

        // not present
        auto const valMiss = df.GetValue("1000", "1000");
        ASSERT_FALSE(valMiss.HasValue());

        // mutable version
        [[maybe_unused]] auto dfM = DefaultDataframe();
        [[maybe_unused]] auto val = dfM.GetValue(DFFields[0], DFRecords[0]);
        static_assert(not std::is_const_v<decltype(val)>);
        static_assert(not std::is_const_v<decltype(val.Value())>);
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
 *  @see   lugizmo::Dataframe.Data() const -> std::optional<T const*>
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
    ASSERT_TRUE(data != nullptr);

    for(size_t rec = 0; rec < DFRecCount; rec++)
    {
        for(size_t fld = 0; fld < DFFldCount; fld++)
        {
            ASSERT_EQ(data[rec * DFFldCount + fld], DFData[rec][fld]);
        }
    }
}

/**
 *  @brief Getting raw underlying memory as a mdspan.
 *  @see   lugizmo::Dataframe.MDSpan() const -> RecsData<T const>
 */
TEST(lugizmo_dataframe_access_row_major, get_mdspan)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;
    using namespace lugizmo::test::integer;

    // empty span
    auto const dfEmpty = DataFrame<float, int, int>{10};
    auto const mdEmpty = dfEmpty.MDSpan();
    ASSERT_TRUE(mdEmpty.empty());
    ASSERT_EQ(mdEmpty.size(), 0);
    ASSERT_EQ(mdEmpty.extent(0), 0);
    ASSERT_EQ(mdEmpty.extent(1), 0);

    // const version
    auto const df = DefaultDataframe();
    auto const md = df.MDSpan();

    ASSERT_EQ(md.extent(0), DFRecCount);
    ASSERT_EQ(md.extent(1), DFFldCount);
    ASSERT_EQ(md.size(), DFRecCount * DFFldCount);
    ASSERT_EQ(md.data_handle(), df.Data());

    // verify a couple of specific positions
    {
        auto const v00      = md[0, 0];
        auto const v0_last  = md[0, DFFldCount - 1];
        auto const vLast0   = md[DFRecCount - 1, 0];
        auto const vLastLast= md[DFRecCount - 1, DFFldCount - 1];

        EXPECT_EQ(v00,          DFData[0][0]);
        EXPECT_EQ(v0_last,      DFData[0][DFFldCount - 1]);
        EXPECT_EQ(vLast0,       DFData[DFRecCount - 1][0]);
        EXPECT_EQ(vLastLast,    DFData[DFRecCount - 1][DFFldCount - 1]);
    }

    // cross‑check every element against DFData (small fixed-size test data)
    for (std::size_t r = 0; r < DFRecCount; ++r)
    {
        for (std::size_t c = 0; c < DFFldCount; ++c)
        {
            auto const value = md[r, c];
            auto const expected = DFData[r][c];
            EXPECT_EQ(value, expected) << "mismatch at (" << r << "," << c << ")";
        }
    }
}
