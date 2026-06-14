// Filename: RM_DataframeTorsTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the dataframe copy/move constructors, assignment operators, and destructor. The lifecycle
// is index-agnostic, so it is exercised across every index configuration via TYPED_TEST; the two
// memory-release tests (using a counting resource) are plain since the free path is index-neutral.
// The following functions are tested here (with names of tests):
//
// ✅ MoveConstruction
// - DataFrame(DataFrame&& other) noexcept
//
// ✅ MoveAssignment / MoveAssignmentReleasesTargetMemory
// - operator=(DataFrame&& other) noexcept -> DataFrame&
//
// ✅ CopyConstruction
// - DataFrame(DataFrame const&) noexcept
//
// ✅ CopyAssignment / SelfCopyAssignment
// - operator=(DataFrame const&) noexcept -> DataFrame&
//
// ✅ DestructorReleasesMemory
// - ~DataFrame() noexcept
//

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <memory>
#include <memory_resource>

#include "RM_DataframeTestConfigs.h"

using namespace lugizmo;
using namespace lugizmo::test;

namespace {

    // Memory resource that tracks outstanding bytes and alloc/free counts, forwarding the work to
    // the new/delete resource. Lets the destructor / move-assign free path be asserted deterministically.
    class CountingResource final : public std::pmr::memory_resource
    {
    public:
        [[nodiscard]] auto Outstanding() const noexcept -> std::size_t { return outstanding; }
        [[nodiscard]] auto AllocCount()  const noexcept -> std::size_t { return allocCount; }
        [[nodiscard]] auto FreeCount()   const noexcept -> std::size_t { return freeCount; }

    private:
        auto do_allocate(std::size_t const bytes, std::size_t const align) -> void* override
        {
            ++allocCount;
            outstanding += bytes;
            return std::pmr::new_delete_resource()->allocate(bytes, align);
        }

        void do_deallocate(void* const ptr, std::size_t const bytes, std::size_t const align) override
        {
            ++freeCount;
            outstanding -= bytes;
            std::pmr::new_delete_resource()->deallocate(ptr, bytes, align);
        }

        [[nodiscard]] auto do_is_equal(memory_resource const& other) const noexcept -> bool override { return this == &other; }

        std::size_t outstanding = 0;
        std::size_t allocCount  = 0;
        std::size_t freeCount   = 0;
    };

