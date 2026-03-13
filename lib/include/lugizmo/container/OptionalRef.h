// Filename: OptionalRef.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_CONTAINER_OPTIONAL_REF_H
#define LUGIZMO_CONTAINER_OPTIONAL_REF_H

#include <concepts>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "lugizmo/Assert.h"

namespace lugizmo {

    /**
     * @brief Optional-like wrapper for references using pointer semantics.
     *
     * @details This type has only two states:
     *          - engaged: stores a valid reference
     *          - empty: no reference stored
     *
     *          There is no "engaged but null" state.
     *
     * @tparam T Type referenced by this optional wrapper.
     */
    template<typename T>
    class OptionalRef
    {
        static_assert(!std::is_reference_v<T>, "OptionalRef<T> expects non-reference T.");
        static_assert(!std::is_void_v<T>, "OptionalRef<void> is not supported.");

        T* ref;

    public:
        constexpr OptionalRef() noexcept : ref(nullptr) {}
        constexpr OptionalRef(std::nullopt_t) noexcept : ref(nullptr) {}
        constexpr explicit OptionalRef(T* value) noexcept : ref(value) {}
        constexpr OptionalRef(T& value) noexcept : ref(std::addressof(value)) {}

        template<typename U>
        requires std::is_convertible_v<U*, T*>
        constexpr OptionalRef(OptionalRef<U> const& other) noexcept : ref(other.Pointer()) {}

        constexpr OptionalRef(OptionalRef const&) noexcept = default;
        constexpr OptionalRef(OptionalRef&&) noexcept      = default;
        constexpr auto operator=(OptionalRef const&) noexcept -> OptionalRef& = default;
        constexpr auto operator=(OptionalRef&&) noexcept -> OptionalRef&      = default;

        constexpr auto operator=(std::nullopt_t) noexcept -> OptionalRef&
        {
            Reset();
            return *this;
        }

        template<typename U>
        requires (!std::same_as<std::remove_cvref_t<U>, OptionalRef>) &&
                 (!std::is_const_v<T>) &&
                 std::assignable_from<T&, U>
        constexpr auto operator=(U&& value) noexcept(noexcept(*ref = std::forward<U>(value))) -> OptionalRef&
        {
            LUGIZMO_ASSERT(ref != nullptr, "OptionalRef assignment requires a value.");
            *ref = std::forward<U>(value);
            return *this;
        }

        [[nodiscard]]
        constexpr auto HasValue() const noexcept -> bool
        {
            return ref != nullptr;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept // NOLINT(google-explicit-constructor)
        {
            return HasValue();
        }

        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wimplicit\"")
        [[nodiscard]]
        constexpr operator T&() const noexcept  // NOLINT(google-explicit-constructor)
        {
            return Value();
        }
        _Pragma("GCC diagnostic pop")

        [[nodiscard]]
        constexpr auto Pointer() const noexcept -> T*
        {
            return ref;
        }

        constexpr auto SetReference(T& value) noexcept -> OptionalRef&
        {
            ref = std::addressof(value);
            return *this;
        }

        constexpr auto SetReference(T* value) noexcept -> OptionalRef&
        {
            ref = value;
            return *this;
        }

        [[nodiscard]]
        constexpr auto Get() const noexcept -> T&
        {
            return Value();
        }

        [[nodiscard]]
        constexpr auto Value() const noexcept -> T&
        {
            LUGIZMO_ASSERT(ref != nullptr, "OptionalRef::Value called on empty optional reference.");
            return *ref;
        }

        [[nodiscard]]
        constexpr auto ValueOr(T& fallback) const noexcept -> T&
        {
            return ref != nullptr ? *ref : fallback;
        }

        [[nodiscard]]
        constexpr auto operator*() const noexcept -> T&
        {
            LUGIZMO_ASSERT(ref != nullptr, "OptionalRef::operator* called on empty optional reference.");
            return *ref;
        }

        [[nodiscard]]
        constexpr auto operator->() const noexcept -> T*
        {
            LUGIZMO_ASSERT(ref != nullptr, "OptionalRef::operator-> called on empty optional reference.");
            return ref;
        }

        constexpr void Reset() noexcept
        {
            ref = nullptr;
        }

        [[nodiscard]]
        friend constexpr auto operator==(OptionalRef lhs, OptionalRef rhs) noexcept -> bool
        {
            return lhs.ref == rhs.ref;
        }

        [[nodiscard]]
        friend constexpr auto operator==(OptionalRef lhs, std::nullopt_t) noexcept -> bool
        {
            return lhs.ref == nullptr;
        }

        [[nodiscard]]
        friend constexpr auto operator==(std::nullopt_t, OptionalRef rhs) noexcept -> bool
        {
            return rhs.ref == nullptr;
        }

        [[nodiscard]]
        friend constexpr auto operator==(OptionalRef lhs, T const& rhs) noexcept(noexcept(lhs.Value() == rhs)) -> bool
        {
            return lhs.ref != nullptr && lhs.Value() == rhs;
        }

        [[nodiscard]]
        friend constexpr auto operator==(T const& lhs, OptionalRef rhs) noexcept(noexcept(lhs == rhs.Value())) -> bool
        {
            return rhs.ref != nullptr && lhs == rhs.Value();
        }
    };

    template<typename T>
    OptionalRef(T&) -> OptionalRef<T>;

} // namespace lugizmo

#endif // LUGIZMO_CONTAINER_OPTIONAL_REF_H
