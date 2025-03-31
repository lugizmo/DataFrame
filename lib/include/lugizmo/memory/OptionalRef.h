// Filename: References.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_OPTIONAL_REFERENCE_H
#define LUGIZMO_DF_OPTIONAL_REFERENCE_H

#include <cassert>
#include <utility>
#include <compare>
#include <optional>
#include <format>

namespace lugizmo {

    /**
     *  Store references in a std::optional like type.
     *  @attention User is responsible to keep references valid.
     */
    template <typename T>
    struct OptionalRef
    {
        using value_type = T;

        // ===== MEMBER FUNCTIONS ==================================================================================================================================================

        constexpr OptionalRef() noexcept = default;
        constexpr explicit OptionalRef(T& ref) noexcept : ptr(std::addressof(ref)) {}
        constexpr explicit OptionalRef(T* ptr) noexcept : ptr(ptr) {}
        constexpr explicit OptionalRef(std::nullopt_t) noexcept : ptr(nullptr) {}

        constexpr OptionalRef(OptionalRef const&) noexcept = default;
        constexpr auto operator=(OptionalRef const&) noexcept -> OptionalRef& = default;
        constexpr OptionalRef& operator=(std::nullopt_t) noexcept { ptr = nullptr; return *this; }

        // ===== OBSERVERS =========================================================================================================================================================

        constexpr auto operator=(T& other) noexcept -> OptionalRef&;
        constexpr auto operator=(T* other) noexcept -> OptionalRef&;

        [[nodiscard]] constexpr auto Value() const noexcept -> T&;

        [[nodiscard]] constexpr auto value() const noexcept -> T&;
        [[nodiscard]] constexpr auto operator->() const noexcept -> T*;
        [[nodiscard]] constexpr auto operator*() const noexcept -> T&;

        template<typename U>
        [[nodiscard]] constexpr auto ValueOr(U&& fallback) const -> T;

        template<typename U>
        [[nodiscard]] constexpr auto value_or(U&& fallback) const -> T;

        [[nodiscard]] constexpr auto HasValue()  const noexcept -> bool;
        [[nodiscard]] constexpr auto has_value() const noexcept -> bool;
        [[nodiscard]] constexpr explicit operator bool() const noexcept;

        // ===== MODIFIERS =========================================================================================================================================================

        constexpr void Reset() noexcept;
        constexpr void reset() noexcept;
        constexpr void Emplace(T& ref) noexcept;
        constexpr void emplace(T& ref) noexcept;
        constexpr void Emplace(T* p) noexcept;
        constexpr void emplace(T* p) noexcept;
        constexpr void Swap(OptionalRef& other) noexcept;
        constexpr void swap(OptionalRef& other) noexcept;

        constexpr auto AsOptional() const noexcept  -> std::optional<T> requires std::is_copy_constructible_v<T> { return ptr ? std::optional<T>(*ptr) : std::nullopt; }
        constexpr auto as_optional() const noexcept -> std::optional<T> requires std::is_copy_constructible_v<T> { return AsOptional(); }

        // ===== MONADIC OPERATORS =================================================================================================================================================

        template<typename F> constexpr auto AndThen(F&& f) const -> decltype(f(**this));
        template<typename F> constexpr auto and_then(F&& f) const -> decltype(f(**this));
        template<typename F> constexpr auto Transform(F&& f) const -> OptionalRef<std::remove_reference_t<decltype(f(**this))>>;
        template<typename F> constexpr auto transform(F&& f) const -> OptionalRef<std::remove_reference_t<decltype(f(**this))>>;
        template<typename F> constexpr auto OrElse(F&& f) const -> OptionalRef;
        template<typename F> constexpr auto or_else(F&& f) const -> OptionalRef;

        // ===== COMPARISONS =======================================================================================================================================================

        friend bool operator==(OptionalRef const& lhs, OptionalRef const& rhs) noexcept
        {
            if (!lhs && !rhs) return true;
            if (lhs && rhs) return *lhs == *rhs;
            return false;
        }

        friend bool operator!=(OptionalRef const& lhs, OptionalRef const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend auto operator<=>(OptionalRef const& lhs, OptionalRef const& rhs) noexcept
        {
            if (!lhs && !rhs) return std::strong_ordering::equal;
            if (!lhs) return std::strong_ordering::less;
            if (!rhs) return std::strong_ordering::greater;
            return *lhs <=> *rhs;
        }

    private:

        T* ptr = nullptr;
    };

    template<typename T>
    constexpr auto OptionalRef<T>::operator=(T& other) noexcept -> OptionalRef&
    {
        ptr = std::addressof(other);
        return *this;
    }

    template<typename T>
    constexpr auto OptionalRef<T>::operator=(T* other) noexcept -> OptionalRef&
    {
        ptr = other;
        return *this;
    }

    template<typename T>
    constexpr auto OptionalRef<T>::Value() const noexcept -> T&
    {
        assert(ptr && "Dereferencing null OptionalRef");
        return *ptr;
    }

    template<typename T>
    constexpr auto OptionalRef<T>::value() const noexcept -> T&
    {
        return Value();
    }

    template<typename T>
    constexpr auto OptionalRef<T>::operator->() const noexcept -> T*
    {
        return ptr;
    }

