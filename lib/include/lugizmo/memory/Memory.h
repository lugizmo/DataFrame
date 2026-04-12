// Filename: Memory.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_MEMORY_H
#define LUGIZMO_DF_MEMORY_H

#include <memory>
#include <memory_resource>
#include <new>
#include <limits>

namespace lugizmo::internal {

    /**
     *  @brief Creates the default backing resource provided by std::pmr::get_default_resource,
     *         wrapped in a shared_ptr with a no-opt deleter no to destroy the global default allocator.
     */
    inline auto BackingResDefault() noexcept -> std::shared_ptr<std::pmr::memory_resource>
    {
        // create a non-owning reference to the global default resource
        static auto defaultResource = std::shared_ptr<std::pmr::memory_resource>(
            std::pmr::get_default_resource(), [](std::pmr::memory_resource*) {});

        return defaultResource;
    }

    /**
     * @brief Preferred alignment for type T.
     *        - at least alignof(T)
     *        - prefers cache-line (hardware_destructive_interference_size)
     *        - bumps to a SIMD-friendly boundary when available
     * @tparam T type to align for.
     * @return alignment to use for type T.
     */
    template<typename T>
    consteval auto Alignment() noexcept -> std::size_t
    {
        // TODO this is pretty aggressive alignment using hardware_destructive_interference_size
        //      maybe split the alignment between scratch memory and dataframe memory
        constexpr std::size_t typeAlign = alignof(T);

        // SIMD-friendly baseline by target
#if defined(__AVX512F__)
        constexpr std::size_t simdAlign = 64;
#elif defined(__AVX2__) || defined(__ARM_FEATURE_SVE)
        constexpr std::size_t simdAlign = 32;
#elif defined(__SSE2__) || defined(__ARM_NEON)
        constexpr std::size_t simdAlign = 16;
#else
        constexpr std::size_t simdAlign = alignof(std::max_align_t);
#endif
        // Cache-line preference (C++17: constant expression)
        constexpr std::size_t cacheAlign = std::hardware_destructive_interference_size;

        // For arithmetic-ish payloads, prefer the larger of SIMD/cache;
        // otherwise just prefer cache alignment. Always respect alignof(T).
        constexpr auto arithmeticLike = std::is_arithmetic_v<T> or std::is_same_v<std::remove_cv_t<T>, std::byte>;
        constexpr auto perfAlign      = arithmeticLike ? (simdAlign > cacheAlign ? simdAlign : cacheAlign) : cacheAlign;

        return perfAlign > typeAlign ? perfAlign : typeAlign;
    }

    /**
     * @brief Allocates storage for `count` elements using Lugizmo's dataframe alignment.
     *
     * @details
     * Use this helper for dataframe backing storage and low-level backend tests. The
     * returned storage must be released with `DeallocateAligned<T>(...)` using the same
     * element count.
     *
     * @tparam T element type of the allocation.
     *
     * @param[in,out] resource PMR resource used for the allocation.
     * @param[in]     count    Number of elements to allocate.
     *
     * @return Pointer to aligned storage, or `nullptr` when `count == 0`.
     */
    template<typename T>
    auto AllocateAligned(std::pmr::memory_resource& resource, std::size_t const count) noexcept -> T*
    {
        if(count == 0) return nullptr;
        return static_cast<T*>(resource.allocate(count * sizeof(T), Alignment<T>()));
    }

    /**
     * @brief Releases storage previously allocated by `AllocateAligned<T>(...)`.
     *
     * @tparam T element type of the allocation.
     *
     * @param[in,out] resource PMR resource used for the deallocation.
     * @param[in,out] data     Pointer returned by `AllocateAligned<T>(...)`.
     * @param[in]     count    Number of allocated elements.
     */
    template<typename T>
    void DeallocateAligned(std::pmr::memory_resource& resource, T* const data, std::size_t const count) noexcept
    {
        if(data == nullptr || count == 0) return;
        resource.deallocate(data, count * sizeof(T), Alignment<T>());
    }


    /**
     * @brief   Computes a sensible next capacity for growth.
     * @details Works on element counts (not bytes). Doubles small allocations,
     *          grows 25% for medium ones, and adds +32M elements for very large arrays.
     *
     * @tparam SizeT unsigned integer type, e.g. std::size_t or uint64_t
     */
    template<std::unsigned_integral SizeT = std::size_t>
    constexpr auto GrowthFactorDefault(SizeT const current) noexcept -> SizeT
    {
        static_assert(std::numeric_limits<SizeT>::digits >= 32, "DefaultGrowthFactor requires at least 32-bit unsigned integer type.");
        static_assert(std::numeric_limits<SizeT>::max() > static_cast<SizeT>(1) * 1024 * 1024 * 1024, "Type too small to represent typical allocation sizes.");

        // sanity check: if the current value is zero, start with 8
        if(current == 0) return 8;

        // below 1M elements → ×2
        if(current < static_cast<SizeT>(1'000'000)) return static_cast<SizeT>(current * 2);

        // below 256M elements → ×1.25
        if(current < static_cast<SizeT>(256'000'000))
        {
            auto const next = static_cast<long double>(current) * 1.25L;

            if(next > static_cast<long double>(std::numeric_limits<SizeT>::max())) return std::numeric_limits<SizeT>::max();
            return static_cast<SizeT>(next);
        }

        // huge → add 32M elements
        constexpr auto step = static_cast<SizeT>(32'000'000);

        if(current > std::numeric_limits<SizeT>::max() - step) return std::numeric_limits<SizeT>::max();
        return current + step;
    }
}

#endif // LUGIZMO_DF_MEMORY_H
