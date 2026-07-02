// Filename: IndexUnique.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_UNIQUE_H
#define LUGIZMO_DF_INDEX_UNIQUE_H

#include <algorithm>
#include <functional>
#include <memory_resource>
#include <numeric>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lugizmo/container/Hashing.h"

#include "IndexBase.h"

namespace lugizmo {

    // ====== DF INDICES ===================================================================================================================

    /**
     * @brief   Unique value index mapping arbitrary keys to dense row positions.
     * @details Keys are stored in physical (insertion) order; the position of a key is its
     *          index in that order, so the reverse lookup `Key(position)` is direct array
     *          access. A hash map provides O(1) `key -> position` lookup.
     *
     * @tparam T key type stored in the index.
     */
    template<typename T>
    struct DFUniqueIndex final : DFBaseUniqueIndex<DFUniqueIndex<T>, T>
    {
        using KeyType      = T;
        using ConstKeyType = KeyType const;
        using KeyView      = std::span<KeyType const>;

        /**
         * @brief Constructs an empty index backed by the default memory resource.
         */
        explicit DFUniqueIndex() noexcept = default;

        /**
         * @brief Constructs an empty index backed by a user-provided memory resource.
         *
         * @param[in] memResource Memory resource used for the key store and lookup map.
         * @param[in] capacity    Number of keys to reserve up front.
         */
        explicit DFUniqueIndex(std::pmr::memory_resource* memResource, size_t const capacity = 0) noexcept :
            keys(memResource),
            keyToPos(memResource)
        {
            keys.reserve(capacity);
            keyToPos.reserve(capacity);
        }

        /**
         * @brief Copy-constructs an index, rebinding storage to a different memory resource.
         *
         * @details Preserves both key order and positions by re-inserting keys in physical order.
         *
         * @param[in] other       Source index to copy.
         * @param[in] memResource Memory resource for the new index's storage.
         */
        explicit DFUniqueIndex(DFUniqueIndex const& other, std::pmr::memory_resource* memResource) noexcept :
            keys(memResource),
            keyToPos(memResource)
        {
            keys.reserve(other.keys.size());
            keyToPos.reserve(other.keys.size());

            for(size_t pos = 0; pos < other.keys.size(); ++pos)
            {
                keys.push_back(other.keys[pos]);
                keyToPos.emplace(keys.back(), pos);
            }
        }

        /**
         * @brief Disabled because PMR container copy construction silently selects
         *        the default memory resource instead of preserving allocator intent.
         *
         * Use `DFUniqueIndex(other, memResource)` to choose the destination resource
         * explicitly.
         */
        DFUniqueIndex(DFUniqueIndex const&) = delete;

        /**
         * @brief Copy-assigns keys and positions while retaining this index's memory resource.
         */
        auto operator=(DFUniqueIndex const&) -> DFUniqueIndex& = default;

        DFUniqueIndex(DFUniqueIndex&&) noexcept = default;
        auto operator=(DFUniqueIndex&&) noexcept -> DFUniqueIndex& = default;

        ~DFUniqueIndex() = default;

        /**
         * @brief Checks whether a key is present in the index.
         *
         * @tparam C Lookup-key type; must be the stored key type or transparently comparable to it.
         *
         * @param[in] key Key to look up.
         * @return `true` if the key is stored, otherwise `false`.
         */
        template<typename C>
        [[nodiscard]]
        auto Has(C const& key) const noexcept -> bool
        {
            static_assert(requires(KeyMap const& map, C const& lookup) {{ map.contains(lookup) } -> std::convertible_to<bool>;},
                          "DFUniqueIndex lookup requires the exact key type or transparent hash/equality support.");

            return keyToPos.contains(key);
        }

        /**
         * @brief Returns all keys in physical (position) order.
         *
         * @return View over the stored keys; index `i` is the key at position `i`.
         */
        [[nodiscard]]
        auto Keys() const -> std::span<KeyType const>
        {
            return std::span<KeyType const>(keys.data(), keys.size());
        }

        /**
         * @brief Resolves a position back to its key.
         *
         * @param[in] position Row position to look up.
         * @return Key at `position`, or `std::nullopt` if out of range.
         */
        [[nodiscard]]
        auto Key(size_t const position) const -> std::optional<KeyType>
        {
            // Position equals the physical slot by construction, so this is a direct lookup.
            if(position >= keys.size()) return std::nullopt;
            return keys[position];
        }

        /**
         * @brief Adds a key, assigning it the next free position.
         *
         * @param[in] key Key to insert (moved into storage).
         * @return Position assigned to the key, or `std::nullopt` if the key already exists.
         */
        auto Add(T key) noexcept -> std::optional<size_t>
        {
            if(keyToPos.contains(key)) return std::nullopt;

            auto const position = keys.size();
            keys.push_back(std::move(key));
            keyToPos.emplace(keys.back(), position);

            return position;
        }

