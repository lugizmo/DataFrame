// Filename: IndexUniqueTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the whole DFUniqueIndex value index.
// The following functions are tested here (with names of tests):
//
// ✅ Construct    - DFUniqueIndex() / DFUniqueIndex(std::pmr::memory_resource*, size_t)
// ✅ Add          - Add(T&&)
// ✅ AddMultiple  - AddMultiple(std::span<KeyType const>)
// ✅ Has          - Has(C const&)
// ✅ Key          - Key(size_t)
// ✅ Position     - Position(C const&)
// ✅ Access       - Keys() / Size() / Empty() / MaxPosition()
// ✅ Drop         - Drop(T const&)
// ✅ Sort         - Sort(Compare comp = {})
// ✅ Tors         - copy / move ctor + assignment / DFUniqueIndex(const&, memory_resource*)
//

#include "gtest/gtest.h"

#include <array>
#include <functional>
#include <memory_resource>
#include <string>

#include "lugizmo/dataframe/IndexUnique.h"

inline auto CreateTestStringIndexUnique() -> lugizmo::DFUniqueIndex<std::string>
{
    lugizmo::DFUniqueIndex<std::string> index;

    auto const positions = std::array<std::string, 3>{"first", "second", "third"};
    index.AddMultiple(positions);

    return index;
}

/**
 *  @brief Constructing a unique index, default and with a memory resource.
 *  @see   lugizmo::DFUniqueIndex::DFUniqueIndex()
 *         lugizmo::DFUniqueIndex::DFUniqueIndex(std::pmr::memory_resource*, size_t)
 */
TEST(DataframeIndexUnique, Construct)
{
    using namespace lugizmo;

    {
        // default constructed: empty and reports no positions
        DFUniqueIndex<std::string> const index;

        ASSERT_EQ(index.Size(), 0);
        ASSERT_TRUE(index.Empty());
        ASSERT_EQ(index.MaxPosition(), std::optional<std::size_t>());
        ASSERT_TRUE(index.Keys().empty());
        ASSERT_EQ(index.Key(0), std::optional<std::string>());
    }

    {
        // constructed with a memory resource and capacity: empty but usable
        std::array<std::byte, 2048> buffer{};
        std::pmr::monotonic_buffer_resource mem(buffer.data(), buffer.size());

        DFUniqueIndex<std::string> index(&mem, 8);

        ASSERT_EQ(index.Size(), 0);
        ASSERT_TRUE(index.Empty());

        ASSERT_EQ(index.Add("first"), 0);
        ASSERT_EQ(index.Add("second"), 1);
        ASSERT_EQ(index.Size(), 2);
        ASSERT_EQ(index.Position("second"), 1);
    }
}

/**
 *  @brief Adding keys assigns increasing positions; duplicates are rejected.
 *  @see   lugizmo::DFUniqueIndex::Add(T&&)
 */
TEST(DataframeIndexUnique, Add)
{
    using namespace lugizmo;

    DFUniqueIndex<std::string> index;

    {
        // sequential additions get the next free position
        ASSERT_EQ(index.Add("first"), 0);
        ASSERT_EQ(index.Add("second"), 1);
        ASSERT_EQ(index.Add("third"), 2);
        ASSERT_EQ(index.Size(), 3);
    }

    {
        // re-adding an existing key is rejected with nullopt and does not grow
        ASSERT_EQ(index.Add("second"), std::optional<std::size_t>());
        ASSERT_EQ(index.Size(), 3);
    }
}

/**
 *  @brief AddMultiple returns the number of newly inserted keys and skips duplicates.
 *  @see   lugizmo::DFUniqueIndex::AddMultiple(std::span<KeyType const>)
 */
TEST(DataframeIndexUnique, AddMultiple)
{
    using namespace lugizmo;

    DFUniqueIndex<std::string> index;
    auto const first = std::array<std::string, 3>{"first", "second", "third"};

    {
        // all keys are new: every one is inserted
        ASSERT_EQ(index.AddMultiple(first), 3);
        ASSERT_EQ(index.Position("first"), 0);
        ASSERT_EQ(index.Position("second"), 1);
        ASSERT_EQ(index.Position("third"), 2);
    }

    {
        // re-adding the same keys inserts nothing and leaves the size unchanged
        auto const sizeBefore = index.Size();
        ASSERT_EQ(index.AddMultiple(first), 0);
        ASSERT_EQ(index.Size(), sizeBefore);
    }

    {
        // a mix of new and existing keys only adds the new ones (above the small threshold)
        auto const mixed = std::array<std::string, 6>{"first", "a", "b", "second", "c", "d"};
        ASSERT_EQ(index.AddMultiple(mixed), 4);
        ASSERT_EQ(index.Size(), 7);
        ASSERT_EQ(index.Position("a"), 3);
        ASSERT_EQ(index.Position("d"), 6);
    }
}

