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

namespace lugizmo {

    template<typename T = size_t>
    struct DFRangeIndexBounds
    {
        static_assert(std::is_integral_v<T>, "DFRangeIndexBounds must be an integral type.");

        T lower;
        T upper;

        [[nodiscard]] auto Size() const noexcept { return upper - lower; }  // TODO support other types like chrono, here should be fine but what about dataframe
        [[nodiscard]] auto size() const noexcept { return Size(); }         // TODO support other types like chrono, here should be fine but what about dataframe

        [[nodiscard]] auto Empty() const noexcept { return Size() == T(0); }  // TODO support other types like chrono, here should be fine but what about dataframe
        [[nodiscard]] auto empty() const noexcept { return Empty(); }         // TODO support other types like chrono, here should be fine but what about dataframe
    };

    template<typename T = size_t>
    struct DFRangeIndex final : DFBaseSequenceIndex<DFRangeIndex<T>, T>
    {
        static_assert(std::is_integral_v<T>, "DFRangeIndex must be an integral type.");

        using KeyType = T;
        using KeyView = DFRangeIndexBounds<T>;

        explicit constexpr DFRangeIndex() noexcept :
            lowerBound(0),
            upperBound(0)
        {
        }

        explicit constexpr DFRangeIndex(KeyType const lower, KeyType const upper) noexcept :
            lowerBound(lower <= upper ? lower : 0),
            upperBound(upper >= lower ? upper : 0)
        {
        }

        /**
        * TODO doc + idea
        */
        [[nodiscard]]
        constexpr auto Keys() const noexcept -> KeyView
        {
            return {.lower = lowerBound, .upper = upperBound};
        }

        /**
        * TODO doc + idea
        */
        [[nodiscard]]
        constexpr auto LowerBound() const noexcept -> KeyType
        {
            return lowerBound;
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        constexpr auto UpperBound() const noexcept -> KeyType
        {
            return upperBound;
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        constexpr auto InBound(KeyType const key) const noexcept -> bool
        {
            return key >= lowerBound && key < upperBound;
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        constexpr auto SetLowerBound(KeyType const key) noexcept -> std::optional<KeyType>
        {
            if(key > upperBound || key == lowerBound) return std::nullopt;
            auto diff = (key - lowerBound) * -1;

            lowerBound = key;
            return diff;
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        constexpr auto SetUpperBound(KeyType const key) noexcept -> std::optional<KeyType>
        {
            if(key < lowerBound || key == upperBound) return std::nullopt;
            auto diff = key - upperBound;

            upperBound = key;
            return diff;
        }

        // TODO add to base
        [[nodiscard]]
        constexpr auto Position(KeyType const key) const noexcept -> std::optional<size_t>
        {
            if(key < lowerBound || key >= upperBound) return std::nullopt;
            return key - lowerBound;
        }

        [[nodiscard]]
        constexpr auto Size() const noexcept -> KeyType
        {
            return upperBound - lowerBound;
        }

        [[nodiscard]]
        constexpr auto Empty() const noexcept -> bool
        {
            return lowerBound == upperBound;
        }

    private:

        KeyType lowerBound;
        KeyType upperBound;
    };
}

#endif // LUGIZMO_DF_INDEX_RANGE_H
