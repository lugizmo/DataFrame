// Filename: IndexHash.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_HASH_H
#define LUGIZMO_DF_INDEX_HASH_H

#include <algorithm>
#include <functional>
#include <numeric>
#include <span>
#include <optional>
#include <ranges>

#include "lugizmo/Assert.h"
#include "lugizmo/container/DataFrameMap.h"

#include "IndexBase.h"

namespace lugizmo {

    // ====== DF INDICES ===================================================================================================================

    template<typename T>
    struct DFUniqueIndex final : DFBaseValueIndex<DFUniqueIndex<T>, T>
    {
        using KeyType      = T;
        using ConstKeyType = KeyType const;
        using KeyView      = std::span<KeyType const>;

        explicit DFUniqueIndex() noexcept : values()
        {
        }

        explicit DFUniqueIndex(std::pmr::memory_resource* memResource, size_t const capacity = 0) noexcept :
            values(memResource)
        {
            values.Reserve(capacity); // TODO check when 0
        }

        DFUniqueIndex(DFUniqueIndex const&) = default;
        auto operator=(DFUniqueIndex const&) -> DFUniqueIndex& = default;

        DFUniqueIndex(DFUniqueIndex&&) noexcept = default;
        auto operator=(DFUniqueIndex&&) noexcept -> DFUniqueIndex& = default;

        ~DFUniqueIndex() = default;

        explicit DFUniqueIndex(DFUniqueIndex const& other, std::pmr::memory_resource* memResource) noexcept :
            values(memResource),
            nextIndex(other.nextIndex)
        {
            values.Reserve(other.Size());
            values.Insert(other.Keys(), other.Positions());
        }

        template<typename C>
        [[nodiscard]]
        auto Has(C const& key) const noexcept -> bool
        {
            static_assert(requires(DataFrameMap<T, size_t> const& map, C const& lookup) {{ map.Contains(lookup) } -> std::convertible_to<bool>;},
                          "DFUniqueIndex lookup requires the exact key type or transparent hash/equality support.");

            return values.Contains(key);
        }

        [[nodiscard]]
        auto Keys() const -> std::span<KeyType const>
        {
            return values.Keys();
        }

        [[nodiscard]]
        auto Key(size_t const position) const -> std::optional<KeyType>
        {
            auto const keys = values.Keys();
            if(position >= keys.size()) return std::nullopt;

            // The invariant is maintained by Add/Drop/Sort: position == physical slot,
            // so the reverse lookup is direct array access (no linear scan).
            LUGIZMO_ASSERT_TRACE(values.Values()[position] == position, "DFUniqueIndex invariant failed: position must equal physical slot.");
            return keys[position];
        }

        auto Add(T key) noexcept -> std::optional<size_t>
        {
            if(values.Contains(key)) return std::nullopt;

            auto const index = nextIndex++;
            values.Insert(std::move(key), index);

            return index;
        }

        auto AddMultiple(std::span<KeyType const> keys) noexcept -> size_t
        {
            constexpr size_t SmallThreshold = 5; // after this we create a temporary list of positions

            size_t count = 0;
            if(keys.size() <= SmallThreshold)
            {
                for(auto key : keys) count += Add(std::move(key)) != std::nullopt;
            }
            else
            {
                // TODO find way to not create a temporary newKeys vector.
                //      maybe create a mask if the KeyType is big and not just and int etc.
                auto positions = std::pmr::vector<size_t>(values.Allocator());
                auto newKeys   = std::pmr::vector<KeyType>(values.Allocator());
                positions.reserve(keys.size());
                newKeys.reserve(keys.size());

                for(auto const& key : keys)
                {
                    if(values.Contains(key)) continue;
                    positions.emplace_back(nextIndex++);
                    newKeys.emplace_back(key);
                }

                if(newKeys.empty()) return false;

                count = newKeys.size();
                values.Insert(newKeys, positions);
            }

            return count;
        }

        auto Drop(T const& key) noexcept -> std::optional<size_t>
        {
            auto [keyIt, valIt] = values.Find(key);
            if(keyIt == values.Keys().end()) return std::nullopt;

            auto itVal = *valIt;
            auto const vals = values.Values();
            std::for_each(vals.begin(), vals.end(), [&itVal](auto& val) { if(val > itVal) val -= 1; });

            values.Erase(key);
            nextIndex--;

            return std::move(itVal);
        }

        /**
         * @brief Sort stored keys and rebuild positions in sorted order.
         *
         * @param comp comparator used to order keys.
         * @return permutation mapping each new position to its previous position.
         */
        template<typename Compare = std::less<KeyType>>
        auto Sort(Compare comp = {}) -> std::pmr::vector<size_t>
        {
            auto permutation = std::pmr::vector<size_t>(values.Allocator());
            permutation.resize(values.Size());

            std::iota(permutation.begin(), permutation.end(), 0UZ);
            std::sort(permutation.begin(), permutation.end(), [&](size_t const lhs, size_t const rhs) { return comp(values.keys[lhs], values.keys[rhs]); });

            auto sortedKeys      = std::pmr::vector<KeyType>(values.Allocator());
            auto sortedPositions = std::pmr::vector<size_t>(values.Allocator());
            sortedKeys.reserve(values.Size());
            sortedPositions.reserve(values.Size());

            for(size_t newPos = 0; newPos < permutation.size(); ++newPos)
            {
                auto const oldPos = permutation[newPos];
                sortedKeys.emplace_back(std::move(values.keys[oldPos]));
                sortedPositions.emplace_back(newPos);
            }

            values.keys   = std::move(sortedKeys);
            values.values = std::move(sortedPositions);
            values.keyToIndex.clear();
            values.keyToIndex.reserve(values.keys.size());

            for(size_t pos = 0; pos < values.keys.size(); ++pos)
            {
                values.keyToIndex.emplace(values.keys[pos], pos);
            }

            return permutation;
        }

        template<typename C>
        [[nodiscard]]
        auto Position(C const& key) const -> std::optional<size_t>
        {
            static_assert(requires(DataFrameMap<T, size_t> const& map, C const& lookup) { map.Find(lookup); },
                          "DFUniqueIndex lookup requires the exact key type or transparent hash/equality support.");

            auto const [_, valIt] = values.Find(key);
            return valIt != values.Values().end() ? std::make_optional(*valIt) : std::nullopt;
        }

        [[nodiscard]]
        auto Positions() const -> std::span<size_t const>
        {
            return values.Values();
        }

        [[nodiscard]]
        auto MaxPosition() const -> std::optional<size_t>
        {
            if(values.Empty()) return std::nullopt;

            LUGIZMO_ASSERT_TRACE(nextIndex > 0, "DFUniqueIndex invariant failed: non-empty index must have nextIndex > 0.");
            return nextIndex - 1;
        }

        [[nodiscard]]
        auto Size() const noexcept -> size_t
        {
            return values.Size();
        }

        [[nodiscard]]
        auto Empty() const noexcept -> bool
        {
            return values.Empty();
        }

    private:

        DataFrameMap<T, size_t> values;      // keys and the associated position
        size_t nextIndex = 0;                // next index to use (when taken +1)
    };

} // namespace lugizmo

#endif // LUGIZMO_DF_INDEX_HASH_H