/**
 *  @brief Has reports membership for stored keys and transparent string literals.
 *  @see   lugizmo::DFUniqueIndex::Has(C const&)
 */
TEST(DataframeIndexUnique, Has)
{
    auto const index = CreateTestStringIndexUnique();

    {
        // present, via string literal and std::string
        ASSERT_TRUE(index.Has("first"));
        ASSERT_TRUE(index.Has(std::string{"third"}));
    }

    {
        // missing key
        ASSERT_FALSE(index.Has("missing"));
    }
}

/**
 *  @brief Key resolves a position back to its key and stays consistent after mutation.
 *  @see   lugizmo::DFUniqueIndex::Key(size_t)
 */
TEST(DataframeIndexUnique, Key)
{
    using namespace lugizmo;

    {
        // valid positions map to their key
        auto const index = CreateTestStringIndexUnique();
        ASSERT_EQ(index.Key(0), "first");
        ASSERT_EQ(index.Key(1), "second");
        ASSERT_EQ(index.Key(2), "third");
    }

    {
        // positions outside the index return nullopt
        auto const index = CreateTestStringIndexUnique();
        ASSERT_EQ(index.Key(3), std::optional<std::string>());
        ASSERT_EQ(index.Key(100), std::optional<std::string>());
    }

    {
        // after dropping the middle key, positions compact and the reverse lookup follows
        auto index = CreateTestStringIndexUnique();
        ASSERT_EQ(index.Drop("second"), 1);

        ASSERT_EQ(index.Key(0), "first");
        ASSERT_EQ(index.Key(1), "third");
        ASSERT_EQ(index.Key(2), std::optional<std::string>());
    }

    {
        // after sorting, the reverse lookup reflects the new order
        DFUniqueIndex<std::string> index;
        index.AddMultiple(std::array<std::string, 3>{"banana", "apple", "cherry"});
        index.Sort();

        ASSERT_EQ(index.Key(0), "apple");
        ASSERT_EQ(index.Key(1), "banana");
        ASSERT_EQ(index.Key(2), "cherry");
    }
}

/**
 *  @brief Position resolves a key to its position, or nullopt when absent.
 *  @see   lugizmo::DFUniqueIndex::Position(C const&)
 */
TEST(DataframeIndexUnique, Position)
{
    auto const index = CreateTestStringIndexUnique();

    {
        // present keys resolve to their position
        ASSERT_EQ(index.Position("first"), 0);
        ASSERT_EQ(index.Position("second"), 1);
        ASSERT_EQ(index.Position("third"), 2);
    }

    {
        // missing key
        ASSERT_EQ(index.Position("missing"), std::optional<std::size_t>());
    }
}

/**
 *  @brief Keys/Position/Size/Empty/MaxPosition expose the stored mapping and counts.
 *  @see   lugizmo::DFUniqueIndex::Keys, Position, Size, Empty, MaxPosition
 */
TEST(DataframeIndexUnique, Access)
{
    using namespace lugizmo;

    auto const index = CreateTestStringIndexUnique();

    {
        // keys are returned in physical order
        auto const keys = index.Keys();
        ASSERT_EQ(keys.size(), 3);
        EXPECT_EQ(keys[0], "first");
        EXPECT_EQ(keys[1], "second");
        EXPECT_EQ(keys[2], "third");
    }

    {
        // positions follow physical order: each key resolves to its slot
        EXPECT_EQ(index.Position("first"), 0);
        EXPECT_EQ(index.Position("second"), 1);
        EXPECT_EQ(index.Position("third"), 2);
    }

    {
        // size and emptiness
        EXPECT_EQ(index.Size(), 3);
        EXPECT_FALSE(index.Empty());

        DFUniqueIndex<std::string> const empty;
        EXPECT_EQ(empty.Size(), 0);
        EXPECT_TRUE(empty.Empty());
    }

    {
        // max position: highest assigned position, nullopt while empty
        EXPECT_EQ(index.MaxPosition(), 2);

        DFUniqueIndex<std::string> const empty;
        EXPECT_EQ(empty.MaxPosition(), std::optional<std::size_t>());
    }
}

/**
 *  @brief Drop removes a key, returns its old position, and compacts the remaining ones.
 *  @see   lugizmo::DFUniqueIndex::Drop(T const&)
 */
