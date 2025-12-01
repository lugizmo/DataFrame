// Filename: DataFrameTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include <memory_resource>
#include <string>
#include <algorithm>
#include <cctype>
#include <array>
#include <span>

#include "gtest/gtest.h"

#include "lugizmo/container/DataFrameMap.h"

TEST(lugizmo_container_dataframe_map_test, default_map)
{
    using namespace lugizmo;

    auto const defaultMap = DataFrameMap<int, int>();
    ASSERT_TRUE(defaultMap.Empty());
    ASSERT_EQ(defaultMap.Size(), 0);

    ASSERT_TRUE(defaultMap.Keys().empty());
    ASSERT_TRUE(defaultMap.Values().empty());
}

TEST(lugizmo_container_dataframe_map_test, default_with_allocator_map)
{
    using namespace lugizmo;
    auto allocator = std::pmr::monotonic_buffer_resource();

    auto const defaultMap = DataFrameMap<int, int>(&allocator);
    ASSERT_TRUE(defaultMap.Empty());
    ASSERT_EQ(defaultMap.Size(), 0);

    auto const* refAllocator = defaultMap.Allocator().resource();
    ASSERT_EQ(refAllocator, &allocator);
}

TEST(lugizmo_container_dataframe_map_test, get_set)
{
    using namespace lugizmo;

    auto defaultMap = DataFrameMap<int, std::string>();

    // reserve capacity
    defaultMap.Reserve(3);
    ASSERT_EQ(defaultMap.Capacity(), 3);

    // insert values
    defaultMap.Insert(1, "first");
    defaultMap.Insert(2, "second");
    defaultMap.Insert(3, "third");

    ASSERT_FALSE(defaultMap.Empty());
    ASSERT_EQ(defaultMap.Size(), 3);
    ASSERT_EQ(defaultMap.Get(1), "first");
    ASSERT_EQ(defaultMap.Get(2), "second");
    ASSERT_EQ(defaultMap.Get(3), "third");

    // insert multiple values at once
    defaultMap.Insert(std::array{4, 5, 6}, std::array<std::string, 3>{"fourth", "fifth", "sixth"});
    ASSERT_EQ(defaultMap.Size(), 6);
    ASSERT_EQ(defaultMap.Get(4), "fourth");
    ASSERT_EQ(defaultMap.Get(5), "fifth");
    ASSERT_EQ(defaultMap.Get(6), "sixth");

    // insert multiple values with wrong sizes
    auto const sizeBeforeWrongInsert = defaultMap.Size();
    defaultMap.Insert(std::array{100, 200}, std::array<std::string, 3>{"one-hundred", "two-hundred", "three-hundred"});
    ASSERT_EQ(defaultMap.Size(), sizeBeforeWrongInsert);

    defaultMap.Insert(std::array{100, 200, 300}, std::array<std::string, 2>{"one-hundred", "two-hundred"});
    ASSERT_EQ(defaultMap.Size(), sizeBeforeWrongInsert);

    // insert and mutate value
    defaultMap.Insert(7, "seven");
    ASSERT_EQ(defaultMap.Get(7), "seven");

    defaultMap.Set(7, "seventh");
    ASSERT_EQ(defaultMap.Get(7), "seventh");
}

