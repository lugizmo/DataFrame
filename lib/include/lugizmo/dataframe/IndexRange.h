// Filename: IndexRange.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_RANGE_H
#define LUGIZMO_DF_INDEX_RANGE_H

#include <type_traits>
#include <utility>
#include <optional>

#include "IndexBase.h"

#include "lugizmo/Assert.h"
#include "lugizmo/container/Concepts.h"

namespace lugizmo {

    template<typename T = size_t>
    struct DFRangeIndexBounds
    {
        static_assert(std::is_integral_v<T>, "DFRangeIndexBounds must be an integral type.");

        T lower;
        T upper;

        // TODO support other types like chrono, here should be fine but what about dataframe + use std::abs when constexpr
        [[nodiscard]] constexpr auto Size() const noexcept -> size_t
        {
            LUGIZMO_ASSERT(lower <= upper, "DFRangeIndexBounds requires lower <= upper.");
            return static_cast<size_t>(upper - lower);
        }
        [[nodiscard]] constexpr auto size() const noexcept -> size_t
        {
            LUGIZMO_ASSERT(lower <= upper, "DFRangeIndexBounds requires lower <= upper.");
            return Size();
        } // TODO support other types like chrono, here should be fine but what about dataframe

        [[nodiscard]] constexpr auto Empty() const noexcept -> bool { return Size() == T(0); }  // TODO support other types like chrono, here should be fine but what about dataframe
        [[nodiscard]] constexpr auto empty() const noexcept -> bool { return Empty(); }         // TODO support other types like chrono, here should be fine but what about dataframe

        struct Iterator
        {
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using iterator_category = std::random_access_iterator_tag;
            using reference         = T;
            using pointer           = T*;

            T value; // Current value being iterated

            // Dereference
            constexpr auto operator*() const noexcept -> T { return value; }
            constexpr auto operator->() const noexcept -> T const* { return &value; }

            // Increment / decrement
            constexpr Iterator& operator++() noexcept { ++value; return *this; }
            constexpr Iterator& operator--() noexcept { --value; return *this; }

            constexpr Iterator operator++(int) noexcept { auto tmp = *this; ++(*this); return tmp; }
            constexpr Iterator operator--(int) noexcept { auto tmp = *this; --(*this); return tmp; }

            // Arithmetic
            constexpr Iterator& operator+=(difference_type n) noexcept
            {
                value = static_cast<T>(value + static_cast<T>(n));
                return *this;
            }

            constexpr Iterator& operator-=(difference_type n) noexcept
            {
                value = static_cast<T>(value - static_cast<T>(n));
                return *this;
            }

            friend constexpr Iterator operator+(Iterator it, difference_type n) noexcept { it += n; return it; }
            friend constexpr Iterator operator+(difference_type n, Iterator it) noexcept { it += n; return it; }
            friend constexpr Iterator operator-(Iterator it, difference_type n) noexcept { it -= n; return it; }
            friend constexpr difference_type operator-(Iterator a, Iterator b) noexcept { return static_cast<difference_type>(a.value) - static_cast<difference_type>(b.value); }

            constexpr reference operator[](difference_type n) const noexcept { return static_cast<T>(value + static_cast<T>(n)); }

            // Comparisons
            constexpr bool operator==(Iterator const& other) const noexcept { return value == other.value; }
            constexpr bool operator!=(Iterator const& other) const noexcept { return value != other.value; }

            constexpr bool operator<(Iterator  const& other) const noexcept { return value < other.value; }
            constexpr bool operator<=(Iterator const& other) const noexcept { return value <= other.value; }
            constexpr bool operator>(Iterator  const& other) const noexcept { return value > other.value; }
            constexpr bool operator>=(Iterator const& other) const noexcept { return value >= other.value; }
        };

        using iterator = Iterator;
        [[nodiscard]] constexpr Iterator begin() const noexcept { return Iterator{.value = lower}; }
        [[nodiscard]] constexpr Iterator end() const noexcept { return Iterator{.value = upper}; }
    };

    template<typename T = size_t>
    struct DFRangeIndex final : DFBaseSequenceIndex<DFRangeIndex<T>, T>
    {
        static_assert(std::is_integral_v<T>, "DFRangeIndex must be an integral type.");
        static_assert(std::is_convertible_v<T, ssize_t>, "DFRangeIndex must be convertable to ssize_t.");

        using KeyType      = T;
        using ConstKeyType = KeyType const;
        using KeyView      = DFRangeIndexBounds<T>;

        explicit constexpr DFRangeIndex() noexcept :
            bounds{.lower = 0, .upper = 0}
        {
        }