TEST(DataframeIndexUnique, Drop)
{
    {
        // dropping front then back returns the dropped positions and updates MaxPosition
        auto index = CreateTestStringIndexUnique();

        ASSERT_EQ(index.Drop("first"), 0);
        ASSERT_EQ(index.Size(), 2);
        ASSERT_EQ(index.MaxPosition(), 1);

        ASSERT_EQ(index.Drop("third"), 1);
        ASSERT_EQ(index.Size(), 1);
        ASSERT_EQ(index.MaxPosition(), 0);
    }

    {
        // dropping the middle key compacts the trailing positions
        auto index = CreateTestStringIndexUnique();

        ASSERT_EQ(index.Drop("second"), 1);
        ASSERT_EQ(index.Size(), 2);
        ASSERT_EQ(index.Position("first"), 0);
        ASSERT_EQ(index.Position("third"), 1);
    }

    {
        // dropping every key empties the index
        auto index = CreateTestStringIndexUnique();

        ASSERT_EQ(index.Drop("first"), 0);
        ASSERT_EQ(index.Drop("second"), 0);
        ASSERT_EQ(index.Drop("third"), 0);
        ASSERT_TRUE(index.Empty());
        ASSERT_EQ(index.MaxPosition(), std::optional<std::size_t>());
    }

    {
        // dropping a missing key is a no-op reported with nullopt
        auto index = CreateTestStringIndexUnique();
        ASSERT_EQ(index.Drop("missing"), std::optional<std::size_t>());
        ASSERT_EQ(index.Size(), 3);
    }
}

/**
 *  @brief Sort orders keys, rebuilds positions, and returns the new-to-old permutation.
 *  @see   lugizmo::DFUniqueIndex::Sort(Compare comp = {})
 */
TEST(DataframeIndexUnique, Sort)
{
    using namespace lugizmo;

    {
        // default comparator sorts ascending; old layout [banana(0), apple(1), cherry(2)]
        DFUniqueIndex<std::string> index;
        index.AddMultiple(std::array<std::string, 3>{"banana", "apple", "cherry"});

        auto const permutation = index.Sort();

        ASSERT_EQ(permutation.size(), 3);
        EXPECT_EQ(permutation[0], 1);
        EXPECT_EQ(permutation[1], 0);
        EXPECT_EQ(permutation[2], 2);

        auto const keys = index.Keys();
        ASSERT_EQ(keys.size(), 3);
        EXPECT_EQ(keys[0], "apple");
        EXPECT_EQ(keys[1], "banana");
        EXPECT_EQ(keys[2], "cherry");

        // positions are renumbered to physical order
        EXPECT_EQ(index.Position("apple"), 0);
        EXPECT_EQ(index.Position("banana"), 1);
        EXPECT_EQ(index.Position("cherry"), 2);
        EXPECT_EQ(index.MaxPosition(), 2);
    }

    {
        // custom comparator sorts descending
        DFUniqueIndex<std::string> index;
        index.AddMultiple(std::array<std::string, 3>{"banana", "apple", "cherry"});

        index.Sort(std::greater<std::string>{});

        auto const keys = index.Keys();
        ASSERT_EQ(keys.size(), 3);
        EXPECT_EQ(keys[0], "cherry");
        EXPECT_EQ(keys[1], "banana");
        EXPECT_EQ(keys[2], "apple");
        EXPECT_EQ(index.Position("apple"), 2);
    }
}

/**
 *  @brief Resource-aware copy construction, copy assignment, and moves preserve the mapping independently.
 *  @see   lugizmo::DFUniqueIndex copy/move constructors and assignment operators
 *         lugizmo::DFUniqueIndex::DFUniqueIndex(DFUniqueIndex const&, std::pmr::memory_resource*)
 */
TEST(DataframeIndexUnique, Tors)
{
    using namespace lugizmo;

    static_assert(!std::is_copy_constructible_v<DFUniqueIndex<std::string>>);

    {
        // copy assignment overwrites the destination
        auto const index = CreateTestStringIndexUnique();

        DFUniqueIndex<std::string> copy;
        copy.Add("temporary");
        copy = index;

        EXPECT_EQ(copy.Size(), 3);
        EXPECT_EQ(copy.Key(0), "first");
        EXPECT_FALSE(copy.Has("temporary"));
    }

    {
        // move construction transfers the mapping
        auto index = CreateTestStringIndexUnique();
        auto moved = DFUniqueIndex<std::string>(std::move(index));

        EXPECT_EQ(moved.Size(), 3);
        EXPECT_EQ(moved.Key(0), "first");
        EXPECT_EQ(moved.Position("third"), 2);
    }

    {
        // move assignment transfers into an existing index
        auto index = CreateTestStringIndexUnique();

        DFUniqueIndex<std::string> moved;
        moved.Add("temporary");
        moved = std::move(index);

        EXPECT_EQ(moved.Size(), 3);
        EXPECT_EQ(moved.Key(0), "first");
        EXPECT_FALSE(moved.Has("temporary"));
    }

    {
        // copy construction with a dedicated memory resource preserves the mapping
        auto const index = CreateTestStringIndexUnique();

        std::array<std::byte, 2048> buffer{};
        std::pmr::monotonic_buffer_resource mem(buffer.data(), buffer.size());
        auto const copy = DFUniqueIndex<std::string>(index, &mem);

        EXPECT_EQ(copy.Size(), index.Size());
        EXPECT_EQ(copy.Keys()[0], "first");
        EXPECT_EQ(copy.Keys()[2], "third");
        EXPECT_EQ(copy.Position("second"), 1);
    }
}
