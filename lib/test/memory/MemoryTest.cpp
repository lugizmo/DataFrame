// Filename: MemoryTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the memory helpers (alignment and growth policy).
// The following functions are tested here (with names of tests):
//
// ✅ AlignmentAtLeastAlignof / AlignmentIsPowerOfTwo / AlignmentTypicalValues / AlignmentNonArithmetic
// - Alignment<T>()
//
// ✅ AllocateAlignedIsAligned / AlignedHelpersHandleZeroCount
// - AllocateAligned<T>(res, count) / DeallocateAligned(res, ptr, count)
//
// ✅ CheckedElementBytes
// - CheckedElementBytes<T>(count)
//
// ✅ GrowthDefault*
// - GrowthFactorDefault(count)
//

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <new>

#include "lugizmo/memory/Memory.h"

using namespace lgz::internal;

namespace {

    /// @brief Alignments should always be a power of two.
    template<typename T>
    constexpr bool IsPowerOfTwo(T const n) noexcept { return n != 0 && (n & (n - 1)) == 0; }

} // namespace

/**
 *  @brief Alignment<T>() is never weaker than alignof(T).
 *  @see   lgz::internal::Alignment<T>()
 */
TEST(MemoryMemoryAlign, AlignmentAtLeastAlignof)
{
    EXPECT_GE(Alignment<int>(), alignof(int));
    EXPECT_GE(Alignment<double>(), alignof(double));
    EXPECT_GE(Alignment<std::max_align_t>(), alignof(std::max_align_t));
}

/**
 *  @brief Alignment<T>() is always a power of two.
 *  @see   lgz::internal::Alignment<T>()
 */
TEST(MemoryMemoryAlign, AlignmentIsPowerOfTwo)
{
    EXPECT_TRUE(IsPowerOfTwo(Alignment<int>()));
    EXPECT_TRUE(IsPowerOfTwo(Alignment<double>()));
    EXPECT_TRUE(IsPowerOfTwo(Alignment<std::byte>()));
}

/**
 *  @brief Alignment<T>() lands on a typical cacheline / SIMD value.
 *  @see   lgz::internal::Alignment<T>()
 */
TEST(MemoryMemoryAlign, AlignmentTypicalValues)
{
    constexpr auto cache = std::hardware_destructive_interference_size;
    constexpr auto aI32  = Alignment<std::int32_t>();
    constexpr auto aF64  = Alignment<double>();

    EXPECT_TRUE(aI32 == cache or aI32 == 16 or aI32 == 32 or aI32 == 64 or aI32 == alignof(int));
    EXPECT_TRUE(aF64 == cache or aF64 == 16 or aF64 == 32 or aF64 == 64 or aF64 == alignof(double));
}

/**
 *  @brief Alignment<T>() works for non-arithmetic types.
 *  @see   lgz::internal::Alignment<T>()
 */
TEST(MemoryMemoryAlign, AlignmentNonArithmetic)
{
    struct Dummy { char data[3]; };

    constexpr auto aDummy = Alignment<Dummy>();
    EXPECT_GE(aDummy, alignof(Dummy));
    EXPECT_TRUE(IsPowerOfTwo(aDummy));
}

/**
 *  @brief AllocateAligned returns a pointer aligned to Alignment<T>().
 *  @see   lgz::internal::AllocateAligned<T> / DeallocateAligned
 */
TEST(MemoryMemoryAlign, AllocateAlignedIsAligned)
{
    std::pmr::monotonic_buffer_resource res;

    auto* ptr = AllocateAligned<double>(res, 10);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % Alignment<double>(), 0u);

    DeallocateAligned(res, ptr, 10);
}

/**
 *  @brief A zero count allocates nothing and deallocating a null pointer is safe.
 *  @see   lgz::internal::AllocateAligned<T> / DeallocateAligned
 */
TEST(MemoryMemoryAlign, AlignedHelpersHandleZeroCount)
{
    std::pmr::monotonic_buffer_resource res;

    auto* ptr = AllocateAligned<double>(res, 0);
    EXPECT_EQ(ptr, nullptr);

    DeallocateAligned(res, ptr, 0);
}

/**
 *  @brief CheckedElementBytes converts valid counts and rejects byte-size overflow.
 *  @see   lgz::internal::CheckedElementBytes<T>()
 */
TEST(MemoryMemoryAlign, CheckedElementBytes)
{
    EXPECT_EQ(CheckedElementBytes<std::uint32_t>(3), 12);

    constexpr auto overflowingCount = std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t) + 1;
    if constexpr(lgz::AssertTraceEnabled())
    {
        EXPECT_DEATH((void)CheckedElementBytes<std::uint32_t>(overflowingCount), "allocation size");
    }
    else
    {
        EXPECT_EQ(CheckedElementBytes<std::uint32_t>(overflowingCount), std::numeric_limits<std::size_t>::max());
    }
}

/**
 *  @brief GrowthFactorDefault doubles for small inputs.
 *  @see   lgz::internal::GrowthFactorDefault(count)
 */
TEST(MemoryMemoryGrowth, GrowthDefaultSmallValues)
{
    EXPECT_EQ(GrowthFactorDefault(0u), 8u);
    EXPECT_EQ(GrowthFactorDefault(10u), 20u);
    EXPECT_EQ(GrowthFactorDefault(999'999u), 1'999'998u);
}

/**
 *  @brief GrowthFactorDefault grows by a smaller factor in the mid range.
 *  @see   lgz::internal::GrowthFactorDefault(count)
 */
TEST(MemoryMemoryGrowth, GrowthDefaultMidRange)
{
    EXPECT_EQ(GrowthFactorDefault(1'000'000u), 1'250'000u);
}

/**
 *  @brief GrowthFactorDefault grows by a fixed slab for huge inputs.
 *  @see   lgz::internal::GrowthFactorDefault(count)
 */
TEST(MemoryMemoryGrowth, GrowthDefaultHugeValues)
{
    constexpr std::size_t big = 300'000'000;
    EXPECT_EQ(GrowthFactorDefault(big), big + 32'000'000);
}

/**
 *  @brief GrowthFactorDefault saturates instead of overflowing.
 *  @see   lgz::internal::GrowthFactorDefault(count)
 */
TEST(MemoryMemoryGrowth, GrowthDefaultPreventsOverflow)
{
    constexpr auto max = std::numeric_limits<std::size_t>::max();
    EXPECT_EQ(GrowthFactorDefault(max - 10), max);
}
