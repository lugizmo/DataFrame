// Filename: DataFrameMapSupport.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_CONTAINER_DATAFRAME_MAP_SUPPORT_H
#define LUGIZMO_CONTAINER_DATAFRAME_MAP_SUPPORT_H

#include <functional>
#include <string>
#include <string_view>

namespace lugizmo {

    template<typename Key>
    struct DataFrameMapHash: std::hash<Key>
    {
    };

    template<typename Key>
    struct DataFrameMapEqual: std::equal_to<Key>
    {
    };

    template<typename CharT, typename Traits, typename Alloc>
    struct DataFrameMapHash<std::basic_string<CharT, Traits, Alloc>>
    {
        using is_transparent = void;
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

    template<typename CharT, typename Traits, typename Alloc>
    struct DataFrameMapEqual<std::basic_string<CharT, Traits, Alloc>>
    {
        using is_transparent = void;
        using StringViewT    = std::basic_string_view<CharT, Traits>;

        [[nodiscard]]
        auto operator()(StringViewT const lhs, StringViewT const rhs) const noexcept -> bool
        {
            return lhs == rhs;
        }
    };

} // namespace lugizmo

#endif // LUGIZMO_CONTAINER_DATAFRAME_MAP_SUPPORT_H
