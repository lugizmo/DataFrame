// Filename: RM_DataframeTorsTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the dataframe constructors/destructor (lifecycle) for the row-major layout.
// The following functions are tested here (with names of tests):
//
// ✅ move_construction / move_assignment
// - DataFrame(DataFrame&& other) noexcept;
// - auto operator=(DataFrame&& other) noexcept -> DataFrame&;
//
// ✅ copy_construction / copy_assignment / self_copy_assignment
// - DataFrame(DataFrame const&) noexcept;
// - auto operator=(DataFrame const&) noexcept -> DataFrame&;
//
// ✅ destructor_releases_memory / move_assignment_releases_target_memory
// - ~DataFrame() noexcept;
//

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include "RM_DataframeTestData.h"

namespace {

    /**
     *  @brief Memory resource that tracks outstanding bytes and alloc/free counts,
     *         forwarding the actual work to the new/delete resource.
     *
     *  Used to assert that the dataframe releases everything it allocated, which
     *  validates the destructor and the move-assignment free path deterministically
     *  (i.e. without relying on sanitizers/valgrind).
     */
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

        [[nodiscard]] auto do_is_equal(memory_resource const& other) const noexcept -> bool override
        {
            return this == &other;
        }

        std::size_t outstanding = 0;
        std::size_t allocCount  = 0;
        std::size_t freeCount   = 0;
    };

    /// @brief Assert that every cell of @param df equals the expected @p data laid out as [record][field].
    template<typename DF, typename Flds, typename Recs, typename Data>
    void ExpectContentEquals(DF const& df, Flds const& flds, Recs const& recs, Data const& data)
    {
        std::size_t fldCount = 0;
        for(auto const& fldName : flds)
        {
            std::size_t recCount = 0;
            for(auto const& recName : recs)
            {
                auto const val = df.GetValue(fldName, recName);
                ASSERT_TRUE(val.HasValue());
                EXPECT_EQ(*val, data[recCount][fldCount]);
                ++recCount;
            }
            ++fldCount;
        }
    }

} // namespace

/**
 *  @brief Moving a dataframe transfers ownership and leaves the source empty.
 *  @see   lugizmo::DataFrame::DataFrame(DataFrame&& other) noexcept;
 */
TEST(lugizmo_dataframe_tors_row_major, move_construction)
{
    using namespace lugizmo;
    using namespace lugizmo::test::str;

    auto df = DefaultDataframe();
    ASSERT_EQ(df.Records().size(), DFRecCount);
    ASSERT_EQ(df.Fields().size(), DFFldCount);
    ASSERT_NE(df.Data(), nullptr);

    auto const mvDf = std::move(df);

    // moved-from: empty and detached from storage
    EXPECT_EQ(df.Records().size(), 0u); // NOLINT(bugprone-use-after-move) intentional: validating moved-from state
    EXPECT_EQ(df.Fields().size(), 0u);
    EXPECT_EQ(df.Values().size(), 0u);
    EXPECT_EQ(df.Data(), nullptr);

    // moved-to: owns the data and keeps all indices
    EXPECT_EQ(mvDf.Records().size(), DFRecCount);
    EXPECT_EQ(mvDf.Fields().size(), DFFldCount);
    EXPECT_EQ(mvDf.Values().size(), DFRecCount * DFFldCount);
    ASSERT_NE(mvDf.Data(), nullptr);

    for(auto const& rec : DFRecords) EXPECT_TRUE(mvDf.HasRecord(rec));
    for(auto const& fld : DFFields)  EXPECT_TRUE(mvDf.HasField(fld));
    ExpectContentEquals(mvDf, DFFields, DFRecords, DFData);
}

/**
 *  @brief Move-assigning transfers ownership into the target and empties the source.
 *  @see   lugizmo::DataFrame::operator=(DataFrame&& other) noexcept -> DataFrame&;
 */
TEST(lugizmo_dataframe_tors_row_major, move_assignment)
{
    using namespace lugizmo;
    using namespace lugizmo::test::integer;

    // move into a populated target to exercise the free-then-take path
    auto src = DefaultDataframe();
    auto dst = DefaultDataframe();

    dst = std::move(src);

    EXPECT_EQ(src.Data(), nullptr); // NOLINT(bugprone-use-after-move) intentional: validating moved-from state
    EXPECT_EQ(src.Records().size(), 0u);
    EXPECT_EQ(src.Fields().size(), 0u);

    ASSERT_NE(dst.Data(), nullptr);
    EXPECT_EQ(dst.Records().size(), DFRecCount);
    EXPECT_EQ(dst.Fields().size(), DFFldCount);
    ExpectContentEquals(dst, DFFields, DFRecords, DFData);
}

/**
 *  @brief Copying yields an independent deep copy: same content, separate storage.
 *  @see   lugizmo::DataFrame::DataFrame(DataFrame const&) noexcept;
 */