TEST(lugizmo_container_dataframe_map_test, ordered)
{
    using namespace lugizmo;
    auto        defaultMap      = DataFrameMap<int, std::string>();
    auto const& defaultMapConst = defaultMap;

    // reserve capacity
    defaultMap.Reserve(3);
    ASSERT_EQ(defaultMap.Capacity(), 3);

    // insert values
    defaultMap.Insert(1, "first");
    defaultMap.Insert(2, "second");
    defaultMap.Insert(3, "third");

    EXPECT_EQ(defaultMapConst.Get(1), std::optional<std::string>("first"));
    EXPECT_EQ(defaultMapConst.Get(2), std::optional<std::string>("second"));
    EXPECT_EQ(defaultMapConst.Get(3), std::optional<std::string>("third"));

    auto const viewFldsInit = defaultMapConst.Keys();
    auto const viewRecsInit = defaultMapConst.Values();
    ASSERT_EQ(viewFldsInit.size(), 3);
    EXPECT_EQ(viewFldsInit[0], 1);
    EXPECT_EQ(viewFldsInit[1], 2);
    EXPECT_EQ(viewFldsInit[2], 3);
    ASSERT_EQ(viewRecsInit.size(), 3);
    EXPECT_EQ(viewRecsInit[0], "first");
    EXPECT_EQ(viewRecsInit[1], "second");
    EXPECT_EQ(viewRecsInit[2], "third");

    defaultMap.Erase(2);
    ASSERT_EQ(defaultMap.Size(), 2);

    defaultMap.Insert(2, "second");
    ASSERT_EQ(defaultMap.Size(), 3);

    EXPECT_EQ(defaultMapConst.Get(1), std::optional<std::string>("first"));
    EXPECT_EQ(defaultMapConst.Get(2), std::optional<std::string>("second"));
    EXPECT_EQ(defaultMapConst.Get(3), std::optional<std::string>("third"));

    auto const viewFldsRem = defaultMapConst.Keys();
    auto const viewRecsRem = defaultMapConst.Values();
    ASSERT_EQ(viewFldsRem.size(), 3);
    EXPECT_EQ(viewFldsRem[0], 1);
    EXPECT_EQ(viewFldsRem[1], 3);
    EXPECT_EQ(viewFldsRem[2], 2);
    ASSERT_EQ(viewRecsRem.size(), 3);
    EXPECT_EQ(viewRecsRem[0], "first");
    EXPECT_EQ(viewRecsRem[1], "third");
    EXPECT_EQ(viewRecsRem[2], "second");
}

TEST(lugizmo_container_dataframe_map_test, erase)
{
    using namespace lugizmo;

    auto map = DataFrameMap<int, std::string>();
    map.Insert(1, "first");
    map.Insert(2, "second");
    map.Insert(3, "third");
    ASSERT_EQ(map.Size(), 3);

    map.Erase(1);
    ASSERT_EQ(map.Size(), 2);
    ASSERT_EQ(map.Get(1), std::nullopt);

    map.Erase(2);
    ASSERT_EQ(map.Size(), 1);
    ASSERT_EQ(map.Get(2), std::nullopt);

    map.Erase(3);
    ASSERT_EQ(map.Size(), 0);
    ASSERT_EQ(map.Get(3), std::nullopt);
}

TEST(lugizmo_container_dataframe_map_test, find)
{
    using namespace lugizmo;

    auto map = DataFrameMap<int, std::string>();
    map.Insert(1, "first");
    map.Insert(2, "second");
    map.Insert(3, "third");

    ASSERT_TRUE(map.Contains(1));
    ASSERT_TRUE(map.Contains(2));
    ASSERT_TRUE(map.Contains(3));
    ASSERT_EQ(map.Get(1), "first");
    ASSERT_EQ(map.Get(2), "second");
    ASSERT_EQ(map.Get(3), "third");

    auto const& constMap = map;
    auto const [cKey1, cVal1] = constMap.Find(1);
    auto const [cKey2, cVal2] = constMap.Find(2);
    auto const [cKey3, cVal3] = constMap.Find(3);

    ASSERT_EQ(*cKey1, 1);
    ASSERT_EQ(*cKey2, 2);
    ASSERT_EQ(*cKey3, 3);
    ASSERT_EQ(*cVal1, "first");
    ASSERT_EQ(*cVal2, "second");
    ASSERT_EQ(*cVal3, "third");

    auto [key1, val1] = map.Find(1);
    auto [key2, val2] = map.Find(2);
    auto [key3, val3] = map.Find(3);

    // keys are always const
    //*key1 = 4;
    //*key2 = 5;
    //*key3 = 6;

    // values can be mutated if map is not const
    *val1 = std::string("firstly");
    *val2 = std::string("secondly");
    *val3 = std::string("thirdly");

    ASSERT_EQ(*key1, 1);
    ASSERT_EQ(*key2, 2);
    ASSERT_EQ(*key3, 3);
    ASSERT_EQ(*cVal1, "firstly");
    ASSERT_EQ(*cVal2, "secondly");
    ASSERT_EQ(*cVal3, "thirdly");
}