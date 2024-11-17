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

namespace lugizmo {

    template <typename Key, typename Value>
    struct DataFrameMap
    {
        struct IteratorPair
        {
            typename std::pmr::vector<Key>::const_iterator   keyIt;
            typename std::pmr::vector<Value>::const_iterator valIt;
        };

        struct MutableIteratorPair
        {
            typename std::pmr::vector<Key>::iterator   keyIt;
            typename std::pmr::vector<Value>::iterator valIt;
        };

        explicit DataFrameMap(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) :
            keys(resource),
            values(resource)
        {
        }

        void Reserve(size_t capacity)
        {
            keys.reserve(capacity);
            values.reserve(capacity);
        }

        void Insert(Key const& key, Value const& value)
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

        void Insert(std::span<Key const> inKeys, std::span<Value const> inValues)
        {
            static constexpr auto SMALL_ENTRIES_SIZE = 10U;

            assert(inKeys.size() == inValues.size() && "Spans have different sizes!");
            if (inKeys.empty()) return;

            // small element size & capacity optimization
            // don't need to create temporaries
            if(inKeys.size() <= this->keys.capacity() || inKeys.size() < SMALL_ENTRIES_SIZE)
            {
                for(size_t i = 0; i < inKeys.size(); ++i) Insert(inKeys[i], inValues[i]);
                return;
            }

            // TODO check if reserving is not better

            // temporary vectors to hold merged keys and values
            std::pmr::vector<Key>   mergedKeys(this->keys.get_allocator());
            std::pmr::vector<Value> mergedValues(this->values.get_allocator());

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
            this->keys = std::move(mergedKeys);
            this->values = std::move(mergedValues);
        }

        [[nodiscard]]
        auto Get(Key const& key) const -> std::optional<Value>
        {
            auto it = std::lower_bound(keys.begin(), keys.end(), key);

            if (it != keys.end() && *it == key)
            {
                return values[std::distance(keys.begin(), it)];
            }

            return std::nullopt;
        }

        [[nodiscard]]
        auto Find(const Key& key) const -> IteratorPair
        {
            if(auto it = std::lower_bound(keys.begin(), keys.end(), key); it != keys.end() && *it == key)
            {
                auto index = std::distance(keys.begin(), it);
                return {.keyIt = it, .valIt = values.begin() + index};
            }

            return {.keyIt = keys.end(), .valIt = values.end()};
        }

        [[nodiscard]]
        auto Find(const Key& key) -> MutableIteratorPair
        {
            if(auto it = std::lower_bound(keys.begin(), keys.end(), key); it != keys.end() && *it == key)
            {
                auto index = std::distance(keys.begin(), it);
                return {.keyIt = it, .valIt = values.begin() + index};
            }

            return {.keyIt = keys.end(), .valIt = values.end()};
        }

        [[nodiscard]]
        auto Contains(Key const& key) const -> bool
        {
            return std::binary_search(keys.begin(), keys.end(), key);
        }

        void Erase(Key const& key)
        {
            auto it = std::lower_bound(keys.begin(), keys.end(), key);

            if (it != keys.end() && *it == key)
            {
                auto index = std::distance(keys.begin(), it);
                keys.erase(it);
                values.erase(values.begin() + index);
            }
        }

        [[nodiscard]]
        auto Size() const -> size_t
        {
            return keys.size();
        }

        [[nodiscard]]
        auto Empty() const -> bool
        {
            return keys.empty();
        }

        auto Keys() const -> std::span<Key const>
        {
            return std::span(keys.data(), keys.size());
        }

        auto Values() const -> std::span<Value const>
        {
            return std::span(values.data(), values.size());
        }

        auto Values() -> std::span<Value>
        {
            return std::span(values.data(), values.size());
        }

        [[nodiscard]]
        auto Allocator() const -> typename std::pmr::vector<Key>::allocator_type //decltype(std::declval<std::pmr::vector<Key>>().get_allocator())&
        {
            return keys.get_allocator();
        }

    private:

        std::pmr::vector<Key> keys;
        std::pmr::vector<Value> values;
    };
}

#endif // LUGIZMO_CONTAINER_SIMPLE_FLAT_MAP_H
