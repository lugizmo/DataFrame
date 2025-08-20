// Filename: References.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_REFERENCES_H
#define LUGIZMO_DF_REFERENCES_H

#include <cassert>
#include <utility>

namespace lugizmo {

    /**
     * @brief     Wrapper mimicking a reference type around a pointer.
     *            See @attention section of implications.
     *            As all types storing a pointer user is responsible to
     *            keep it valid until it is referenced.
     *
     * @attention This is a class potentially breaking reference semantics and
     *            should only be used when known that references are valid all time.
     *            You should wrap this class in something like an optional when a
     *            reference is not valid.
     *
     * @tparam T  Type to store the pointer to.
     */
    template <typename T>
    class NullableAssignableReferenceWrapper
    {
        T* ptr;

    public:

        constexpr explicit NullableAssignableReferenceWrapper() noexcept : ptr(nullptr) {}
        constexpr explicit NullableAssignableReferenceWrapper(T* ref) noexcept : ptr(ref) {}
        constexpr ~NullableAssignableReferenceWrapper() noexcept = default;

        constexpr NullableAssignableReferenceWrapper(NullableAssignableReferenceWrapper const& other) noexcept                    = default;
        constexpr NullableAssignableReferenceWrapper(NullableAssignableReferenceWrapper && other) noexcept                        = default;
        constexpr auto operator=(NullableAssignableReferenceWrapper const& other) noexcept -> NullableAssignableReferenceWrapper& = default;
        constexpr auto operator=(NullableAssignableReferenceWrapper && other) noexcept -> NullableAssignableReferenceWrapper&     = default;

        constexpr auto operator=(const T& value) noexcept -> NullableAssignableReferenceWrapper&
        {
            assert(ptr != nullptr);

            *ptr = value;
            return *this;
        }

        constexpr auto operator=(T&& value) noexcept -> NullableAssignableReferenceWrapper&
        {
            assert(ptr != nullptr);

            *ptr = std::move(value);
            return *this;
        }

        [[nodiscard]]
        constexpr auto Get() const noexcept -> T const&
        {
            assert(ptr != nullptr);
            return *ptr;
        }

        [[nodiscard]]
        constexpr auto Get() noexcept -> T&
        {
            assert(ptr != nullptr);
            return *ptr;
        }

        _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wimplicit\"")
        [[nodiscard]]
        constexpr operator T const&() const noexcept  // NOLINT(google-explicit-constructor)
        {
            assert(ptr != nullptr); return *ptr;
        }

        [[nodiscard]]
        constexpr operator T&() noexcept  // NOLINT(google-explicit-constructor)
        {
            assert(ptr != nullptr); return *ptr;
        }
        _Pragma("GCC diagnostic pop")
    };

    static_assert(std::is_trivially_copyable_v<NullableAssignableReferenceWrapper<int>>, "Iterator value should just point/reference to the actual value.");
}

#endif // LUGIZMO_DF_REFERENCES_H
