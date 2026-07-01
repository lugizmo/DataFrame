// Filename: Selector.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_SELECTOR_H
#define LUGIZMO_DF_SELECTOR_H

#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace lugizmo {

    namespace internal {

        /**
         * @brief Stores a selector key by reference for lvalues and by value for rvalues.
         *
         * Borrowing avoids copying large existing keys. Owning an rvalue prevents
         * selectors created from temporary keys from dangling. A borrowing
         * selector requires its original key to outlive every use of the selector.
         */
        template<typename Key>
        class SelectorValue
        {
            using Reference = std::reference_wrapper<Key const>;
            std::variant<Reference, Key> value;

        protected:

            constexpr explicit SelectorValue(Key const& key) noexcept :
                value(std::in_place_type<Reference>, std::cref(key))
            {
            }

            constexpr explicit SelectorValue(Key&& key) noexcept(std::is_nothrow_move_constructible_v<Key>) :
                value(std::in_place_type<Key>, std::move(key))
            {
            }

        public:

            constexpr SelectorValue(SelectorValue const&) = default;
            constexpr SelectorValue(SelectorValue&&) = default;
            constexpr auto operator=(SelectorValue const&) -> SelectorValue& = default;
            constexpr auto operator=(SelectorValue&&) -> SelectorValue& = default;
            constexpr ~SelectorValue() = default;

            /**
             * @brief Returns the selected key regardless of its storage policy.
             * @return Reference to the borrowed key or to the selector's owned key.
             */
            [[nodiscard]] constexpr auto Value() const noexcept -> Key const&
            {
                if(auto const* reference = std::get_if<Reference>(&value)) return reference->get();
                return std::get<Key>(value);
            }
        };

    } // namespace internal

    /**
     * @brief DataFrame pipeline adaptor selecting one field's values.
     *
     * An lvalue key is borrowed without copying. An rvalue key is owned by the
     * selector. A borrowing selector must not outlive its key.
     *
     * @tparam F Field key type.
     */
    template<typename F>
    struct SelectField : internal::SelectorValue<F>
    {
        using Base = internal::SelectorValue<F>;

        constexpr explicit SelectField(F const& value) noexcept : Base(value) {}
        constexpr explicit SelectField(F&& value) noexcept(std::is_nothrow_move_constructible_v<F>) : Base(std::move(value)) {}
    };

    template<typename F>
    SelectField(F const&) -> SelectField<std::remove_cv_t<F>>;

    template<typename F>
    SelectField(F&&) -> SelectField<std::remove_cvref_t<F>>;

    /**
     * @brief DataFrame pipeline adaptor selecting one field with record keys.
     *
     * Storage and lifetime semantics match `SelectField`.
     *
     * @see SelectField
     * @tparam F Field key type.
     */
    template<typename F>
    struct SelectFieldIndexed : internal::SelectorValue<F>
    {
        using Base = internal::SelectorValue<F>;

        constexpr explicit SelectFieldIndexed(F const& value) noexcept : Base(value) {}
        constexpr explicit SelectFieldIndexed(F&& value) noexcept(std::is_nothrow_move_constructible_v<F>) : Base(std::move(value)) {}
    };

    template<typename F>
    SelectFieldIndexed(F const&) -> SelectFieldIndexed<std::remove_cv_t<F>>;

    template<typename F>
    SelectFieldIndexed(F&&) -> SelectFieldIndexed<std::remove_cvref_t<F>>;

    /**
     * @brief DataFrame pipeline adaptor selecting one record's values.
     *
     * Storage and lifetime semantics match `SelectField`.
     *
     * @see SelectField
     * @tparam R Record key type.
     */
    template<typename R>
    struct SelectRecord : internal::SelectorValue<R>
    {
        using Base = internal::SelectorValue<R>;

        constexpr explicit SelectRecord(R const& value) noexcept : Base(value) {}
        constexpr explicit SelectRecord(R&& value) noexcept(std::is_nothrow_move_constructible_v<R>) : Base(std::move(value)) {}
    };

    template<typename R>
    SelectRecord(R const&) -> SelectRecord<std::remove_cv_t<R>>;

    template<typename R>
    SelectRecord(R&&) -> SelectRecord<std::remove_cvref_t<R>>;

    /**
     * @brief DataFrame pipeline adaptor selecting one record with field keys.
     *
     * Storage and lifetime semantics match `SelectField`.
     *
     * @see SelectRecord
     * @tparam R Record key type.
     */
    template<typename R>
    struct SelectRecordIndexed : internal::SelectorValue<R>
    {
        using Base = internal::SelectorValue<R>;

        constexpr explicit SelectRecordIndexed(R const& value) noexcept : Base(value) {}
        constexpr explicit SelectRecordIndexed(R&& value) noexcept(std::is_nothrow_move_constructible_v<R>) : Base(std::move(value)) {}
    };

    template<typename R>
    SelectRecordIndexed(R const&) -> SelectRecordIndexed<std::remove_cv_t<R>>;

    template<typename R>
    SelectRecordIndexed(R&&) -> SelectRecordIndexed<std::remove_cvref_t<R>>;

} // namespace lugizmo

#endif // LUGIZMO_DF_SELECTOR_H
