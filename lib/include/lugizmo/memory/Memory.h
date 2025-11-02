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

namespace lugizmo::internal {

    /// @brief TODO doc
    ///
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
}

#endif // LUGIZMO_DF_MEMORY_H
