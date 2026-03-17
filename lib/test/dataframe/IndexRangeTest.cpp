// Filename: IndexUniqueTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <optional>
#include <string>

#include "lugizmo/dataframe/IndexRange.h"

TEST(lugizmo_dataframe_index_range_test, int_default_index)
{
    constexpr auto index = lugizmo::DFRangeIndex<int>();

    ASSERT_TRUE(index.Empty());
    ASSERT_TRUE(index.Size() == 0);
    ASSERT_EQ(index.LowerBound(), 0);
    ASSERT_EQ(index.UpperBound(), 0);
}

TEST(lugizmo_dataframe_index_range_test, int_wrong_init)
{
    constexpr auto index = lugizmo::DFRangeIndex(10, 5);
    ASSERT_TRUE(index.Empty());
    ASSERT_TRUE(index.Size() == 0);
    ASSERT_EQ(index.LowerBound(), 0);
    ASSERT_EQ(index.UpperBound(), 0);
}

TEST(lugizmo_dataframe_index_range_test, int_get)
{
    constexpr auto index = lugizmo::DFRangeIndex(0, 10);
    ASSERT_FALSE(index.Empty());
    ASSERT_EQ(index.Size(), 10);
    ASSERT_EQ(index.LowerBound(), 0);
    ASSERT_EQ(index.UpperBound(), 10);
}

TEST(lugizmo_dataframe_index_range_test, int_in_bound)
{
    constexpr auto index = lugizmo::DFRangeIndex(0, 10);

    ASSERT_FALSE(index.InBound(-2));
    ASSERT_FALSE(index.InBound(-1));
    ASSERT_TRUE(index.InBound(0));
    ASSERT_TRUE(index.InBound(1));
    ASSERT_TRUE(index.InBound(2));
    ASSERT_TRUE(index.InBound(3));
    ASSERT_TRUE(index.InBound(4));
    ASSERT_TRUE(index.InBound(5));
    ASSERT_TRUE(index.InBound(6));
    ASSERT_TRUE(index.InBound(7));
    ASSERT_TRUE(index.InBound(8));
    ASSERT_TRUE(index.InBound(9));
    ASSERT_FALSE(index.InBound(10));
    ASSERT_FALSE(index.InBound(11));
}

TEST(lugizmo_dataframe_index_range_test, int_change)
{
    // positive values index
    {
        auto index = lugizmo::DFRangeIndex(0, 10);
        ASSERT_FALSE(index.SetLowerBound(11).has_value());
        ASSERT_FALSE(index.SetUpperBound(-1).has_value());

        auto const removeLower = index.SetLowerBound(2);
        ASSERT_TRUE(removeLower.has_value() && removeLower.value() == -2);
        ASSERT_TRUE(index.Size() == 8);

        auto const removeUpper = index.SetUpperBound(8);
        ASSERT_TRUE(removeUpper.has_value() && removeUpper.value() == -2);
        ASSERT_TRUE(index.Size() == 6);

        auto const addLower = index.SetLowerBound(1);
        ASSERT_TRUE(addLower.has_value() && addLower.value() == 1);
        ASSERT_TRUE(index.Size() == 7);

        auto const addUpper = index.SetUpperBound(11);
        ASSERT_TRUE(addUpper.has_value() && addUpper.value() == 3);
        ASSERT_TRUE(index.Size() == 10);
    }

    // negative values index
    {
        auto index = lugizmo::DFRangeIndex(-10, 0);
        ASSERT_FALSE(index.SetLowerBound(1).has_value());
        ASSERT_FALSE(index.SetUpperBound(-11).has_value());

        auto const removeLower = index.SetLowerBound(-8);
        ASSERT_TRUE(removeLower.has_value() && removeLower.value() == -2);
        ASSERT_TRUE(index.Size() == 8);

        auto const removeUpper = index.SetUpperBound(-2);
        ASSERT_TRUE(removeUpper.has_value() && removeUpper.value() == -2);
        ASSERT_TRUE(index.Size() == 6);

        auto const addLower = index.SetLowerBound(-12);
        ASSERT_TRUE(addLower.has_value() && addLower.value() == 4);
        ASSERT_TRUE(index.Size() == 10);

        auto const addUpper = index.SetUpperBound(-1);
        ASSERT_TRUE(addUpper.has_value() && addUpper.value() == 1);
        ASSERT_TRUE(index.Size() == 11);
    }

    // sign switch values index
    {
        auto index = lugizmo::DFRangeIndex(-10, 10);
        ASSERT_FALSE(index.SetLowerBound(11).has_value());
        ASSERT_FALSE(index.SetUpperBound(-11).has_value());

        // without sign changes
        auto const removeLower = index.SetLowerBound(-8);
        ASSERT_TRUE(removeLower.has_value() && removeLower.value() == -2);
        ASSERT_TRUE(index.Size() == 18);

        auto const removeUpper = index.SetUpperBound(8);
        ASSERT_TRUE(removeUpper.has_value() && removeUpper.value() == -2);
        ASSERT_TRUE(index.Size() == 16);

        auto const addLower = index.SetLowerBound(-15);
        ASSERT_TRUE(addLower.has_value() && addLower.value() == 7);
        ASSERT_TRUE(index.Size() == 23);

        auto const addUpper = index.SetUpperBound(15);
        ASSERT_TRUE(addUpper.has_value() && addUpper.value() == 7);
        ASSERT_TRUE(index.Size() == 30);

        // with sign changes
        auto const removeLowerSig = index.SetLowerBound(1);
        ASSERT_TRUE(removeLowerSig.has_value() && removeLowerSig.value() == -16);
        ASSERT_TRUE(index.Size() == 14);

        auto const addLowerBack = index.SetLowerBound(-5);
        ASSERT_TRUE(addLowerBack.has_value() && addLowerBack.value() == 6);
        ASSERT_TRUE(index.Size() == 20);

        auto const removeUpperSig = index.SetUpperBound(-2);
        ASSERT_TRUE(removeUpperSig.has_value() && removeUpperSig.value() == -17);
        ASSERT_TRUE(index.Size() == 3);

        auto const addUpperBack = index.SetUpperBound(5);
        ASSERT_TRUE(addUpperBack.has_value() && addUpperBack.value() == 7);
        ASSERT_TRUE(index.Size() == 10);
    }
}

TEST(lugizmo_dataframe_index_range_test, copy_constructor)
{
    auto const index = lugizmo::DFRangeIndex(-5, 7);
    auto const copy  = lugizmo::DFRangeIndex(index);

    ASSERT_EQ(copy.LowerBound(), -5);
    ASSERT_EQ(copy.UpperBound(), 7);
    ASSERT_EQ(copy.Size(), 12);
    ASSERT_TRUE(copy.InBound(-5));
    ASSERT_TRUE(copy.InBound(6));
    ASSERT_FALSE(copy.InBound(7));
}