        /**
         * @brief Adds multiple keys in order, skipping any that already exist.
         *
         * @param[in] newKeys Keys to insert; each new key gets the next free position.
         * @return Number of keys actually inserted.
         */
        auto AddMultiple(std::span<KeyType const> newKeys) noexcept -> size_t
        {
            auto const projected = keys.size() + newKeys.size();
            keys.reserve(projected);
            keyToPos.reserve(projected);

            size_t count = 0;
            for(auto const& key : newKeys)
            {
                if(keyToPos.contains(key)) continue;

                auto const position = keys.size();
                keys.push_back(key);
                keyToPos.emplace(keys.back(), position);
                ++count;
            }

            return count;
        }

        /**
         * @brief Removes a key and compacts the positions of the keys that followed it.
         *
         * @param[in] key Key to remove.
         * @return Position the key occupied before removal, or `std::nullopt` if it was absent.
         */
        auto Drop(T const& key) noexcept -> std::optional<size_t>
        {
            auto const it = keyToPos.find(key);
            if(it == keyToPos.end()) return std::nullopt;

            auto const position = it->second;

            keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(position));
            keyToPos.erase(it);

            // Erasing shifts every following key down one slot; re-point their positions.
            for(size_t pos = position; pos < keys.size(); ++pos) keyToPos[keys[pos]] = pos;

            return position;
        }

        /**
         * @brief Sort keys and rebuild positions in sorted order.
         *
         * @tparam Compare Comparator type ordering two keys.
         *
         * @param comp comparator used to order keys.
         * @return permutation mapping each new position to its previous position.
         */
        template<typename Compare = std::less<KeyType>>
        auto Sort(Compare comp = {}) -> std::pmr::vector<size_t>
        {
            // Build the new-to-old permutation by sorting position indices by their key.
            auto permutation = std::pmr::vector<size_t>(keys.get_allocator());
            permutation.resize(keys.size());

            std::ranges::iota(permutation, 0UZ);
            std::sort(permutation.begin(), permutation.end(), [&](size_t const lhs, size_t const rhs) { return comp(keys[lhs], keys[rhs]); });

            // Materialize keys in the new order.
            auto sortedKeys = std::pmr::vector<KeyType>(keys.get_allocator());
            sortedKeys.reserve(keys.size());
            for(auto const oldPos : permutation) sortedKeys.emplace_back(std::move(keys[oldPos]));

            keys = std::move(sortedKeys);

            // Positions are the physical slots again, so rebuild the lookup map from scratch.
            keyToPos.clear();
            keyToPos.reserve(keys.size());
            for(size_t pos = 0; pos < keys.size(); ++pos) keyToPos.emplace(keys[pos], pos);

            return permutation;
        }

        /**
         * @brief Resolves a key to its position.
         *
         * @tparam C Lookup-key type; must be the stored key type or transparently comparable to it.
         *
         * @param[in] key Key to look up.
         * @return Position of the key, or `std::nullopt` if it is absent.
         */
        template<typename C>
        [[nodiscard]]
        auto Position(C const& key) const -> std::optional<size_t>
        {
            static_assert(requires(KeyMap const& map, C const& lookup) { map.find(lookup); },
                          "DFUniqueIndex lookup requires the exact key type or transparent hash/equality support.");

            auto const it = keyToPos.find(key);
            return it != keyToPos.end() ? std::make_optional(it->second) : std::nullopt;
        }

        /**
         * @brief Returns the highest assigned position.
         *
         * @return Last position (`Size() - 1`), or `std::nullopt` when the index is empty.
         */
        [[nodiscard]]
        auto MaxPosition() const noexcept -> std::optional<size_t>
        {
            if(keys.empty()) return std::nullopt;
            return keys.size() - 1;
        }

        /**
         * @brief Returns the number of keys in the index.
         */
        [[nodiscard]]
        auto Size() const noexcept -> size_t
        {
            return keys.size();
        }

        /**
         * @brief Returns whether the index holds no keys.
         */
        [[nodiscard]]
        auto Empty() const noexcept -> bool
        {
            return keys.empty();
        }

    private:

        /// @brief Hash map type backing the `key -> position` lookup (transparent for strings).
        using KeyMap = std::pmr::unordered_map<T, size_t, TransparentHash<T>, TransparentEqual<T>>;

        std::pmr::vector<T> keys;      // key at position i, in physical (insertion) order
        KeyMap              keyToPos;  // key -> position (physical slot)
    };

} // namespace lugizmo

#endif // LUGIZMO_DF_INDEX_UNIQUE_H
