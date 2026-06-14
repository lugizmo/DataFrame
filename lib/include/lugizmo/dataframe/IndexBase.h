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
    struct DFBaseUniqueIndex
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

    /**
     * @brief   CRTP interface for range indices that map a contiguous key range to positions.
     * @details Forwards every operation to the concrete `Derived` index (e.g. `DFRangeIndex`),
     *          which stores only `[lower, upper)` bounds. Positions run `[0, Size())`, where
     *          position `0` is `LowerBound()`. Signatures mirror the concrete implementation.
     *
     * @tparam Derived Concrete index type providing the implementation.
     * @tparam KeyType Key type spanned by the range.
     */
    template <typename Derived, typename KeyType>
    struct DFBaseRangeIndex
    {
        /**
         * @brief  Returns the inclusive lower bound (the key at position `0`).
         * @return Lowest key in the range.
         */
        [[nodiscard]]
        auto LowerBound() const noexcept -> KeyType
        {
            return static_cast<Derived const*>(this)->LowerBound();
        }

        /**
         * @brief  Returns the position of the lower bound.
         * @return Always `0`.
         */
        [[nodiscard]]
        auto LowerBoundPosition() const noexcept -> size_t
        {
            return static_cast<Derived const*>(this)->LowerBoundPosition();
        }

        /**
         * @brief  Returns the exclusive upper bound (one past the last key).
         * @return One-past-the-last key in the range.
         */
        [[nodiscard]]
        auto UpperBound() const noexcept -> KeyType
        {
            return static_cast<Derived const*>(this)->UpperBound();
        }

        /**
         * @brief  Returns the position of the upper bound.
         * @return One-past-the-last position, i.e. `Size()`.
         */
        [[nodiscard]]
        auto UpperBoundPosition() const noexcept -> size_t
        {
            return static_cast<Derived const*>(this)->UpperBoundPosition();
        }

        /**
         * @brief Checks whether a key lies within `[LowerBound(), UpperBound())`.
         *
         * @param[in] key Key to test.
         * @return `true` if the key is inside the range, otherwise `false`.
         */
        [[nodiscard]]
        auto InBound(KeyType const key) const noexcept -> bool
        {
            return static_cast<Derived const*>(this)->InBound(key);
        }

        /**
         * @brief Resolves a key to its position within the range.
         *
         * @param[in] key Key to look up.
         * @return Position `(key - LowerBound()) / Step()`, or `std::nullopt` if the key is not a
         *         member of the range.
         */
        [[nodiscard]]
        auto Position(KeyType const key) const noexcept -> std::optional<size_t>
        {
            return static_cast<Derived const*>(this)->Position(key);
        }

        /**
         * @brief Resolves a position back to its key (the inverse of `Position`).
         *
         * @param[in] position Position within the range.
         * @return Key `LowerBound() + position * Step()`, or `std::nullopt` if `position` is out
         *         of range.
         */
        [[nodiscard]]
        auto Key(size_t const position) const noexcept -> std::optional<KeyType>
        {
            return static_cast<Derived const*>(this)->Key(position);
        }

        /**
         * @brief Moves the lower bound, growing or shrinking the range from the front.
         *
         * @param[in] key New lower bound.
         * @return Signed change in element count (positive when keys are added, negative when
         *         removed), or `std::nullopt` if the move is rejected (e.g. no change, or past
         *         the upper bound).
         */
        [[maybe_unused]]
        auto SetLowerBound(KeyType const key) noexcept -> std::optional<ssize_t>
        {
            return static_cast<Derived*>(this)->SetLowerBound(key);
        }

        /**
         * @brief Moves the upper bound, growing or shrinking the range from the back.
         *
         * @param[in] key New upper bound.
         * @return Signed change in element count (positive when keys are added, negative when
         *         removed), or `std::nullopt` if the move is rejected (e.g. no change, or past
         *         the lower bound).
         */
        [[maybe_unused]]
        auto SetUpperBound(KeyType const key) noexcept -> std::optional<ssize_t>
        {
            return static_cast<Derived*>(this)->SetUpperBound(key);
        }

        /**
         * @brief Returns the number of keys spanned by the range.
         */
        [[nodiscard]]
        auto Size() const noexcept -> size_t
        {
            return static_cast<Derived const*>(this)->Size();
        }

        /**
         * @brief Returns whether the range spans no keys.
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
    concept DFUnqIndex = requires { typename T::KeyType; } && std::is_base_of_v<DFBaseUniqueIndex<T, typename T::KeyType>, T>;

    /**
     *  @brief   Concept of a Dataframe Value Index, where both type must be DFValIndex.
     *  @details Only checks if contains a KeyType and inherits from DFBaseValueIndex.
     */
    template<typename T1, typename T2>
    concept DFUnqIndices = requires { DFUnqIndex<T1> and DFUnqIndex<T2>; };

    /**
     *  @brief   Concept of a Dataframe Range Index.
     *  @details Only checks if contains a KeyType and inherits from DFBaseRangeIndex.
     */
    template<typename T>
    concept DFRngIndex = requires { typename T::KeyType; } && std::is_base_of_v<DFBaseRangeIndex<T, typename T::KeyType>, T>;

    /**
     *  @brief   Concept of a Dataframe Range Index, where both types must be DFRngIndex.
     *  @details Only checks if contains a KeyType and inherits from DFBaseRangeIndex.
     */
    template<typename T1, typename T2>
    concept DFRngIndices = requires { DFRngIndex<T1> and DFRngIndex<T2>; };

    /**
     *  @brief Concept of a Dataframe Index Type.
     */
    template<typename T>
    concept DFIdxType = DFUnqIndex<T> || DFRngIndex<T>;

} // namespace lugizmo

#endif // LUGIZMO_DF_INDEX_BASE_H
