// Filename: IndexUniqueTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <array>
#include <string>

#include "lugizmo/dataframe/IndexUnique.h"

inline auto CreateTestStringIndexUnique() -> lugizmo::DFUniqueIndex<std::string>
{
    lugizmo::DFUniqueIndex<std::string> index;

    auto const positions = std::array<std::string, 3>{"first", "second", "third"};
    index.AddMultiple(positions);

    return index;
}

TEST(lugizmo_dataframe_index_unqiue_test, add_string)
{
    lugizmo::DFUniqueIndex<std::string> index;

    auto const pos1 = index.Add("first");
    auto const pos2 = index.Add("second");
    auto const pos3 = index.Add("third");

    ASSERT_TRUE(pos1.has_value());
    ASSERT_TRUE(pos2.has_value());
    ASSERT_TRUE(pos3.has_value());
    ASSERT_EQ(pos1, 0);
    ASSERT_EQ(pos2, 1);
    ASSERT_EQ(pos3, 2);

    auto const positions = std::array<std::string, 3>{"fourth", "fifth", "sixth"};
    index.AddMultiple(positions);

    ASSERT_EQ(index.Position("fourth"), 3);
    ASSERT_EQ(index.Position("fifth"), 4);
    ASSERT_EQ(index.Position("sixth"), 5);
}

TEST(lugizmo_dataframe_index_unqiue_test, get_string)
{
    auto const index = CreateTestStringIndexUnique();

    ASSERT_EQ(index.Size(), 3);
    ASSERT_EQ(index.Empty(), false);
    ASSERT_EQ(index.MaxPosition(), 2);

    ASSERT_EQ(index.Key(0), "first");
    ASSERT_EQ(index.Key(1), "second");
    ASSERT_EQ(index.Key(2), "third");
    ASSERT_EQ(index.Position("first"), 0);
    ASSERT_EQ(index.Position("second"), 1);
    ASSERT_EQ(index.Position("third"), 2);

    auto const keys = index.Keys();
    auto const poss = index.Positions();

    ASSERT_EQ(keys[0], "first");
    ASSERT_EQ(keys[1], "second");
    ASSERT_EQ(keys[2], "third");

    ASSERT_EQ(poss[0], 0);
    ASSERT_EQ(poss[1], 1);
    ASSERT_EQ(poss[2], 2);
}

TEST(lugizmo_dataframe_index_unqiue_test, drop_string)
{
    auto index = CreateTestStringIndexUnique();

    index.Drop("first");
    ASSERT_EQ(index.Size(), 2);
    ASSERT_EQ(index.Empty(), false);
    ASSERT_EQ(index.MaxPosition(), 1);

    index.Drop("third");
    ASSERT_EQ(index.Size(), 1);
    ASSERT_EQ(index.Empty(), false);
    ASSERT_EQ(index.MaxPosition(), 0);

    index.Drop("second");
    ASSERT_EQ(index.Size(), 0);
    ASSERT_EQ(index.Empty(), true);
    ASSERT_EQ(index.MaxPosition(), std::optional<std::size_t>());

    auto indexMiddle = CreateTestStringIndexUnique();
    indexMiddle.Drop("second");
    ASSERT_EQ(indexMiddle.Size(), 2);
    ASSERT_EQ(indexMiddle.Empty(), false);
    ASSERT_EQ(indexMiddle.MaxPosition(), 1);
}