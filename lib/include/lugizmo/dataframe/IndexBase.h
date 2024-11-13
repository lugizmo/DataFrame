// Filename: IndexBase.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_INDEX_BASE_H
#define LUGIZMO_DF_INDEX_BASE_H

namespace lugizmo {

    /**
     *  TODO doc
     *  @tparam Derived
     *  @tparam KeyType
     */
    template <typename Derived, typename KeyType>
    struct DFBaseIndex
    {
    /**
     * TODO doc + idea
     */
        [[nodiscard]]
        auto Keys() const -> std::span<KeyType const>
        {
            return static_cast<Derived*>(this)->Keys();
        }

        /**
         * TODO doc + idea
         */
        [[nodiscard]]
        auto Key(size_t const position) const -> std::optional<KeyType>
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

    /**
     *  @brief   Concept of a Dataframe Index.
     *  @details Only checks if contains a KeyType and inherits from DFBaseIndex.
     */
    template <typename Derived>
    concept DFIndex = requires { typename Derived::KeyType; } && std::is_base_of_v<DFBaseIndex<Derived, typename Derived::KeyType>, Derived>;
}
#endif // LUGIZMO_DF_INDEX_BASE_H