    // Builds a populated frame whose cell (field f, record r) holds `r * FLD_COUNT + f`.
    template<typename Cfg>
    auto BuildFilled() -> typename Cfg::DF
    {
        auto df = Cfg::Build();
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
            {
                df.AssignValue(Cfg::FieldKey(f), Cfg::RecordKey(r), static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
        return df;
    }

    // Asserts every cell of a BuildFilled-shaped frame holds its expected value.
    template<typename Cfg, typename DF>
    void ExpectFilled(DF const& df)
    {
        for (std::size_t f = 0; f < Cfg::FLD_COUNT; ++f)
        {
            for (std::size_t r = 0; r < Cfg::REC_COUNT; ++r)
            {
                auto const val = df.GetValue(Cfg::FieldKey(f), Cfg::RecordKey(r));
                ASSERT_TRUE(val.HasValue());
                EXPECT_EQ(*val, static_cast<int>(r * Cfg::FLD_COUNT + f));
            }
        }
    }

    // Builds a populated 3x3 int frame on the given resource (for the memory-release tests).
    auto BuildOnResource(DataFrame<int, int, int>::MemRsc const& res) -> DataFrame<int, int, int>
    {
        auto df = DataFrame<int, int, int>{0, res};
        df.AddFields(std::array{0, 1, 2});
        df.AddRecords(std::array{0, 1, 2});
        for (int f = 0; f < 3; ++f)
        {
            for (int r = 0; r < 3; ++r) df.AssignValue(f, r, r * 3 + f);
        }
        return df;
    }

} // namespace

// ======= Lifecycle (index-agnostic) =============================================================

template<typename>
class RM_DataframeTors: public testing::Test // NOLINT(readability-identifier-naming)
{
};

TYPED_TEST_SUITE(RM_DataframeTors, IndexConfigs);

/**
 *  @brief Moving a dataframe transfers ownership and leaves the source empty.
 *  @see   lugizmo::DataFrame::DataFrame(DataFrame&& other) noexcept
 */
TYPED_TEST(RM_DataframeTors, MoveConstruction)
{
    using Cfg = TypeParam;
    auto df   = BuildFilled<Cfg>();

    auto const moved = std::move(df);

    {
        // moved-from frame is empty and detached from storage
        EXPECT_EQ(df.FieldSize(), 0u);  // NOLINT(bugprone-use-after-move) intentional: validating moved-from state
        EXPECT_EQ(df.RecordSize(), 0u);
        EXPECT_EQ(df.Values().size(), 0u);
        EXPECT_EQ(df.Data(), nullptr);
    }

    {
        // moved-to frame owns the data and keeps all content
        ASSERT_NE(moved.Data(), nullptr);
        EXPECT_EQ(moved.FieldSize(), Cfg::FLD_COUNT);
        EXPECT_EQ(moved.RecordSize(), Cfg::REC_COUNT);
        ExpectFilled<Cfg>(moved);
    }
}

/**
 *  @brief Move-assigning transfers ownership into the target and empties the source.
 *  @see   lugizmo::DataFrame::operator=(DataFrame&& other) noexcept -> DataFrame&
 */
TYPED_TEST(RM_DataframeTors, MoveAssignment)
{
    using Cfg = TypeParam;

    // move into a populated target to exercise the free-then-take path
    auto src = BuildFilled<Cfg>();
    auto dst = BuildFilled<Cfg>();

    dst = std::move(src);

    EXPECT_EQ(src.Data(), nullptr);  // NOLINT(bugprone-use-after-move) intentional
    EXPECT_EQ(src.FieldSize(), 0u);
    EXPECT_EQ(src.RecordSize(), 0u);

    ASSERT_NE(dst.Data(), nullptr);
    EXPECT_EQ(dst.FieldSize(), Cfg::FLD_COUNT);
    EXPECT_EQ(dst.RecordSize(), Cfg::REC_COUNT);
    ExpectFilled<Cfg>(dst);
}

/**
 *  @brief Copying yields an independent deep copy: same content, separate storage.
 *  @see   lugizmo::DataFrame::DataFrame(DataFrame const&) noexcept
 */
TYPED_TEST(RM_DataframeTors, CopyConstruction)
{
    using Cfg = TypeParam;
    auto orig = BuildFilled<Cfg>();
    auto copy = orig;

    {
        // independent storage, identical shape and content
        ASSERT_NE(copy.Data(), nullptr);
        EXPECT_NE(orig.Data(), copy.Data());
        EXPECT_EQ(copy.FieldSize(), Cfg::FLD_COUNT);
        EXPECT_EQ(copy.RecordSize(), Cfg::REC_COUNT);
        ExpectFilled<Cfg>(copy);
    }

    {
        // mutating the copy must not affect the original
        copy[Cfg::FieldKey(0), Cfg::RecordKey(0)] = 9999;
        EXPECT_EQ((copy[Cfg::FieldKey(0), Cfg::RecordKey(0)]), 9999);

        auto const origVal = orig.GetValue(Cfg::FieldKey(0), Cfg::RecordKey(0));
        ASSERT_TRUE(origVal.HasValue());
        EXPECT_EQ(*origVal, 0);
    }
}

/**
 *  @brief Copy-assigning produces an independent deep copy of the source.
 *  @see   lugizmo::DataFrame::operator=(DataFrame const&) noexcept -> DataFrame&
 */
TYPED_TEST(RM_DataframeTors, CopyAssignment)
{
    using Cfg = TypeParam;
    auto orig = BuildFilled<Cfg>();
    typename Cfg::DF dst;

    dst = orig;

    {
        // deep copy: separate storage, identical content
        ASSERT_NE(dst.Data(), nullptr);
        EXPECT_NE(dst.Data(), orig.Data());
        ExpectFilled<Cfg>(dst);
    }

    {
        // independence after assignment
        dst[Cfg::FieldKey(1), Cfg::RecordKey(1)] = -1;
        auto const origVal = orig.GetValue(Cfg::FieldKey(1), Cfg::RecordKey(1));
        ASSERT_TRUE(origVal.HasValue());
        EXPECT_EQ(*origVal, static_cast<int>(1 * Cfg::FLD_COUNT + 1));
    }
}

/**
 *  @brief Self-copy-assignment is a no-op and keeps the dataframe valid.
 *  @see   lugizmo::DataFrame::operator=(DataFrame const&) noexcept -> DataFrame&
 */
TYPED_TEST(RM_DataframeTors, SelfCopyAssignment)
{
    using Cfg = TypeParam;
    auto df = BuildFilled<Cfg>();

    // route through a pointer so the compiler does not flag the obvious self-assign
    auto const* const self = &df;
    df = *self;

    ASSERT_NE(df.Data(), nullptr);
    EXPECT_EQ(df.FieldSize(), Cfg::FLD_COUNT);
    EXPECT_EQ(df.RecordSize(), Cfg::REC_COUNT);
    ExpectFilled<Cfg>(df);
}

// ======= Memory release (index-neutral; counting resource) ======================================

/**
 *  @brief The destructor releases every byte the dataframe allocated.
 *  @see   lugizmo::DataFrame::~DataFrame() noexcept
 */
TEST(RM_DataframeTorsMemory, DestructorReleasesMemory)
{
    auto const res = std::make_shared<CountingResource>();
    {
        auto const df = BuildOnResource(res);
        ASSERT_NE(df.Data(), nullptr);
        ASSERT_GT(res->Outstanding(), 0u);
    }

    EXPECT_EQ(res->Outstanding(), 0u);
    EXPECT_GT(res->AllocCount(), 0u);
    EXPECT_EQ(res->AllocCount(), res->FreeCount());
}

/**
 *  @brief Move-assignment releases the target's previous storage and leaks nothing.
 *  @see   lugizmo::DataFrame::operator=(DataFrame&& other) noexcept -> DataFrame&
 */
TEST(RM_DataframeTorsMemory, MoveAssignmentReleasesTargetMemory)
{
    auto const res = std::make_shared<CountingResource>();
    {
        auto a = BuildOnResource(res);
        auto b = BuildOnResource(res);

        auto const before = res->Outstanding();
        b = std::move(a);

        EXPECT_LT(res->Outstanding(), before); // b's previous storage was released
        EXPECT_EQ(a.Data(), nullptr);          // NOLINT(bugprone-use-after-move) intentional
        ASSERT_NE(b.Data(), nullptr);
    }

    EXPECT_EQ(res->Outstanding(), 0u);
}
