// Filename: DataFrameMap.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H
#define LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H

#include <memory_resource>
#include <vector>
#include <algorithm>
#include <optional>
#include <span>

#include "lugizmo/Assert.h"
#include "lugizmo/container/Concepts.h"

namespace lugizmo {

    /**
     * @brief   Flatmap like used by the dataframe class.
     * @detauls Does not use hashes and buckets. A dataframe does
     *          only contain one field/record of the same name.
     *
     * @tparam Key   Type of the key to store.
     * @tparam Value Type of the value to store.
     */
    template <typename Key, typename Value>
    struct DataFrameMap
    {
        // TODO This class should be more specialized and handle matrix position only
        //      so not being a normal flat_map.

        /// @brief Const iterator to keys and values.
        struct IteratorPair
        {
            typename std::pmr::vector<Key>::const_iterator   keyIt;
            typename std::pmr::vector<Value>::const_iterator valIt;
        };

        /// @brief Mutable iterator to keys and values.
        struct MutableIteratorPair
        {
            typename std::pmr::vector<Key>::const_iterator keyIt;
            typename std::pmr::vector<Value>::iterator     valIt;
        };

        /**
         * @brief Default constructor for a DataFrameMap.
         * @param resource Excepts a user defined memory resource. If not provided
         *                 default system memory resource is used.
         */
        explicit DataFrameMap(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) noexcept;

        /**
         * @brief Reserves capacity for keys and values.
         * @param capacity to uses for keys and values storage.
         */
        void Reserve(size_t capacity) noexcept;

        /**
         *  @return Currently available capacity in the map.
         */
        auto Capacity() noexcept -> size_t;

        /**
         *  @brief Store a key value pair in the map.
         */
        void Insert(Key const& key, Value const& value) noexcept;

        /**
         * @brief   Stores a list of keys and associated values in the map.
         * @details Keys and values must have the same size otherwise nothing is stored.
         */
        void Insert(std::span<Key const> inKeys, std::span<Value const> inValues) noexcept;

        /**
         * @param key to get value for.
         * @return If key is stored in the map associated value otherwise nullopt.
         */
        [[nodiscard]]
        auto Get(Key const& key) const noexcept -> std::optional<Value>;

        /**
         * @param key to find and set value for.
         * @param val to set, if key exists.
         * @return True when value was set otherwise false.
         */
        [[maybe_unused]]
        auto Set(Key const& key, Value&& val) noexcept -> bool;

        /**
         *  @return Const iterator to key-value-pair if in map.
         */
        [[nodiscard]]
        auto Find(const Key& key) const noexcept -> IteratorPair;

        /**
         *  @return Mutable iterator to key-value-pair if in map.
         */
        [[nodiscard]]
        auto Find(const Key& key) noexcept -> MutableIteratorPair;

        /**
         *  @param key to check if in map.
         *  @return true when key in map.
         */
        template<typename C>
        [[nodiscard]]
        auto Contains(C const& key) const noexcept -> bool;

        /**
         * @param key to remove from map.
         * @return true when key was found and removed otherwise false.
         */
        [[maybe_unused]]
        auto Erase(Key const& key) noexcept -> bool;

        /// @return size of key/values stored in the map.
        [[nodiscard]]
        auto Size() const noexcept -> size_t;

        /// @return true when noting stored in the map.
        [[nodiscard]]
        auto Empty() const noexcept -> bool;

        /// @return Span to constant keys in the map. (Physical Order)
        auto Keys() const noexcept -> std::span<Key const>;

        /// @return Span to constant values in the map. (Physical Order)
        auto Values() const noexcept -> std::span<Value const>;

        /// @return Backing allocator.
        [[nodiscard]]
        auto Allocator() const noexcept -> typename std::pmr::vector<Key>::allocator_type;

    private:

        template<typename T>
        friend struct DFUniqueIndex;

        // keys[i] / values[i] are in "physical" order (e.g. matrix order)
        std::pmr::vector<Key>   keys;
        std::pmr::vector<Value> values;

        // keys[i] / values[i] are in "physical" order (e.g. matrix order)
        std::pmr::vector<size_t> keyOrder;

        /// @return Span to mutable values in the map. (Physical Order)
        auto Values() noexcept -> std::span<Value>;

        [[nodiscard]]
        auto LowerBoundKey(Key const& key) const noexcept -> typename std::pmr::vector<size_t>::const_iterator;
    };

    template<class K, class V>
    DataFrameMap<K, V>::DataFrameMap(std::pmr::memory_resource* resource) noexcept :
        keys(resource),
        values(resource),
        keyOrder(resource)
    {
    }