        explicit constexpr DFRangeIndex(KeyType const lower, KeyType const upper) noexcept :
            bounds{.lower = lower <= upper ? lower : 0, .upper = upper >= lower ? upper : 0}
        {
        }

        /**
         * TODO doc + test
         */
        template<typename C>
        [[nodiscard]]
        auto Has(C const& key) const noexcept -> bool
        {
            static_assert(ComparableType<C, T>);

            // TODO put compare logic somewhere else
            if constexpr (std::is_integral_v<C> && std::is_integral_v<T>)
            {
                return std::cmp_greater_equal(key, bounds.lower) && std::cmp_less(key, bounds.upper);
            }
            else if constexpr(std::is_arithmetic_v<C> && std::is_arithmetic_v<T>)
            {
                using CT    = std::common_type_t<C, T>;
                const CT k  = static_cast<CT>(key);
                const CT lo = static_cast<CT>(bounds.lower);
                const CT hi = static_cast<CT>(bounds.upper);
                return k >= lo && k < hi;
            }
            else
            {
                return key >= bounds.lower && key < bounds.upper;
            }
        }

        /**
        * TODO doc + idea
        */
        [[nodiscard]]
        constexpr auto Keys() const noexcept -> KeyView
        {
            return {.lower = bounds.lower, .upper = bounds.upper};
        }

        /**
        * TODO doc + idea
        */
        constexpr auto Bounds() const noexcept -> DFRangeIndexBounds<T> const&
        {
            return bounds;
        }

        /**
        * TODO doc + idea
        */
        [[nodiscard]]
        constexpr auto LowerBound() const noexcept -> KeyType
        {
            return bounds.lower;
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        constexpr auto LowerBoundPosition() const noexcept -> size_t
        {
            return 0;
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        constexpr auto UpperBound() const noexcept -> KeyType
        {
            return bounds.upper;
        }

        [[nodiscard]]
        constexpr auto UpperBoundPosition() const noexcept -> size_t
        {
            return static_cast<size_t>(bounds.upper - bounds.lower);
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        constexpr auto InBound(KeyType const key) const noexcept -> bool
        {
            return key >= bounds.lower && key < bounds.upper;
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        constexpr auto SetLowerBound(KeyType const key) noexcept -> std::optional<ssize_t>
        {
            if(key > bounds.upper || key == bounds.lower) return std::nullopt;
            ssize_t diff = (key - bounds.lower) * static_cast<ssize_t>(-1);

            bounds.lower = key;
            return diff;
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        constexpr auto SetUpperBound(KeyType const key) noexcept -> std::optional<ssize_t>
        {
            if(key < bounds.lower || key == bounds.upper) return std::nullopt;
            ssize_t diff = key - bounds.upper;

            bounds.upper = key;
            return diff;
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        constexpr auto SetLowerUpperBound(std::optional<KeyType> const lower,
                                          std::optional<KeyType> const upper) noexcept -> std::pair<std::optional<ssize_t>, std::optional<ssize_t>>
        {
            auto const newLower = lower.value_or(bounds.lower);
            auto const newUpper = upper.value_or(bounds.upper);

            if(newLower > newUpper) return { std::nullopt, std::nullopt };

            // update bounds and compute differences
            std::optional<ssize_t> lowerDiff, upperDiff;

            if(newLower != bounds.lower)
            {
                auto const dLower = static_cast<ssize_t>(bounds.lower) - static_cast<ssize_t>(newLower);
                lowerDiff  = dLower;
                bounds.lower = newLower;
            }

            if(newUpper != bounds.upper)
            {
                auto const dUpper = static_cast<ssize_t>(newUpper) - static_cast<ssize_t>(bounds.upper);
                upperDiff  = dUpper;
                bounds.upper = newUpper;
            }

            return { lowerDiff, upperDiff };
        }

        // TODO add to base
        [[nodiscard]]
        constexpr auto Position(KeyType const key) const noexcept -> std::optional<size_t>
        {
            if(key < bounds.lower || key >= bounds.upper) return std::nullopt;
            return key - bounds.lower;
        }

        [[nodiscard]]
        constexpr auto Size() const noexcept -> size_t
        {
            // TODO ensure this is never < 0
            LUGIZMO_ASSERT_TRACE(bounds.lower <= bounds.upper, "DFRangeIndex invariant failed: lower bound must not exceed upper bound.");
            return static_cast<size_t>(bounds.upper - bounds.lower);
        }

        [[nodiscard]]
        constexpr auto Empty() const noexcept -> bool
        {
            return bounds.lower == bounds.upper;
        }

    private:

        DFRangeIndexBounds<T> bounds;
    };
}

#endif // LUGIZMO_DF_INDEX_RANGE_H