TEST(lugizmo_dataframe_tors_row_major, copy_construction)
{
    using namespace lugizmo;
    using namespace lugizmo::test::integer;

    auto orig = DefaultDataframe();
    auto copy = orig;

    // independent storage, identical shape and content
    ASSERT_NE(copy.Data(), nullptr);
    EXPECT_NE(orig.Data(), copy.Data());
    EXPECT_EQ(copy.Records().size(), orig.Records().size());
    EXPECT_EQ(copy.Fields().size(), orig.Fields().size());
    ExpectContentEquals(copy, DFFields, DFRecords, DFData);

    // mutating the copy must not affect the original
    copy[DFFields[0], DFRecords[0]] = 9999;
    auto const val = copy[DFFields[0], DFRecords[0]];
    EXPECT_EQ(val, 9999);

    auto const origVal = orig.GetValue(DFFields[0], DFRecords[0]);
    ASSERT_TRUE(origVal.HasValue());
    EXPECT_EQ(*origVal, DFData[0][0]);
}

/**
 *  @brief Copy-assigning produces an independent deep copy of the source.
 *  @see   lugizmo::DataFrame::operator=(DataFrame const&) noexcept -> DataFrame&;
 */
TEST(lugizmo_dataframe_tors_row_major, copy_assignment)
{
    using namespace lugizmo;
    using namespace lugizmo::test::integer;

    auto orig = DefaultDataframe();
    auto dst  = DataFrame<int, int, int>{};

    dst = orig;

    ASSERT_NE(dst.Data(), nullptr);
    EXPECT_NE(dst.Data(), orig.Data());
    EXPECT_EQ(dst.Records().size(), DFRecCount);
    EXPECT_EQ(dst.Fields().size(), DFFldCount);
    ExpectContentEquals(dst, DFFields, DFRecords, DFData);

    // independence after assignment
    dst[DFFields[1], DFRecords[1]] = -1;
    auto const origVal = orig.GetValue(DFFields[1], DFRecords[1]);
    ASSERT_TRUE(origVal.HasValue());
    EXPECT_EQ(*origVal, DFData[1][1]);
}

/**
 *  @brief Self-copy-assignment is a no-op and keeps the dataframe valid.
 *  @see   lugizmo::DataFrame::operator=(DataFrame const&) noexcept -> DataFrame&;
 */
TEST(lugizmo_dataframe_tors_row_major, self_copy_assignment)
{
    using namespace lugizmo;
    using namespace lugizmo::test::integer;

    auto df = DefaultDataframe();

    // route through a pointer so the compiler does not flag the obvious self-assign
    auto const* const self = &df;
    df = *self;

    ASSERT_NE(df.Data(), nullptr);
    EXPECT_EQ(df.Records().size(), DFRecCount);
    EXPECT_EQ(df.Fields().size(), DFFldCount);
    ExpectContentEquals(df, DFFields, DFRecords, DFData);
}

/**
 *  @brief The destructor releases every byte the dataframe allocated.
 *  @see   lugizmo::DataFrame::~DataFrame() noexcept;
 */
TEST(lugizmo_dataframe_tors_row_major, destructor_releases_memory)
{
    using namespace lugizmo;
    using namespace lugizmo::test::integer;

    auto const res = std::make_shared<CountingResource>();
    {
        auto const df = DataFrame<int, int, int>::FromFieldsAndRecords(DFFields, DFRecords, DFData, 0, res);
        ASSERT_NE(df.Data(), nullptr);
        ASSERT_GT(res->Outstanding(), 0u);
    }

    EXPECT_EQ(res->Outstanding(), 0u);
    EXPECT_GT(res->AllocCount(), 0u);
    EXPECT_EQ(res->AllocCount(), res->FreeCount());
}

/**
 *  @brief Move-assignment releases the target's previous storage and leaks nothing.
 *  @see   lugizmo::DataFrame::operator=(DataFrame&& other) noexcept -> DataFrame&;
 */
TEST(lugizmo_dataframe_tors_row_major, move_assignment_releases_target_memory)
{
    using namespace lugizmo;
    using namespace lugizmo::test::integer;

    auto const res = std::make_shared<CountingResource>();
    {
        auto a = DataFrame<int, int, int>::FromFieldsAndRecords(DFFields, DFRecords, DFData, 0, res);
        auto b = DataFrame<int, int, int>::FromFieldsAndRecords(DFFields, DFRecords, DFData, 0, res);

        auto const before = res->Outstanding();
        b = std::move(a);

        EXPECT_LT(res->Outstanding(), before); // b's previous storage was released
        EXPECT_EQ(a.Data(), nullptr);          // NOLINT(bugprone-use-after-move) intentional
        ASSERT_NE(b.Data(), nullptr);
        ExpectContentEquals(b, DFFields, DFRecords, DFData);
    }

    EXPECT_EQ(res->Outstanding(), 0u);
}
