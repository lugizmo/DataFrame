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

namespace lugizmo {

    template <typename Derived, typename KeyType>
    struct DFBaseIndex
    {
        [[nodiscard]]
        auto Keys() const -> std::span<KeyType const>
        {
            return static_cast<Derived*>(this)->Keys();
        }

        [[nodiscard]]
        auto Key(size_t const position) const -> std::optional<KeyType>
        {
            return static_cast<Derived*>(this)->Key(position);
        }

        auto Add(KeyType&& key) noexcept -> bool
        {
            return static_cast<Derived*>(this)->Add(key);
        }

        [[nodiscard]]
        auto Position(KeyType const& key) const noexcept -> std::optional<size_t>
        {
            return static_cast<Derived const*>(this)->Position(key);
        }

        [[nodiscard]]
        auto Positions() const -> std::span<size_t const>
        {
            return static_cast<Derived*>(this)->Positions();
        }

        [[nodiscard]]
        auto MaxPosition() const -> std::optional<size_t>
        {
            return static_cast<Derived*>(this)->MaxPosition();
        }

        [[nodiscard]]
        auto Drop(KeyType const& key) noexcept -> bool
        {
            return static_cast<Derived*>(this)->Drop(key);
        }

        [[nodiscard]]
        auto Size() const noexcept -> size_t
        {
            return static_cast<Derived const*>(this)->Size();
        }

        [[nodiscard]]
        auto Empty() const noexcept -> bool
        {
            return static_cast<Derived const*>(this)->Empty();
        }
    };

    template<template<typename, typename> class Base, typename Derived, typename KeyType>
    struct IsDFIndex
    {
        static constexpr bool value = std::is_base_of_v<Base<Derived, KeyType>, Derived>;
    };

    template<typename Derived>
    concept DFIndex = IsDFIndex<DFBaseIndex, Derived, typename Derived::KeyType>::value;

    // ====== DF INDICES ===================================================================================================================

    template<typename T>
    struct DFHashIndex final : DFBaseIndex<DFHashIndex<T>, T>
    {
        using KeyType = T;
        using KeyView = std::span<KeyType const>;

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
        size_t nextIndex = 0;
    };

    static_assert(DFIndex<DFHashIndex<int>>);

//    /**
//     *  Ranged index between to integer values.
//     */
//    struct DFRangeIndex final : DFBaseIndex<DFRangeIndex, size_t>
//    {
//        using KeyType = size_t;
//
//        size_t begin;   ///< First value to get data for. See end for limit.
//        size_t end;     ///< One behind the last element to get data for. If begin == end no data. (TODO add assert for max size_t)
//
//        explicit DFRangeIndex(size_t const inBegin, size_t const inEnd) :
//            begin(inBegin <= inEnd ? inBegin : 0),
//            end(inBegin <= inEnd ? inEnd : 0)
//        {
//            assert(inBegin <= inEnd && "End before begin!");
//        }
//
//        static auto operator[](size_t const inBegin, size_t const inEnd) -> DFRangeIndex {
//            return DFRangeIndex(inBegin, inEnd);
//        }
//
//        [[nodiscard]]
//        auto Begin() const noexcept -> KeyType
//        {
//            return begin;
//        }
//
//        [[nodiscard]]
//        auto End() const noexcept -> KeyType
//        {
//            return end;
//        }
//
//        // You cannot add to a range.
//        // TODO add function to adjust the range, including dropping when range gets smaller.
//        // auto Add(T const& key) noexcept -> std::optional<size_t>
//
//        [[nodiscard]]
//        auto Position(KeyType const& key) const -> std::optional<KeyType>
//        {
//            return key < end ? std::make_optional(key) : std::nullopt;
//        }
//
//        //[[nodiscard]]
//        //auto Drop(T const& key) noexcept -> bool {}
//
//        //
//        // void Compact() {}
//
//        [[nodiscard]]
//        auto Size() const noexcept -> KeyType
//        {
//            return end - begin;
//        }
//
//        [[nodiscard]]
//        auto Empty() const -> bool
//        {
//            return begin == end;
//        }
//    };
//
//    static_assert(DFIndex<DFRangeIndex>);
}

#endif // LUGIZMO_DF_INDEX_H
