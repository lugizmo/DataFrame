// Filename: RM_DataframeUserTypesTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test that the dataframe handles user-provided value/key types correctly. Currently covers a
// non-trivial value type (a C++-like move/copy type owning a heap resource, rule of five):
// adds/upserts copy/move as expected, every stored element owns a live resource, and teardown
// releases everything (no leaks).
//
// TODO (later): special key/value types supported by the library -- e.g. std::string
// (transparent hash / equality lookup), std::chrono (range keys, formatting), etc.
//
// ✅ MoveCopyAndDestroy
// - element lifetime across adding / upsert / frame destruction
//
// ✅ StringKeys
// - std::string field/record keys: transparent hash / equality lookup
//   (std::string, std::string_view, const char*)
//

#include "gtest/gtest.h"

#include <array>
#include <string>
#include <string_view>

#include "lugizmo/DataFrame.h"

namespace {

    // A C++-like value type: owns a heap resource with full copy and move semantics and counts
    // each operation so the dataframe's element handling can be observed.
    struct MoveCopyType
    {
        int* payload = nullptr;

        static inline int defaultCtor = 0;
        static inline int valueCtor   = 0;
        static inline int copyCtor    = 0;
        static inline int moveCtor    = 0;
        static inline int copyAssign  = 0;
        static inline int moveAssign  = 0;
        static inline int dtor        = 0;
        static inline int alive       = 0; // net owners (payload != nullptr)

        MoveCopyType() noexcept { ++defaultCtor; }

        explicit MoveCopyType(int const value) : payload(new int(value))
        {
            ++valueCtor;
            ++alive;
        }

        MoveCopyType(MoveCopyType const& other) : payload(other.payload != nullptr ? new int(*other.payload) : nullptr)
        {
            ++copyCtor;
            if(payload != nullptr) ++alive;
        }

        MoveCopyType(MoveCopyType&& other) noexcept : payload(other.payload)
        {
            ++moveCtor;
            other.payload = nullptr;
        }

        auto operator=(MoveCopyType const& other) -> MoveCopyType&
        {
            ++copyAssign;
            if(this == &other) return *this;

            Release();
            if(other.payload != nullptr)
            {
                payload = new int(*other.payload);
                ++alive;
            }
            return *this;
        }

        auto operator=(MoveCopyType&& other) noexcept -> MoveCopyType&
        {
            ++moveAssign;
            if(this == &other) return *this;

            Release();
            payload       = other.payload;
            other.payload = nullptr;
            return *this;
        }

        ~MoveCopyType()
        {
            ++dtor;
            Release();
        }

        [[nodiscard]] auto HasValue() const noexcept -> bool { return payload != nullptr; }
        [[nodiscard]] auto Value() const noexcept -> int { return payload != nullptr ? *payload : -1; }

        static void ResetCounters() noexcept
        {
            defaultCtor = valueCtor = copyCtor = moveCtor = copyAssign = moveAssign = dtor = alive = 0;
        }

    private:

        void Release() noexcept
        {
            if(payload == nullptr) return;
            delete payload;
            payload = nullptr;
            --alive;
        }
    };

} // namespace

/**
 *  @brief The dataframe copies/moves/destroys a rule-of-five value type without leaking.
 *  @see   lugizmo::DataFrame element lifetime (Add*, UpsertValue, destructor)
 */
TEST(RM_DataframeUserTypes, MoveCopyAndDestroy)
{
    using DF = lugizmo::DataFrame<MoveCopyType, int, int>;
    MoveCopyType::ResetCounters();

    {
        MoveCopyType const fieldDefault{10};
        MoveCopyType const recordDefault{20};
        MoveCopyType const replacement{77};

        {
            auto df = DF{};

            {
                // adding fields/records copies the default value into each new cell
                ASSERT_TRUE(df.AddField(1, fieldDefault));
                ASSERT_TRUE(df.AddRecord(10, recordDefault));
                ASSERT_TRUE(df.AddRecord(11, recordDefault));
                ASSERT_TRUE(df.AddRecord(12, recordDefault));
                ASSERT_TRUE(df.AddField(2, fieldDefault));

                EXPECT_EQ(df.FieldSize(), 2);
                EXPECT_EQ(df.RecordSize(), 3);
                EXPECT_EQ(df.Size(), 6);
                EXPECT_GT(MoveCopyType::copyCtor, 0); // defaults copied into the cells
            }

            {
                // upserting an lvalue copy; upserting an rvalue move
                df.UpsertValue(2, 11, replacement);

                auto const moveAssignBefore = MoveCopyType::moveAssign;
                df.UpsertValue(2, 11, MoveCopyType{88});
                EXPECT_GT(MoveCopyType::moveAssign, moveAssignBefore);

                auto const value = df.GetValue(2, 11);
                ASSERT_NE(value, nullptr);
                EXPECT_TRUE(value->HasValue());
                EXPECT_EQ(value->Value(), 88);
            }

            {
                // every stored element owns a live resource (no moved-from holes)
                for(auto const& entry : df.Values()) EXPECT_TRUE(entry.HasValue());

                EXPECT_GT(MoveCopyType::moveCtor, 0); // values moved during reallocation
            }
        }

        // after the frame is destroyed, only the three stack locals remain alive
        EXPECT_EQ(MoveCopyType::alive, 3);
    }

    // all owners released -> no leaks
    EXPECT_EQ(MoveCopyType::alive, 0);
}

/**
 *  @brief std::string keys support transparent lookup (std::string / string_view / const char*),
 *         so heterogeneous keys resolve without constructing a temporary std::string.
 *  @see   lugizmo::TransparentHash / TransparentEqual
 */
TEST(RM_DataframeUserTypes, StringKeys)
{
    using namespace lugizmo;
    using namespace std::string_view_literals;

    auto df = DataFrame<int, std::string, std::string>();
    ASSERT_EQ(df.AddFields(std::array<std::string, 2>{"alpha", "beta"}), 2);
    ASSERT_EQ(df.AddRecords(std::array<std::string, 2>{"row0", "row1"}), 2);
    ASSERT_TRUE(df.AssignValue("alpha", "row0", 1));
    ASSERT_TRUE(df.AssignValue("beta", "row1", 2));

    {
        // membership via std::string, std::string_view and const char*
        EXPECT_TRUE(df.HasField(std::string{"alpha"}));
        EXPECT_TRUE(df.HasField("alpha"sv));
        EXPECT_TRUE(df.HasField("alpha"));
        EXPECT_FALSE(df.HasField("gamma"sv));

        EXPECT_TRUE(df.HasRecord("row1"sv));
        EXPECT_TRUE(df.HasRecord("row1"));
        EXPECT_FALSE(df.HasRecord("row9"sv));
    }

    {
        // value access via heterogeneous string keys resolves the same cell
        EXPECT_EQ((df["alpha"sv, "row0"sv]), 1);

        auto const val = df.GetValue("beta"sv, "row1"sv);
        ASSERT_NE(val, nullptr);
        EXPECT_EQ(*val, 2);

        // a non-member heterogeneous key resolves to nothing
        EXPECT_EQ(df.GetValue("alpha"sv, "row9"sv), nullptr);
    }
}
