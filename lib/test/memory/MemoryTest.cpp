// Filename: MemoryTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <memory_resource>

#include "lugizmo/memory/Memory.h"

/// @brief Alignments should always be a power of two.
template<typename T>
constexpr bool IsPowerOfTwo(T const n) noexcept { return n && (n & n - 1) == 0; }

TEST(lugizmo_dataframe_memory_align, _at_least_align_t)
{
    using namespace lugizmo::internal;
    EXPECT_GE(Alignment<int>(), alignof(int));
    EXPECT_GE(Alignment<double>(), alignof(double));
    EXPECT_GE(Alignment<std::max_align_t>(), alignof(std::max_align_t));
}

TEST(lugizmo_dataframe_memory_align, power_of_two)
{
    using namespace lugizmo::internal;
    EXPECT_TRUE(IsPowerOfTwo(Alignment<int>()));
    EXPECT_TRUE(IsPowerOfTwo(Alignment<double>()));
    EXPECT_TRUE(IsPowerOfTwo(Alignment<std::byte>()));
}

TEST(lugizmo_dataframe_memory_align, typical_values)
{
    using namespace lugizmo::internal;

    constexpr auto cache = std::hardware_destructive_interference_size;
    constexpr auto aI32  = Alignment<std::int32_t>();
    constexpr auto aF64  = Alignment<double>();

    // Typical cacheline / SIMD alignments are 16, 32, or 64 bytes.
    EXPECT_TRUE(aI32 == cache or aI32 == 16 or aI32 == 32 or aI32 == 64 or aI32 == alignof(int));
    EXPECT_TRUE(aF64 == cache or aF64 == 16 or aF64 == 32 or aF64 == 64 or aF64 == alignof(double));
}

TEST(lugizmo_dataframe_memory_align, non_arithmetic_types)
{
    using namespace lugizmo::internal;
    struct Dummy { char data[3]; };

    constexpr auto aDummy = Alignment<Dummy>();
    EXPECT_GE(aDummy, alignof(Dummy));
    EXPECT_TRUE(IsPowerOfTwo(aDummy));
}

TEST(lugizmo_dataframe_memory_align, allocated_pointer_aligned)
{
    using namespace lugizmo::internal;
    std::pmr::monotonic_buffer_resource res;

    auto* ptr = AllocateAligned<double>(res, 10);
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % Alignment<double>(), 0u);

    DeallocateAligned(res, ptr, 10);
}

TEST(lugizmo_dataframe_memory_align, aligned_helpers_handle_zero_count)
{
    using namespace lugizmo::internal;
    std::pmr::monotonic_buffer_resource res;

    auto* ptr = AllocateAligned<double>(res, 0);
    EXPECT_EQ(ptr, nullptr);

    DeallocateAligned(res, ptr, 0);
}

TEST(lugizmo_dataframe_memory_growth, default_double_small_values)
{
    using namespace lugizmo::internal;
    EXPECT_EQ(GrowthFactorDefault(0u), 8u);
    EXPECT_EQ(GrowthFactorDefault(10u), 20u);
    EXPECT_EQ(GrowthFactorDefault(999'999u), 1'999'998u);
}

TEST(lugizmo_dataframe_memory_growth, default_mid_range)
{
    using namespace lugizmo::internal;
    EXPECT_EQ(GrowthFactorDefault(1'000'000u), 1'250'000u);
}

TEST(lugizmo_dataframe_memory_growth, default_huge_values)
{
    using namespace lugizmo::internal;
    constexpr std::size_t big = 300'000'000;

    EXPECT_EQ(GrowthFactorDefault(big), big + 32'000'000);
}

TEST(lugizmo_dataframe_memory_growth, default_prevents_overflow)
{
    using namespace lugizmo::internal;
    constexpr auto max = std::numeric_limits<std::size_t>::max();
    EXPECT_EQ(GrowthFactorDefault(max - 10), max);
}
