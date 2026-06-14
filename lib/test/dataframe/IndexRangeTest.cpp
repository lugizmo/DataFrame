// Filename: IndexRangeTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the DFRangeIndex computed (strided) range index.
// The following functions are tested here (with names of tests):
//
// ✅ DefaultIndex      - DFRangeIndex() / Empty / Size / LowerBound / UpperBound
// ✅ InvertedBounds    - DFRangeIndex(lower > upper) (normalizes to empty)
// ✅ Accessors         - Size / LowerBound / UpperBound
// ✅ InBound           - InBound(key)
// ✅ SetBounds         - SetLowerBound / SetUpperBound (element-count deltas)
// ✅ CopyConstructor   - DFRangeIndex(DFRangeIndex const&)
// ✅ Strided           - Step / Size / Position / Keys (step != 1)
// ✅ HasStrided        - Has(key) on a strided grid
// ✅ Bijection         - Key(pos) <-> Position(key) round trip
// ✅ ChronoHours       - DFRangeIndex<std::chrono::hours>
// ✅ CustomType        - DFRangeIndex over a custom affine key (DFRngKey)
//

#include "gtest/gtest.h"

#include <array>
#include <chrono>
#include <compare>
#include <cstddef>
#include <optional>
#include <string>

#include "lugizmo/dataframe/IndexRange.h"

namespace {

    /// @brief A minimal custom key type, affine over std::ptrdiff_t, satisfying DFRangeKey.
    struct Tick
    {
        int value = 0;

        constexpr auto operator<=>(Tick const&) const = default;

        constexpr friend auto operator-(Tick a, Tick b) noexcept -> std::ptrdiff_t { return a.value - b.value; }
        constexpr friend auto operator+(Tick t, std::ptrdiff_t n) noexcept -> Tick { return Tick{static_cast<int>(t.value + n)}; }
    };

    static_assert(lugizmo::DFRngKey<Tick>, "Tick should satisfy DFRangeKey.");
    static_assert(lugizmo::DFRngKey<std::chrono::hours>, "std::chrono::hours should satisfy DFRangeKey.");

} // namespace

TEST(DataframeIndexRange, DefaultIndex)
{
    constexpr auto index = lugizmo::DFRangeIndex<int>();

    ASSERT_TRUE(index.Empty());
    ASSERT_TRUE(index.Size() == 0);
    ASSERT_EQ(index.LowerBound(), 0);
    ASSERT_EQ(index.UpperBound(), 0);
}

TEST(DataframeIndexRange, InvertedBounds)
{
    constexpr auto index = lugizmo::DFRangeIndex(10, 5);
    ASSERT_TRUE(index.Empty());
    ASSERT_TRUE(index.Size() == 0);
    ASSERT_EQ(index.LowerBound(), 0);
    ASSERT_EQ(index.UpperBound(), 0);
}

TEST(DataframeIndexRange, Accessors)
{
    constexpr auto index = lugizmo::DFRangeIndex(0, 10);
    ASSERT_FALSE(index.Empty());
    ASSERT_EQ(index.Size(), 10);
    ASSERT_EQ(index.LowerBound(), 0);
    ASSERT_EQ(index.UpperBound(), 10);
}

TEST(DataframeIndexRange, InBound)
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

TEST(DataframeIndexRange, SetBounds)
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

TEST(DataframeIndexRange, CopyConstructor)
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

TEST(DataframeIndexRange, Strided)
{
    constexpr auto index = lugizmo::DFRangeIndex(0, 10, 2); // 0, 2, 4, 6, 8

    ASSERT_EQ(index.Step(), 2);
    ASSERT_EQ(index.Size(), 5);
    ASSERT_EQ(index.UpperBoundPosition(), 5);

    // on-grid keys resolve to a position; off-grid and out-of-range keys do not
    ASSERT_EQ(index.Position(0), 0);
    ASSERT_EQ(index.Position(4), 2);
    ASSERT_EQ(index.Position(8), 4);
    ASSERT_EQ(index.Position(3), std::nullopt);  // not on the step grid
    ASSERT_EQ(index.Position(10), std::nullopt); // out of range

    // iteration yields the strided keys
    constexpr auto expected = std::array{0, 2, 4, 6, 8};
    size_t i = 0;
    for (auto const key : index.Keys()) { ASSERT_EQ(key, expected.at(i++)); }
    ASSERT_EQ(i, 5);

    // an unaligned upper still counts the last element (ceil)
    constexpr auto odd = lugizmo::DFRangeIndex(0, 9, 3); // 0, 3, 6
    ASSERT_EQ(odd.Size(), 3);
    ASSERT_EQ(odd.Position(6), 2);
    ASSERT_EQ(odd.Position(9), std::nullopt);
}

