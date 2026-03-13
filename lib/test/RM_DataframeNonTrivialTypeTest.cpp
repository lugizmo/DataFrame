// Filename: RM_DataframeNonTrivialTypeTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

namespace {

    struct MoveTracked
    {
        int* payload = nullptr;

        static inline int ctorCount       = 0;
        static inline int copyCtorCount   = 0;
        static inline int moveCtorCount   = 0;
        static inline int copyAssignCount = 0;
        static inline int moveAssignCount = 0;
        static inline int dtorNullCount   = 0;
        static inline int dtorOwnedCount  = 0;
        static inline int aliveOwners     = 0;

        explicit MoveTracked(int const value = 0) : payload(new int(value))
        {
            ++ctorCount;
            ++aliveOwners;
        }

        MoveTracked(MoveTracked const& other) : payload(other.payload ? new int(*other.payload) : nullptr)
        {
            ++copyCtorCount;
            if(payload != nullptr) ++aliveOwners;
        }

        MoveTracked(MoveTracked&& other) noexcept : payload(other.payload)
        {
            ++moveCtorCount;
            other.payload = nullptr;
        }

        auto operator=(MoveTracked const& other) noexcept -> MoveTracked&
        {
            ++copyAssignCount;
            if(this == &other) return *this;

            if(other.payload == nullptr)
            {
                ReleaseOwner();
                return *this;
            }

            if(payload == nullptr)
            {
                payload = new int(*other.payload);
                ++aliveOwners;
                return *this;
            }

            *payload = *other.payload;
            return *this;
        }

        auto operator=(MoveTracked&& other) noexcept -> MoveTracked&
        {
            ++moveAssignCount;
            if(this == &other) return *this;

            ReleaseOwner();
            payload       = other.payload;
            other.payload = nullptr;
            return *this;
        }

        ~MoveTracked()
        {
            if(payload == nullptr)
            {
                ++dtorNullCount;
                return;
            }

            ++dtorOwnedCount;
            delete payload;
            payload = nullptr;
            --aliveOwners;
        }

        [[nodiscard]] auto HasValue() const noexcept -> bool { return payload != nullptr; }
        [[nodiscard]] auto Value() const noexcept -> int { return payload != nullptr ? *payload : -1; }

        static void Reset()
        {
            ctorCount       = 0;
            copyCtorCount   = 0;
            moveCtorCount   = 0;
            copyAssignCount = 0;
            moveAssignCount = 0;
            dtorNullCount   = 0;
            dtorOwnedCount  = 0;
            aliveOwners     = 0;
        }

    private:

        void ReleaseOwner() noexcept
        {
            if(payload == nullptr) return;
            delete payload;
            payload = nullptr;
            --aliveOwners;
        }
    };
}

TEST(lugizmo_dataframe_non_trivial_row_major, moves_and_destructors)
{
    using DF = lugizmo::DataFrame<MoveTracked, int, int>;
    MoveTracked::Reset();

    int ownedElementsInDf = 0;

    {
        MoveTracked fieldDefault{10};
        MoveTracked rowDefault{20};
        MoveTracked replacement{77};

        {
            auto df = DF{};
            EXPECT_TRUE(df.AddField(1, fieldDefault));
            EXPECT_TRUE(df.AddRecord(10, rowDefault));
            EXPECT_TRUE(df.AddRecord(11, rowDefault));
            EXPECT_TRUE(df.AddRecord(12, rowDefault));
            EXPECT_TRUE(df.AddField(2, fieldDefault));

            df.UpsertValue(2, 11, replacement);
            auto const moveAssignBefore = MoveTracked::moveAssignCount;
            df.UpsertValue(2, 11, MoveTracked{88});

            EXPECT_EQ(df.RecordSize(), 3);
            EXPECT_EQ(df.FieldSize(), 2);
            EXPECT_EQ(df.Size(), 6);

            auto value = df.GetValue(2, 11);
            ASSERT_TRUE(value.HasValue());
            EXPECT_TRUE(value->HasValue());
            EXPECT_EQ(value->Value(), 88);
            EXPECT_GT(MoveTracked::moveAssignCount, moveAssignBefore);

            for(auto const& entry : df.Values()) EXPECT_TRUE(entry.HasValue());

            EXPECT_GT(MoveTracked::moveCtorCount, 0);
            EXPECT_GT(MoveTracked::dtorNullCount, 0);
            EXPECT_EQ(MoveTracked::dtorOwnedCount, 0);

            ownedElementsInDf = static_cast<int>(df.Size());
        }

        EXPECT_EQ(MoveTracked::dtorOwnedCount, ownedElementsInDf);
        EXPECT_EQ(MoveTracked::aliveOwners, 3);
    }

    EXPECT_EQ(MoveTracked::aliveOwners, 0);
}
