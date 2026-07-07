// Filename: SliceConcepts.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_SLICE_CONCEPTS_H
#define LUGIZMO_DF_SLICE_CONCEPTS_H

#include <ranges>
#include <type_traits>

#include "lugizmo/dataframe/index/IndexRange.h"

namespace lgz {

    namespace internal {

        template<typename T>
        struct DFSliceRangeBoundsTraits
        {
            static constexpr bool IsBounds = false;
            using KeyType = void;
        };

        template<typename Key>
        struct DFSliceRangeBoundsTraits<DFRangeIndexBounds<Key>>
        {
            static constexpr bool IsBounds = true;
            using KeyType = Key;
        };

        template<typename T>
        using DFSliceRangeBoundsTraitsT = DFSliceRangeBoundsTraits<std::remove_cvref_t<T>>;

    } // namespace internal

    /** @brief Selection bounds compatible with a computed range index. */
    template<typename Selection, typename Index>
    concept DFSliceRangeSelectionFor =
            DFRngIndex<Index> &&
            internal::DFSliceRangeBoundsTraitsT<Selection>::IsBounds &&
            std::same_as<typename internal::DFSliceRangeBoundsTraitsT<Selection>::KeyType, typename Index::KeyType>;

    /** @brief Input range of discrete keys accepted by an index. */
    template<typename Selection, typename Index>
    concept DFSliceKeySelectionFor =
            std::ranges::input_range<Selection> &&
            !internal::DFSliceRangeBoundsTraitsT<Selection>::IsBounds &&
            requires(Index const& index, std::ranges::range_reference_t<Selection> key)
            {
                index.Position(key);
            };

    /** @brief Either calculated range bounds or a range of discrete keys for an index. */
    template<typename Selection, typename Index>
    concept DFSliceSelectionFor = DFSliceRangeSelectionFor<Selection, Index> || DFSliceKeySelectionFor<Selection, Index>;

} // namespace lgz

#endif // LUGIZMO_DF_SLICE_CONCEPTS_H