TEST(DataframeIndexRange, HasStrided)
{
    constexpr auto index = lugizmo::DFRangeIndex(0, 10, 2); // 0, 2, 4, 6, 8

    // on-grid keys are members; in-bounds but off-grid keys are not
    ASSERT_TRUE(index.Has(0));
    ASSERT_TRUE(index.Has(8));
    ASSERT_FALSE(index.Has(3));   // in [0,10) but off the step grid
    ASSERT_FALSE(index.Has(10));  // out of range

    // exactly Size() keys in [lower, upper) are members
    size_t members = 0;
    for (int key = index.LowerBound(); key < index.UpperBound(); ++key)
    {
        if (index.Has(key)) ++members;
    }
    ASSERT_EQ(members, index.Size());

    // contiguous range: every in-bounds key is a member
    constexpr auto contiguous = lugizmo::DFRangeIndex(0, 5);
    ASSERT_TRUE(contiguous.Has(3));
    ASSERT_FALSE(contiguous.Has(5));
}

TEST(DataframeIndexRange, Bijection)
{
    constexpr auto index = lugizmo::DFRangeIndex(-5, 7); // step 1

    // position -> key -> position round trips for every position
    for (size_t pos = 0; pos < index.Size(); ++pos)
    {
        auto const key = index.Key(pos);
        ASSERT_TRUE(key.has_value());
        ASSERT_EQ(index.Position(*key), pos);
    }

    ASSERT_EQ(index.Key(0), -5);
    ASSERT_EQ(index.Key(11), 6);
    ASSERT_EQ(index.Key(12), std::nullopt); // out of range

    // inverse direction for a strided range
    constexpr auto strided = lugizmo::DFRangeIndex(0, 10, 2);
    ASSERT_EQ(strided.Key(0), 0);
    ASSERT_EQ(strided.Key(3), 6);
    ASSERT_EQ(strided.Key(5), std::nullopt);
}

TEST(DataframeIndexRange, ChronoHours)
{
    using namespace std::chrono;

    auto const index = lugizmo::DFRangeIndex(hours{0}, hours{24}, hours{6}); // 0h, 6h, 12h, 18h

    ASSERT_EQ(index.Size(), 4);
    ASSERT_EQ(index.Step(), hours{6});
    ASSERT_EQ(index.LowerBound(), hours{0});
    ASSERT_EQ(index.UpperBound(), hours{24});

    // forward: key -> position
    ASSERT_EQ(index.Position(hours{0}), 0);
    ASSERT_EQ(index.Position(hours{12}), 2);
    ASSERT_EQ(index.Position(hours{18}), 3);
    ASSERT_EQ(index.Position(hours{5}), std::nullopt);  // off the 6h grid
    ASSERT_EQ(index.Position(hours{24}), std::nullopt); // out of range

    // inverse: position -> key
    ASSERT_EQ(index.Key(0), hours{0});
    ASSERT_EQ(index.Key(3), hours{18});
    ASSERT_EQ(index.Key(4), std::nullopt);

    // membership respects the grid
    ASSERT_TRUE(index.Has(hours{6}));
    ASSERT_FALSE(index.Has(hours{7}));
    ASSERT_FALSE(index.Has(hours{24}));

    // iteration yields the strided time spans
    constexpr auto expected = std::array{hours{0}, hours{6}, hours{12}, hours{18}};
    size_t i = 0;

    for(auto const span : index.Keys()) { ASSERT_EQ(span, expected.at(i++)); }
    ASSERT_EQ(i, 4);
}

TEST(DataframeIndexRange, CustomType)
{
    auto const index = lugizmo::DFRangeIndex(Tick{0}, Tick{10}, std::ptrdiff_t{2}); // 0, 2, 4, 6, 8

    ASSERT_EQ(index.Size(), 5);
    ASSERT_EQ(index.Step(), 2);
    ASSERT_EQ(index.LowerBound(), Tick{0});
    ASSERT_EQ(index.UpperBound(), Tick{10});

    ASSERT_EQ(index.Position(Tick{4}), 2);
    ASSERT_EQ(index.Position(Tick{3}), std::nullopt);  // off grid
    ASSERT_EQ(index.Position(Tick{10}), std::nullopt); // out of range

    ASSERT_EQ(index.Key(0), Tick{0});
    ASSERT_EQ(index.Key(4), Tick{8});
    ASSERT_EQ(index.Key(5), std::nullopt);

    ASSERT_TRUE(index.Has(Tick{6}));
    ASSERT_FALSE(index.Has(Tick{7}));
}