    template<typename T>
    constexpr auto OptionalRef<T>::operator*() const noexcept -> T&
    {
        return Value();
    }

    template<typename T>
    template<typename U>
    constexpr auto OptionalRef<T>::ValueOr(U&& fallback) const -> T
    {
        return ptr ? *ptr : static_cast<T>(std::forward<U>(fallback));
    }

    template<typename T>
    template<typename U>
    constexpr auto OptionalRef<T>::value_or(U&& fallback) const -> T
    {
        return ValueOr(std::forward<U>(fallback));
    }

    template<typename T>
    constexpr auto OptionalRef<T>::HasValue()  const noexcept -> bool
    {
        return ptr != nullptr;
    }

    template<typename T>
    constexpr auto OptionalRef<T>::has_value() const noexcept -> bool
    {
        return HasValue();
    }

    template<typename T>
    constexpr OptionalRef<T>::operator bool() const noexcept
    {
        return HasValue();
    }

    template<typename T>
    constexpr void OptionalRef<T>::Reset() noexcept
    {
        ptr = nullptr;
    }


    template<typename T>
    constexpr void OptionalRef<T>::reset() noexcept
    {
        Reset();
    }

    template<typename T>
    constexpr void OptionalRef<T>::Emplace(T& ref) noexcept
    {
        ptr = std::addressof(ref);
    }

    template<typename T>
    constexpr void OptionalRef<T>::emplace(T& ref) noexcept
    {
        Emplace(ref);
    }

    template<typename T>
    constexpr void OptionalRef<T>::Emplace(T* p) noexcept
    {
        ptr = p;
    }

    template<typename T>
    constexpr void OptionalRef<T>::emplace(T* p) noexcept
    {
        Emplace(p);
    }

    template<typename T>
    constexpr void OptionalRef<T>::Swap(OptionalRef& other) noexcept
    {
        std::swap(ptr, other.ptr);
    }

    template <typename T> constexpr void OptionalRef<T>::swap(OptionalRef &other) noexcept
    {
        Swap(other);
    }

    template <typename T>
    template<typename F>
    constexpr auto OptionalRef<T>::AndThen(F&& f) const -> decltype(f(**this))
    {
        using R = decltype(f(**this));

        if(ptr) return std::forward<F>(f)(**this);
        return R{};
    }

    template<typename T>
    template<typename F>
    constexpr auto OptionalRef<T>::and_then(F&& f) const -> decltype(f(**this))
    {
        return AndThen(std::forward<F>(f));
    }

    template<typename T>
    template<typename F>
    constexpr auto OptionalRef<T>::Transform(F&& f) const -> OptionalRef<std::remove_reference_t<decltype(f(**this))>>
    {
        using U = std::remove_reference_t<decltype(f(**this))>;

        if(ptr) return OptionalRef<U>{f(**this)};
        return OptionalRef<U>{};
    }

    template<typename T>
    template<typename F>
    constexpr auto OptionalRef<T>::transform(F&& f) const -> OptionalRef<std::remove_reference_t<decltype(f(**this))>>
    {
        return Transform(std::forward<F>(f));
    }

    template<typename T>
    template<typename F>
    constexpr auto OptionalRef<T>::OrElse(F&& f) const -> OptionalRef
    {
        if (*this) return *this;
        return std::forward<F>(f)();
    }

    template<typename T>
    template<typename F>
    constexpr auto OptionalRef<T>::or_else(F&& f) const -> OptionalRef
    {
        if (*this) return *this;
        return std::forward<F>(f)();
    }

    template<typename>
    struct IsOptionalRef : std::false_type {};

    template<typename T>
    struct IsOptionalRef<OptionalRef<T>> : std::true_type {};

    template<typename T>
    concept OptionalRefType = IsOptionalRef<std::remove_cvref_t<T>>::value;

    template<typename T>
    struct RemoveOptionalRef { using Type = T; };

    template<typename T>
    struct RemoveOptionalRef<OptionalRef<T>> { using Type = T; };

    template<typename T>
    using RemovedOptionalRef = typename RemoveOptionalRef<T>::Type;
}

namespace std {

    template<typename T>
    constexpr void swap(lugizmo::OptionalRef<T>& lhs, lugizmo::OptionalRef<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<typename T>
    struct hash<lugizmo::OptionalRef<T>>
    {
        static_assert(std::is_invocable_r_v<std::size_t, std::hash<T>, T>, "Type is not hashable!");

        constexpr auto operator()(lugizmo::OptionalRef<T> const& opt) const noexcept -> size_t
        {
            if (opt) return std::hash<T>{}(*opt);
            return std::hash<std::optional<T>>{}(std::nullopt);
        }
    };

    template<typename T>
    struct formatter<lugizmo::OptionalRef<T>> : formatter<T>
    {
        template<typename FormatContext>
        constexpr auto format(lugizmo::OptionalRef<T> const& opt, FormatContext& ctx) const
        {
            if(opt) return formatter<T>::format(*opt, ctx);
            return std::format_to(ctx.out(), "null");
        }
    };
}

#endif // LUGIZMO_DF_OPTIONAL_REFERENCE_H
