// Filename: DataFrameMap.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H
#define LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H

#include <memory_resource>
#include <algorithm>
#include <concepts>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "lugizmo/Assert.h"
#include "lugizmo/container/Concepts.h"

#include "DataFrameMapSupport.h"

namespace lugizmo {

    /**
     * @brief   Flatmap like used by the dataframe class.
     * @detauls Does not use hashes and buckets. A dataframe does
     *          only contain one field/record of the same name.
     *
     * @tparam Key   Type of the key to store.
     * @tparam Value Type of the value to store.
     */
    template<typename Key, typename Value, typename Hash = DataFrameMapHash<std::remove_const_t<Key>>, typename KeyEqual = DataFrameMapEqual<std::remove_const_t<Key>>>
    struct DataFrameMap
    {
        using StoredKey = std::remove_const_t<Key>;

        static_assert(requires(StoredKey const& key) {{ Hash{}(key) } -> std::convertible_to<std::size_t>;},
                      "DataFrameMap requires hash support for the key type.");

        // TODO This class should be more specialized and handle matrix position only
        //      so not being a normal flat_map.

        /// @brief Const iterator to keys and values.
        struct IteratorPair
        {
            std::pmr::vector<Key>::const_iterator   keyIt;
            std::pmr::vector<Value>::const_iterator valIt;
        };

        /// @brief Mutable iterator to keys and values.
        struct MutableIteratorPair
        {
            std::pmr::vector<Key>::const_iterator keyIt;
            std::pmr::vector<Value>::iterator     valIt;
        };

        /**
         * @brief Default constructor for a DataFrameMap.
         * @param resource Excepts a user-defined memory resource. If not provided, the
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
         *  @details Accepts both lvalue and rvalue keys; an rvalue key is moved into storage.
         */
        template<typename InKey>
        void Insert(InKey&& key, Value const& value) noexcept;

        /**
         * @brief   Stores a list of keys and associated values in the map.
         * @details Keys and values must have the same size, otherwise nothing is stored.
         */
        void Insert(std::span<Key const> inKeys, std::span<Value const> inValues) noexcept;

        /**
         * @param key to get value for.
         * @return If the key is stored in the map associated value otherwise nullopt.
         */
        template<typename LookupKey>
        [[nodiscard]]
        auto Get(LookupKey const& key) const noexcept -> std::optional<Value>;

        /**
         * @param key to find and set value for.
         * @param val to set if the key exists.
         * @return True when value was set otherwise false.
         */
        [[maybe_unused]]
        auto Set(Key const& key, Value val) noexcept -> bool;

        /**
         *  @return Const iterator to key-value-pair if in the map.
         */
        template<typename LookupKey>
        [[nodiscard]]
        auto Find(LookupKey const& key) const noexcept -> IteratorPair;

        /**
         *  @return Mutable iterator to key-value-pair if in the map.
         */
        template<typename LookupKey>
        [[nodiscard]]
        auto Find(LookupKey const& key) noexcept -> MutableIteratorPair;

        /**
         *  @param key to check if in the map.
         *  @return true when key in the map.
         */
        template<typename C>
        [[nodiscard]]
        auto Contains(C const& key) const noexcept -> bool;

