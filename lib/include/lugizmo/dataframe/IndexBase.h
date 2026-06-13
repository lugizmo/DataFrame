// Filename: IndexBase.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_BASE_H
#define LUGIZMO_DF_INDEX_BASE_H

#include <type_traits>
#include <span>
#include <optional>
#include <utility>

namespace lugizmo {

    /**
     * @brief   CRTP interface for value (label) indices that map unique keys to dense positions.
     * @details Forwards every operation to the concrete `Derived` index (e.g. `DFUniqueIndex`),
     *          so generic code can operate on any value index uniformly. Signatures mirror the
     *          concrete implementation.
     *
     * @tparam Derived Concrete index type providing the implementation.
     * @tparam KeyType Key type stored in the index.
     */
    template <typename Derived, typename KeyType>
    struct DFBaseValueIndex
    {
        /**
         * @brief  Returns all keys in physical (position) order.
         * @return View over the stored keys; index `i` is the key at position `i`.
         */
        [[nodiscard]]
        auto Keys() const -> std::span<KeyType const>
        {
            return static_cast<Derived const*>(this)->Keys();
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
            return static_cast<Derived const*>(this)->Key(position);
        }

        /**
         * @brief Adds a key, assigning it the next free position.
         *
         * @param[in] key Key to insert (moved into storage).
         * @return Position assigned to the key, or `std::nullopt` if the key already exists.
         */
        auto Add(KeyType key) noexcept -> std::optional<size_t>
        {
            return static_cast<Derived*>(this)->Add(std::move(key));
        }

        /**
         * @brief Adds multiple keys in order, skipping any that already exist.
         *
         * @param[in] keys Keys to insert; each new key gets the next free position.
         * @return Number of keys actually inserted.
         */
        auto AddMultiple(std::span<KeyType const> keys) noexcept -> size_t
        {
            return static_cast<Derived*>(this)->AddMultiple(keys);
        }

        /**
         * @brief Resolves a key to its position.
         *
         * @param[in] key Key to look up.
         * @return Position of the key, or `std::nullopt` if it is absent.
         */
        [[nodiscard]]
        auto Position(KeyType const& key) const -> std::optional<size_t>
        {
            return static_cast<Derived const*>(this)->Position(key);
        }

        /**
         * @brief  Returns the highest assigned position.
         * @return Last position (`Size() - 1`), or `std::nullopt` when the index is empty.
         */
        [[nodiscard]]
        auto MaxPosition() const noexcept -> std::optional<size_t>
        {
            return static_cast<Derived const*>(this)->MaxPosition();
        }

        /**
         * @brief Removes a key and compacts the positions of the keys that followed it.
         *
         * @param[in] key Key to remove.
         * @return Position the key occupied before removal, or `std::nullopt` if it was absent.
         */
        auto Drop(KeyType const& key) noexcept -> std::optional<size_t>
        {
            return static_cast<Derived*>(this)->Drop(key);
        }

        /**
         * @brief Returns the number of keys in the index.
         */
        [[nodiscard]]
        auto Size() const noexcept -> size_t
        {
            return static_cast<Derived const*>(this)->Size();
        }

        /**
         * @brief Returns whether the index holds no keys.
         */
        [[nodiscard]]
        auto Empty() const noexcept -> bool
        {
            return static_cast<Derived const*>(this)->Empty();
        }
    };

    template <typename Derived, typename KeyType>
    struct DFBaseSequenceIndex
    {
        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto LowerBound() const noexcept -> KeyType
        {
            return static_cast<Derived*>(this)->LowerBound();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto LowerBoundPosition() const noexcept -> size_t
        {
            return static_cast<Derived*>(this)->LowerBoundPosition();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto UpperBound() const noexcept -> KeyType
        {
            return static_cast<Derived*>(this)->UpperBound();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto UpperBoundPosition() const noexcept -> size_t
        {
            return static_cast<Derived*>(this)->UpperBoundPosition();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto InBound(KeyType const key) const noexcept -> bool
        {
            return static_cast<Derived*>(this)->InBound(key);
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        auto SetLowerBound(KeyType const key) noexcept -> std::optional<KeyType>
        {
            return static_cast<Derived*>(this)->SetLowerBound(key);
        }

        /**
         * TODO doc + idea
         */
        [[maybe_unused]]
        auto SetUpperBound(KeyType const key) noexcept -> std::optional<KeyType>
        {
            return static_cast<Derived*>(this)->SetUpperBound(key);
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Size() const noexcept -> size_t
        {
            return static_cast<Derived const*>(this)->Size();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Empty() const noexcept -> bool
        {
            return static_cast<Derived const*>(this)->Empty();
        }
    };

    /**
     *  @brief   Concept of a Dataframe Value Index.
     *  @details Only checks if contains a KeyType and inherits from DFBaseValueIndex.
     */
    template<typename T>
    concept DFValIndex = requires { typename T::KeyType; } && std::is_base_of_v<DFBaseValueIndex<T, typename T::KeyType>, T>;

    /**
     *  @brief   Concept of a Dataframe Value Index, where both type must be DFValIndex.
     *  @details Only checks if contains a KeyType and inherits from DFBaseValueIndex.
     */
    template<typename T1, typename T2>
    concept DFValIndices = requires { DFValIndex<T1> and DFValIndex<T2>; };

    /**
     *  @brief   Concept of a Dataframe Sequence Index.
     *  @details Only checks if contains a KeyType and inherits from DFBaseSequenceIndex.
     */
    template<typename T>
    concept DFSeqIndex = requires { typename T::KeyType; } && std::is_base_of_v<DFBaseSequenceIndex<T, typename T::KeyType>, T>;

    /**
     *  @brief   Concept of a Dataframe Sequence Index, where both types must be DFSeqIndex.
     *  @details Only checks if contains a KeyType and inherits from DFBaseSequenceIndex.
     */
    template<typename T1, typename T2>
    concept DFSeqIndices = requires { DFSeqIndex<T1> and DFSeqIndex<T2>; };

    /**
     *  @brief Concept of a Dataframe Index Type.
     */
    template<typename T>
    concept DFIdxType = DFValIndex<T> || DFSeqIndex<T>;

}
#endif // LUGIZMO_DF_INDEX_BASE_H
