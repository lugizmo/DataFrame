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

    constexpr auto aI32 = Alignment<std::int32_t>();
    constexpr auto aF64 = Alignment<double>();

    // Typical cacheline / SIMD alignments are 16, 32, or 64 bytes.
    EXPECT_TRUE(aI32 == 16 or aI32 == 32 or aI32 == 64 or aI32 == alignof(int));
    EXPECT_TRUE(aF64 == 16 or aF64 == 32 or aF64 == 64 or aF64 == alignof(double));
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

    constexpr auto align = Alignment<double>();
    auto* ptr = static_cast<double*>(res.allocate(10 * sizeof(double), align));
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % align, 0u);

    res.deallocate(ptr, 10 * sizeof(double), align);
}