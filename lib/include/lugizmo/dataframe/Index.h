// Filename: Index.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_H
#define LUGIZMO_DF_INDEX_H

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
    struct DFHashIndex final : DFBaseIndex<DFHashIndex<T>, T>
    {
        using KeyType = T;
        using KeyView = std::span<KeyType const>;

        explicit DFHashIndex() noexcept : values()
        {
        }

        explicit DFHashIndex(std::pmr::memory_resource* memResource, size_t const capacity = 0) :
            values(memResource)
        {
            values.Reserve(capacity); // TODO check when 0
        }

        [[nodiscard]]
        auto Keys() const -> std::span<KeyType const>
        {
            auto const keys = values.Keys();
            return std::span<KeyType const>(keys.data(), keys.size());
        }

        [[nodiscard]]
        auto Key(size_t const position) const -> std::optional<KeyType>
        {
            // TODO this is linear lookup
            //      this will be slow when operation on big ranges!
            auto const poss = values.Values();

            auto const opos = std::find(poss.begin(), poss.end(), position);
            if(opos == poss.end()) return std::nullopt;

            auto const dist = std::distance(poss.begin(), opos);
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

        auto AddMultiple(std::span<KeyType const> keys) noexcept -> bool
        {
            auto const startIndex = nextIndex;
            auto positions = std::pmr::vector<size_t>(keys.size(), values.Allocator());

            auto vectorPos = 0;
            for(size_t i = startIndex; i < keys.size(); i = ++nextIndex)
            {
                positions[vectorPos] = i;
                ++vectorPos;
            }

            values.Insert(keys, positions);
            return true;
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

    static_assert(DFIndex<DFHashIndex<int>>);
}

#endif // LUGIZMO_DF_INDEX_H
