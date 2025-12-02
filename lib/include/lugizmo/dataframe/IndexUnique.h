// Filename: IndexHash.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_HASH_H
#define LUGIZMO_DF_INDEX_HASH_H

#include <cassert>
#include <algorithm>
#include <span>
#include <optional>
#include <ranges>

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

        // constructors ...
        // destructor

        template<typename C>
        [[nodiscard]]
        auto Has(C const& key) const noexcept -> bool
        {
            static_assert(ComparableType<C, T>);
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
            // TODO this is linear lookup
            //      this will be slow when operation on big ranges!
            auto const poss = values.Values();

            auto const opos = std::find(poss.begin(), poss.end(), position);
            if(opos == poss.end()) return std::nullopt;

            auto const dist = static_cast<size_t>(std::distance(poss.begin(), opos));
            auto const keys = values.Keys();

            return keys[dist];
        }

        auto Add(T&& key) noexcept -> std::optional<size_t>
        {
            if(values.Contains(key)) return std::nullopt;

            auto const index = nextIndex++;
            values.Insert(std::forward<T>(key), index);

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
                values.Insert(keys, positions);
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

        [[nodiscard]]
        auto Position(T const& key) const -> std::optional<size_t>
        {
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

            assert(nextIndex > 0);
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
}

#endif // LUGIZMO_DF_INDEX_HASH_H
