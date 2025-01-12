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

    template<typename T>
    struct DFRangeIndexBounds
    {
        T lower;
        T upper;

        auto Size() const noexcept { return upper - lower; }
        auto size() const noexcept { return Size(); }
    };

    template<typename T = size_t> requires std::is_integral_v<T>
    struct DFRangeIndex final : DFBaseSequenceIndex<DFRangeIndex<T>, T>
    {
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
            if(key > upperBound) return std::nullopt;
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
            if(key < lowerBound) return std::nullopt;
            auto diff = key - upperBound;

            upperBound = key;
            return diff;
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