        /**
         * @param key to remove from the map.
         * @return true when the key was found and removed otherwise false.
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
        auto Allocator() const noexcept -> std::pmr::vector<Key>::allocator_type;

    private:

        template<typename T>
        friend struct DFUniqueIndex;

        // keys[i] / values[i] are in "physical" order (e.g., matrix order)
        std::pmr::vector<StoredKey>                                keys;
        std::pmr::vector<Value>                                    values;
        std::pmr::unordered_map<StoredKey, size_t, Hash, KeyEqual> keyToIndex;

        /// @return Span to mutable values in the map. (Physical Order)
        auto Values() noexcept -> std::span<Value>;

        template<typename LookupKey>
        static constexpr bool SUPPORTS_TRANSPARENT_LOOKUP = requires(Hash const& hash, KeyEqual const& equal, LookupKey const& lookup, StoredKey const& key) {
            typename Hash::is_transparent;
            typename KeyEqual::is_transparent;
            { hash(lookup) } -> std::convertible_to<std::size_t>;
            { hash(key) } -> std::convertible_to<std::size_t>;
            { equal(lookup, key) } -> std::convertible_to<bool>;
            { equal(key, lookup) } -> std::convertible_to<bool>;
        };

        template<typename LookupKey>
        static constexpr bool SUPPORTS_LOOKUP = std::same_as<std::remove_cvref_t<LookupKey>, StoredKey> || SUPPORTS_TRANSPARENT_LOOKUP<LookupKey>;
    };

    template<class K, class V, class H, class E>
    DataFrameMap<K, V, H, E>::DataFrameMap(std::pmr::memory_resource* resource) noexcept : keys(resource), values(resource), keyToIndex(0, H{}, E{}, resource)
    {
    }

    template<class K, class V, class H, class E>
    void DataFrameMap<K, V, H, E>::Reserve(size_t capacity) noexcept
    {
        keys.reserve(capacity);
        values.reserve(capacity);
        keyToIndex.reserve(capacity);
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Capacity() noexcept -> size_t
    {
        return std::min(keys.capacity(), values.capacity());
    }

    template<class K, class V, class H, class E>
    template<typename InKey>
    void DataFrameMap<K, V, H, E>::Insert(InKey&& key, V const& value) noexcept
    {
        if(auto const it = keyToIndex.find(key); it != keyToIndex.end())
        {
            values[it->second] = value;
            return;
        }

        auto const newIndex = keys.size();
        keys.push_back(std::forward<InKey>(key));
        values.push_back(value);
        keyToIndex.emplace(keys.back(), newIndex);

        LUGIZMO_ASSERT_TRACE(keys.size() == values.size(), "DataFrameMap invariant failed: keys and values size mismatch.");
        LUGIZMO_ASSERT_TRACE(keys.size() == keyToIndex.size(), "DataFrameMap invariant failed: keys and index size mismatch.");
    }

    template<class K, class V, class H, class E>
    void DataFrameMap<K, V, H, E>::Insert(std::span<K const> inKeys, std::span<V const> inValues) noexcept
    {
        // check input
        if(inKeys.empty() || inKeys.size() != inValues.size()) return;

        // Reserve once to avoid repeated reallocations.
        auto const need = keys.size() + inKeys.size();
        if(keys.capacity() < need) keys.reserve(need);
        if(values.capacity() < need) values.reserve(need);
        if(keyToIndex.size() < need) keyToIndex.reserve(need);

        for(size_t i = 0; i < inKeys.size(); ++i) Insert(inKeys[i], inValues[i]);
    }

    template<class K, class V, class H, class E>
    template<typename LookupKey>
    auto DataFrameMap<K, V, H, E>::Get(LookupKey const& key) const noexcept -> std::optional<V>
    {
        static_assert(SUPPORTS_LOOKUP<LookupKey>, "DataFrameMap lookup requires the exact key type or transparent hash/equality support.");
        if(auto const it = keyToIndex.find(key); it != keyToIndex.end()) { return values[it->second]; }

        return std::nullopt;
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Set(K const& key, V val) noexcept -> bool
    {
        if(auto const it = keyToIndex.find(key); it != keyToIndex.end())
        {
            values[it->second] = std::move(val);
            return true;
        }

        return false;
    }

    template<class K, class V, class H, class E>
    template<typename LookupKey>
    auto DataFrameMap<K, V, H, E>::Find(LookupKey const& key) const noexcept -> IteratorPair
    {
        static_assert(SUPPORTS_LOOKUP<LookupKey>, "DataFrameMap lookup requires the exact key type or transparent hash/equality support.");
        if(auto const it = keyToIndex.find(key); it != keyToIndex.end())
        {
            auto const idx = it->second;
            return {
                .keyIt = keys.begin() + static_cast<std::ptrdiff_t>(idx),
                .valIt = values.begin() + static_cast<std::ptrdiff_t>(idx)
            };
        }

        return {.keyIt = keys.end(), .valIt = values.end()};
    }

    template<class K, class V, class H, class E>
    template<typename LookupKey>
    auto DataFrameMap<K, V, H, E>::Find(LookupKey const& key) noexcept -> MutableIteratorPair
    {
        static_assert(SUPPORTS_LOOKUP<LookupKey>, "DataFrameMap lookup requires the exact key type or transparent hash/equality support.");
        if(auto const it = keyToIndex.find(key); it != keyToIndex.end())
        {
            auto const idx = it->second;
            return {.keyIt = keys.begin() + static_cast<std::ptrdiff_t>(idx), .valIt = values.begin() + static_cast<std::ptrdiff_t>(idx)};
        }

        return {.keyIt = keys.end(), .valIt = values.end()};
    }

    template<class K, class V, class H, class E>
    template<typename C>
    auto DataFrameMap<K, V, H, E>::Contains(C const& key) const noexcept -> bool
    {
        static_assert(SUPPORTS_LOOKUP<C>, "DataFrameMap lookup requires the exact key type or transparent hash/equality support.");
        return keyToIndex.contains(key);
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Erase(K const& key) noexcept -> bool
    {
        auto const it = keyToIndex.find(key);
        if(it == keyToIndex.end()) return false;
        auto const eraseIndex = it->second;
        keyToIndex.erase(it);

        // erase from physical storage (keys/values are in physical order)
        keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(eraseIndex));
        values.erase(values.begin() + static_cast<std::ptrdiff_t>(eraseIndex));

        for(size_t idx = eraseIndex; idx < keys.size(); ++idx) { keyToIndex[keys[idx]] = idx; }

        LUGIZMO_ASSERT_TRACE(keys.size() == values.size(), "DataFrameMap invariant failed: keys and values size mismatch after erase.");
        LUGIZMO_ASSERT_TRACE(keys.size() == keyToIndex.size(), "DataFrameMap invariant failed: keys and index size mismatch after erase.");

        return true;
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Size() const noexcept -> size_t
    {
        return keys.size();
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Empty() const noexcept -> bool
    {
        return keys.empty();
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Keys() const noexcept -> std::span<K const>
    {
        return std::span(keys.data(), keys.size());
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Values() const noexcept -> std::span<V const>
    {
        return std::span(values.data(), values.size());
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Values() noexcept -> std::span<V>
    {
        return std::span(values.data(), values.size());
    }

    template<class K, class V, class H, class E>
    auto DataFrameMap<K, V, H, E>::Allocator() const noexcept -> std::pmr::vector<K>::allocator_type
    {
        return keys.get_allocator();
    }

} // namespace lugizmo

#endif // LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H
