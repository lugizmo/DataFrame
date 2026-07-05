// Filename: DeducingThis.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_DEDUCING_THIS_H
#define LUGIZMO_DF_DEDUCING_THIS_H

#include <functional>
#include <optional>
#include <span>
#include <type_traits>

namespace lgz::meta {

    /**
     * @brief True when the deduced object parameter (`this auto& self`) is const.
     */
    template<typename Self>
    inline constexpr bool IsConstThisV = std::is_const_v<std::remove_reference_t<Self>>;

    /**
     * @brief Function form of `IsConstThisV` for readable `if constexpr` conditions.
     */
    template<typename Self>
    [[nodiscard]]
    consteval auto IsConstThis() noexcept -> bool
    {
        return IsConstThisV<Self>;
    }

    /**
     * @brief Selects `MutableT` or `MutableT const` based on constness of `Self`.
     */
    template<typename Self, typename MutableT>
    using ThisValueT = std::conditional_t<IsConstThisV<Self>, std::add_const_t<MutableT>, MutableT>;

    /**
     * @brief Reference to `ThisValueT`.
     */
    template<typename Self, typename MutableT>
    using ThisValueRefT = ThisValueT<Self, MutableT>&;

    /**
     * @brief `std::span` with element constness matching `Self`.
     */
    template<typename Self, typename MutableT>
    using ThisValueSpanT = std::span<ThisValueT<Self, MutableT>>;

    /**
     * @brief `std::reference_wrapper` with value constness matching `Self`.
     */
    template<typename Self, typename MutableT>
    using ThisRefWrapperT = std::reference_wrapper<ThisValueT<Self, MutableT>>;

    /**
     * @brief Optional reference-wrapper with value constness matching `Self`.
     */
    template<typename Self, typename MutableT>
    using ThisRefWrapperOptT = std::optional<ThisRefWrapperT<Self, MutableT>>;

} // namespace lgz::meta

#endif // LUGIZMO_DF_DEDUCING_THIS_H