    template<class K, class V>
    void DataFrameMap<K, V>::Reserve(size_t capacity) noexcept
    {
        keys.reserve(capacity);
        values.reserve(capacity);
        keyOrder.reserve(capacity);
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Capacity() noexcept -> size_t
    {
        return std::min({keys.capacity(), values.capacity(), keyOrder.capacity()});
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::LowerBoundKey(K const& key) const noexcept -> typename std::pmr::vector<size_t>::const_iterator
    {
        return std::lower_bound(keyOrder.begin(), keyOrder.end(), key, [this](size_t idx, K const& k) { return keys[idx] < k; });
    }

    template<class K, class V>
    void DataFrameMap<K, V>::Insert(K const& key, V const& value) noexcept
    {
        auto const it = LowerBoundKey(key);
        if(it != keyOrder.end() && !(key < keys[*it]) && !(keys[*it] < key))
        {
            // key exists -> update value at the physical index
            values[*it] = value;
            return;
        }

        // new key: append in physical order
        auto const newIndex = keys.size();

        keys.push_back(key);
        values.push_back(value);
        keyOrder.insert(it, newIndex);

        LUGIZMO_ASSERT_TRACE(keys.size() == values.size(), "DataFrameMap invariant failed: keys and values size mismatch.");
        LUGIZMO_ASSERT_TRACE(keys.size() == keyOrder.size(), "DataFrameMap invariant failed: keys and keyOrder size mismatch.");
    }

    template<class K, class V>
    void DataFrameMap<K, V>::Insert(std::span<K const> inKeys, std::span<V const> inValues) noexcept
    {
        // check input
        if(inKeys.empty() || inKeys.size() != inValues.size()) return;

        // Reserve once to avoid repeated reallocations.
        auto const need = keys.size() + inKeys.size();
        if(keys.capacity() < need) keys.reserve(need);
        if(values.capacity() < need) values.reserve(need);
        if(keyOrder.capacity() < need) keyOrder.reserve(need);

        for (size_t i = 0; i < inKeys.size(); ++i)
        {
            Insert(inKeys[i], inValues[i]);
        }
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Get(K const& key) const noexcept -> std::optional<V>
    {
        if(auto const it = LowerBoundKey(key); it != keyOrder.end() && !(key < keys[*it]) && !(keys[*it] < key))
        {
            return values[*it];
        }

        return std::nullopt;
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Set(K const& key, V&& val) noexcept -> bool
    {
        if(auto const it = LowerBoundKey(key); it != keyOrder.end() && !(key < keys[*it]) && !(keys[*it] < key))
        {
            values[*it] = std::forward<V>(val);
            return true;
        }

        return false;
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Find(const K& key) const noexcept -> IteratorPair
    {
        if(auto const it = LowerBoundKey(key); it != keyOrder.end() && !(key < keys[*it]) && !(keys[*it] < key))
        {
            auto const idx = *it;
            return {
                .keyIt = keys.begin() + static_cast<std::ptrdiff_t>(idx),
                .valIt = values.begin() + static_cast<std::ptrdiff_t>(idx)
            };
        }

        return {.keyIt = keys.end(), .valIt = values.end()};
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Find(const K& key) noexcept -> MutableIteratorPair
    {
        if(auto const it = LowerBoundKey(key); it != keyOrder.end() && !(key < keys[*it]) && !(keys[*it] < key))
        {
            auto const idx = *it;
            return {
                .keyIt = keys.begin() + static_cast<std::ptrdiff_t>(idx),
                .valIt = values.begin() + static_cast<std::ptrdiff_t>(idx)
            };
        }

        return {.keyIt = keys.end(), .valIt = values.end()};
    }

    template<class K, class V>
    template<typename C>
    auto DataFrameMap<K, V>::Contains(C const& key) const noexcept -> bool
    {
        static_assert(ComparableType<C, K>);

        auto const it = std::lower_bound(keyOrder.begin(), keyOrder.end(), key, [this](size_t idx, C const& k) { return keys[idx] < k; });
        return it != keyOrder.end() && !(key < keys[*it]) && !(keys[*it] < key);
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Erase(K const& key) noexcept -> bool
    {
        auto const it = LowerBoundKey(key);
        if (it == keyOrder.end() || (keys[*it] < key) || (key < keys[*it])) return false;

        auto const eraseIndex = *it;

        // erase from physical storage (keys/values are in physical order)
        keys.erase(keys.begin()   + static_cast<std::ptrdiff_t>(eraseIndex));
        values.erase(values.begin() + static_cast<std::ptrdiff_t>(eraseIndex));

        // erase from keyOrder and fix its indices -> physical positions
        keyOrder.erase(it);
        for(auto& idx : keyOrder) if (idx > eraseIndex) --idx;

        LUGIZMO_ASSERT_TRACE(keys.size() == values.size(), "DataFrameMap invariant failed: keys and values size mismatch after erase.");
        LUGIZMO_ASSERT_TRACE(keys.size() == keyOrder.size(), "DataFrameMap invariant failed: keys and keyOrder size mismatch after erase.");

        return true;
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Size() const noexcept -> size_t
    {
        return keys.size();
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Empty() const noexcept -> bool
    {
        return keys.empty();
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Keys() const noexcept -> std::span<K const>
    {
        return std::span(keys.data(), keys.size());
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Values() const noexcept -> std::span<V const>
    {
        return std::span(values.data(), values.size());
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Values() noexcept -> std::span<V>
    {
        return std::span(values.data(), values.size());
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Allocator() const noexcept -> typename std::pmr::vector<K>::allocator_type
    {
        return keys.get_allocator();
    }
}

#endif // LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H
