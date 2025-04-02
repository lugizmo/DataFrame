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

namespace lugizmo {

    /**
     *  TODO doc
     *  @tparam Derived
     *  @tparam KeyType
     */
    template <typename Derived, typename KeyType>
    struct DFBaseValueIndex
    {
        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Keys() const noexcept -> std::span<KeyType const>
        {
            return static_cast<Derived*>(this)->Keys();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Key(size_t const position) const noexcept -> std::optional<KeyType>
        {
            return static_cast<Derived*>(this)->Key(position);
        }

        /**
         * TODO doc + idea
         */
        auto Add(KeyType&& key) noexcept -> bool
        {
            return static_cast<Derived*>(this)->Add(key);
        }

        /**
         * TODO doc + idea
         */
        auto AddMultiple(std::span<KeyType const> keys) noexcept -> bool
        {
            return static_cast<Derived*>(this)->AddMultiple(keys);
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Position(KeyType const& key) const noexcept -> std::optional<size_t>
        {
            return static_cast<Derived const*>(this)->Position(key);
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Positions() const -> std::span<size_t const>
        {
            return static_cast<Derived*>(this)->Positions();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto MaxPosition() const -> std::optional<size_t>
        {
            return static_cast<Derived*>(this)->MaxPosition();
        }

        /**
         * TODO doc + idea
         * TODO should nodiscard or better a "no-opt"
         */
        [[nodiscard]]
        auto Drop(KeyType const& key) noexcept -> bool
        {
            return static_cast<Derived*>(this)->Drop(key);
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
        auto Size() const noexcept -> KeyType
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
     *  @brief   Concept of a Dataframe Sequence Index.
     *  @details Only checks if contains a KeyType and inherits from DFBaseSequenceIndex.
     */
    template<typename T>
    concept DFSeqIndex = requires { typename T::KeyType; } && std::is_base_of_v<DFBaseSequenceIndex<T, typename T::KeyType>, T>;

    /**
     *  @brief Concept of a Dataframe Index Type.
     */
    template<typename T>
    concept DFIdxType= DFValIndex<T> || DFSeqIndex<T>;

}
#endif // LUGIZMO_DF_INDEX_BASE_H
