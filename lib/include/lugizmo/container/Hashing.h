// Filename: Hashing.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_CONTAINER_HASHING_H
#define LUGIZMO_CONTAINER_HASHING_H

#include <functional>
#include <string>
#include <string_view>

namespace lgz {

    /**
     * @brief   Hash functor usable as the `Hash` of an unordered associative container.
     * @details The primary template just forwards to `std::hash<Key>`. The `std::basic_string`
     *          specialization is transparent (`is_transparent`), so containers can be looked up
     *          with a `string_view` or C-string without constructing a temporary `std::string`.
     *
     * @tparam Key Key type to hash.
     */
    template<typename Key>
    struct TransparentHash: std::hash<Key>
    {
    };

    /**
     * @brief   Equality functor usable as the `KeyEqual` of an unordered associative container.
     * @details The primary template forwards to `std::equal_to<Key>`; the `std::basic_string`
     *          specialization is transparent to match `TransparentHash`.
     *
     * @tparam Key Key type to compare.
     */
    template<typename Key>
    struct TransparentEqual: std::equal_to<Key>
    {
    };

    /// @brief Transparent string hashing: hashes `std::string`, `string_view`, and C-strings alike.
    template<typename CharT, typename Traits, typename Alloc>
    struct TransparentHash<std::basic_string<CharT, Traits, Alloc>>
    {
        using is_transparent = void; // NOLINT
        using StringViewT    = std::basic_string_view<CharT, Traits>;

        [[nodiscard]]
        auto operator()(StringViewT const key) const noexcept -> std::size_t
        {
            return std::hash<StringViewT>{}(key);
        }

        [[nodiscard]]
        auto operator()(std::basic_string<CharT, Traits, Alloc> const& key) const noexcept -> std::size_t
        {
            return operator()(StringViewT{key});
        }

        [[nodiscard]]
        auto operator()(CharT const* key) const noexcept -> std::size_t
        {
            return operator()(StringViewT{key});
        }
    };

    /// @brief Transparent string equality: compares any string-like operands as `string_view`.
    template<typename CharT, typename Traits, typename Alloc>
    struct TransparentEqual<std::basic_string<CharT, Traits, Alloc>>
    {
        using is_transparent = void; // NOLINT
        using StringViewT    = std::basic_string_view<CharT, Traits>;

        [[nodiscard]]
        auto operator()(StringViewT const lhs, StringViewT const rhs) const noexcept -> bool
        {
            return lhs == rhs;
        }
    };

} // namespace lgz

#endif // LUGIZMO_CONTAINER_HASHING_H
