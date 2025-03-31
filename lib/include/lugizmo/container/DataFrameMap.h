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

        /// @return size of key/values stored in map.
        [[nodiscard]]
        auto Size() const noexcept -> size_t;

        /// @return true when noting stored in map.
        [[nodiscard]]
        auto Empty() const noexcept -> bool;

        /// @return Span to constant keys in map.
        auto Keys() const noexcept -> std::span<Key const>;

        /// @return Span to constant values in map.
        auto Values() const noexcept -> std::span<Value const>;

        /// @return Span to mutable values in map.
        auto Values() noexcept -> std::span<Value>;

        /// @return Backing allocator.
        [[nodiscard]]
        auto Allocator() const noexcept -> typename std::pmr::vector<Key>::allocator_type;

    private:

        std::pmr::vector<Key>   keys;
        std::pmr::vector<Value> values;
    };

    template<class K, class V>
    DataFrameMap<K, V>::DataFrameMap(std::pmr::memory_resource* resource) noexcept :
        keys(resource),
        values(resource)
    {
    }

    template<class K, class V>
    void DataFrameMap<K, V>::Reserve(size_t capacity) noexcept
    {
       keys.reserve(capacity);
       values.reserve(capacity);
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Capacity() noexcept -> size_t
    {
        return std::min(keys.capacity(), values.capacity());
    }

    template<class K, class V>
    void DataFrameMap<K, V>::Insert(K const& key, V const& value) noexcept
    {
        // find the insertion point or existing element
        auto it = std::lower_bound(keys.begin(), keys.end(), key);

        if (it != keys.end() && *it == key)
        {
            // update the value if the key exists
            values[std::distance(keys.begin(), it)] = value;
        }
        else
        {
            // calculate the insertion index
            auto index = std::distance(keys.begin(), it);

            // insert key and value at the calculated position
            keys.insert(it, key);
            values.insert(values.begin() + index, value);
        }
    }

    template<class K, class V>
    void DataFrameMap<K, V>::Insert(std::span<K const> inKeys, std::span<V const> inValues) noexcept
    {
        static constexpr auto SMALL_ENTRIES_SIZE = 10U;

        // check input
        if(inKeys.empty() || inKeys.size() != inValues.size()) return;

        // small element size & capacity optimization
        // don't need to create temporaries
        if(inKeys.size() <= this->keys.capacity() || inKeys.size() < SMALL_ENTRIES_SIZE)
        {
            for(size_t i = 0; i < inKeys.size(); ++i) Insert(inKeys[i], inValues[i]);
            return;
        }

        // TODO check if reserving is not better

        // temporary vectors to hold merged keys and values
        std::pmr::vector<K> mergedKeys(this->keys.get_allocator());
        std::pmr::vector<V> mergedValues(this->values.get_allocator());

        mergedKeys.reserve(this->keys.size() + inKeys.size());
        mergedValues.reserve(this->values.size() + inValues.size());

        // merge existing keys/values with new keys/values
        auto existingKeyIt   = this->keys.begin();
        auto existingValueIt = this->values.begin();
        auto newKeyIt        = inKeys.begin();
        auto newValueIt      = inValues.begin();

        while (existingKeyIt != this->keys.end() && newKeyIt != inKeys.end())
        {
            if (*existingKeyIt < *newKeyIt)
            {
                // existing key is smaller, keep it
                mergedKeys.push_back(*existingKeyIt);
                mergedValues.push_back(*existingValueIt);
                ++existingKeyIt;
                ++existingValueIt;
            }
            else if (*newKeyIt < *existingKeyIt)
            {
                // new key is smaller, insert it
                mergedKeys.push_back(*newKeyIt);
                mergedValues.push_back(*newValueIt);
                ++newKeyIt;
                ++newValueIt;
            }
            else
            {
                // key are equal, replace value
                mergedKeys.push_back(*existingKeyIt);
                mergedValues.push_back(*newValueIt);
                ++existingKeyIt;
                ++existingValueIt;
                ++newKeyIt;
                ++newValueIt;
            }
        }

        // add remaining elements from existing keys/values
        mergedKeys.insert(mergedKeys.end(), existingKeyIt, this->keys.end());
        mergedValues.insert(mergedValues.end(), existingValueIt, this->values.end());

        // add remaining elements from new keys/values
        while (newKeyIt != inKeys.end())
        {
            mergedKeys.push_back(*newKeyIt);
            mergedValues.push_back(*newValueIt);
            ++newKeyIt;
            ++newValueIt;
        }

        // replace existing keys and values with merged results
        this->keys   = std::move(mergedKeys);
        this->values = std::move(mergedValues);
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Get(K const& key) const noexcept -> std::optional<V>
    {
        auto it = std::lower_bound(keys.begin(), keys.end(), key);

        if (it != keys.end() && *it == key)
        {
            return values[std::distance(keys.begin(), it)];
        }

        return std::nullopt;
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Set(K const& key, V&& val) noexcept -> bool
    {
        if(auto it = Find(key); it.valIt != values.end())
        {
            *it.valIt = std::forward<V>(val);
            return true;
        }

        return false;
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Find(const K& key) const noexcept -> IteratorPair
    {
        if(auto it = std::lower_bound(keys.begin(), keys.end(), key); it != keys.end() && *it == key)
        {
            auto index = std::distance(keys.begin(), it);
            return {.keyIt = it, .valIt = values.begin() + index};
        }

        return {.keyIt = keys.end(), .valIt = values.end()};
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Find(const K& key) noexcept -> MutableIteratorPair
    {
        if(auto it = std::lower_bound(keys.begin(), keys.end(), key); it != keys.end() && *it == key)
        {
            auto index = std::distance(keys.begin(), it);
            return {.keyIt = it, .valIt = values.begin() + index};
        }

        return {.keyIt = keys.end(), .valIt = values.end()};
    }

    template<class K, class V>
    template<typename C>
    auto DataFrameMap<K, V>::Contains(C const& key) const noexcept -> bool
    {
        static_assert(ComparableType<C, K>);
        return std::binary_search(keys.begin(), keys.end(), key);
    }

    template<class K, class V>
    auto DataFrameMap<K, V>::Erase(K const& key) noexcept -> bool
    {
        auto it = std::lower_bound(keys.begin(), keys.end(), key);

        if (it != keys.end() && *it == key)
        {
            auto index = std::distance(keys.begin(), it);
            keys.erase(it);
            values.erase(values.begin() + index);

            return true;
        }

        return false;
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
