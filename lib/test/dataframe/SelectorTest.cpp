// Filename: SelectorTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test DataFrame selector adaptor storage and lifetime behavior.
//
// ✅ SelectFieldBorrowed        - lvalue field keys are borrowed without copying
// ✅ SelectFieldIndexedOwned    - temporary field keys are owned
// ✅ SelectRecordBorrowedCopy   - copied borrowing selectors retain the reference
// ✅ SelectRecordIndexedOwned   - owned selectors are independently copyable
// ✅ ConstRvalueOwned           - const rvalues are copied into owned storage
//

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include "lugizmo/dataframe/Selector.h"

namespace {

    struct LargeKey
    {
        inline static std::size_t copies = 0;
        inline static std::size_t moves  = 0;

        std::array<int, 64> values{};

        explicit LargeKey(int const value)
        {
            values.fill(value);
        }

        LargeKey(LargeKey const& other) : values(other.values)
        {
            ++copies;
        }

        LargeKey(LargeKey&& other) noexcept : values(std::move(other.values))
        {
            ++moves;
        }

        auto operator=(LargeKey const& other) -> LargeKey&
        {
            values = other.values;
            ++copies;
            return *this;
        }

        auto operator=(LargeKey&& other) noexcept -> LargeKey&
        {
            values = std::move(other.values);
            ++moves;
            return *this;
        }

        static void ResetCounts() noexcept
        {
            copies = 0;
            moves  = 0;
        }
    };

} // namespace

/**
 * @brief SelectField borrows an lvalue key without copying or moving it.
 * @see   lugizmo::SelectField
 */
TEST(Selector, SelectFieldBorrowed)
{
    using namespace lugizmo;

    auto key = LargeKey(7);
    LargeKey::ResetCounts();
    auto selector = SelectField(key);

    static_assert(std::is_same_v<decltype(selector), SelectField<LargeKey, std::reference_wrapper<LargeKey const>>>);
    static_assert(sizeof(selector) == sizeof(std::reference_wrapper<LargeKey const>));
    EXPECT_EQ(&selector.Value(), &key);
    EXPECT_EQ(LargeKey::copies, 0);
    EXPECT_EQ(LargeKey::moves, 0);
}

/**
 * @brief SelectFieldIndexed owns a temporary key moved into the selector.
 * @see   lugizmo::SelectFieldIndexed
 */
TEST(Selector, SelectFieldIndexedOwned)
{
    using namespace lugizmo;

    LargeKey::ResetCounts();
    auto selector = SelectFieldIndexed(LargeKey(8));

    EXPECT_EQ(selector.Value().values.front(), 8);
    EXPECT_EQ(selector.Value().values.back(), 8);
    EXPECT_EQ(LargeKey::copies, 0);
    EXPECT_GT(LargeKey::moves, 0);
}

/**
 * @brief Copying a borrowing SelectRecord keeps borrowing the original key.
 * @see   lugizmo::SelectRecord
 */
TEST(Selector, SelectRecordBorrowedCopy)
{
    using namespace lugizmo;

    auto key = LargeKey(9);
    auto selector = SelectRecord(key);
    LargeKey::ResetCounts();
    auto copy = selector;

    EXPECT_EQ(&copy.Value(), &key);
    EXPECT_EQ(LargeKey::copies, 0);
    EXPECT_EQ(LargeKey::moves, 0);
}

/**
 * @brief Copying an owning SelectRecordIndexed gives the copy its own key.
 * @see   lugizmo::SelectRecordIndexed
 */
TEST(Selector, SelectRecordIndexedOwned)
{
    using namespace lugizmo;

    auto selector = SelectRecordIndexed(LargeKey(10));
    LargeKey::ResetCounts();
    auto copy = selector;

    EXPECT_NE(&copy.Value(), &selector.Value());
    EXPECT_EQ(copy.Value().values.front(), 10);
    EXPECT_EQ(LargeKey::copies, 1);
}

/**
 * @brief A const rvalue is copied into owned storage instead of being borrowed.
 * @see   lugizmo::SelectField
 */
TEST(Selector, ConstRvalueOwned)
{
    using namespace lugizmo;

    auto const key = LargeKey(11);
    LargeKey::ResetCounts();
    auto selector = SelectField(std::move(key));

    static_assert(std::is_same_v<decltype(selector), SelectField<LargeKey>>);
    EXPECT_NE(&selector.Value(), &key);
    EXPECT_EQ(selector.Value().values.front(), 11);
    EXPECT_EQ(LargeKey::copies, 1);
}
