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

namespace lugizmo {

    namespace internal {

        /**
         * @brief Stores a selector key according to its compile-time storage policy.
         * @tparam Key     Selector key type.
         * @tparam Storage Either `Key` or `std::reference_wrapper<Key const>`.
         */
        template<typename Key, typename Storage>
        class SelectorValue;

        /** @brief Borrowed selector storage used for lvalue keys. */
        template<typename Key>
        class SelectorValue<Key, std::reference_wrapper<Key const>>
        {
            using Reference = std::reference_wrapper<Key const>;
            Reference value;

        public:

            constexpr explicit SelectorValue(Key const& key) noexcept :
                value(std::cref(key))
            {
            }

            /** @return Reference to the borrowed key. */
            [[nodiscard]] constexpr auto Value() const noexcept -> Key const&
            {
                return value.get();
            }
        };

        /** @brief Owning selector storage used for rvalue keys. */
        template<typename Key>
        class SelectorValue<Key, Key>
        {
            Key value;

        public:
            constexpr explicit SelectorValue(Key&& key) noexcept(std::is_nothrow_move_constructible_v<Key>) :
                value(std::move(key))
            {
            }

            constexpr explicit SelectorValue(Key const&& key) noexcept(std::is_nothrow_copy_constructible_v<Key>)
                requires std::is_copy_constructible_v<Key> :
                value(key)
            {
            }

            /** @return Reference to the owned key. */
            [[nodiscard]] constexpr auto Value() const noexcept -> Key const&
            {
                return value;
            }
        };

    } // namespace internal

    /**
     * @brief DataFrame pipeline adaptor selecting one field's values.
     *
     * An lvalue key is borrowed without copying. The selector owns the rvalue key.
     * A borrowing selector must not outlive its key.
     *
     * @tparam F       Field key type.
     * @tparam Storage Compile-time borrowed or owning storage policy.
     */
    template<typename F, typename Storage = F>
    struct SelectField : internal::SelectorValue<F, Storage>
    {
        using internal::SelectorValue<F, Storage>::SelectorValue;
    };

    template<typename F>
    SelectField(F&) -> SelectField<std::remove_cv_t<F>, std::reference_wrapper<std::remove_cv_t<F> const>>;

    template<typename F>
    SelectField(F&&) -> SelectField<std::remove_cvref_t<F>>;

    /**
     * @brief DataFrame pipeline adaptor selecting one field with record keys.
     *
     * Storage and lifetime semantics match `SelectField`.
     *
     * @see SelectField
     * @tparam F       Field key type.
     * @tparam Storage Compile-time borrowed or owning storage policy.
     */
    template<typename F, typename Storage = F>
    struct SelectFieldIndexed : internal::SelectorValue<F, Storage>
    {
        using internal::SelectorValue<F, Storage>::SelectorValue;
    };

    template<typename F>
    SelectFieldIndexed(F&) -> SelectFieldIndexed<std::remove_cv_t<F>, std::reference_wrapper<std::remove_cv_t<F> const>>;

    template<typename F>
    SelectFieldIndexed(F&&) -> SelectFieldIndexed<std::remove_cvref_t<F>>;

    /**
     * @brief DataFrame pipeline adaptor selecting one record's values.
     *
     * Storage and lifetime semantics match `SelectField`.
     *
     * @see SelectField
     * @tparam R       Record key type.
     * @tparam Storage Compile-time borrowed or owning storage policy.
     */
    template<typename R, typename Storage = R>
    struct SelectRecord : internal::SelectorValue<R, Storage>
    {
        using internal::SelectorValue<R, Storage>::SelectorValue;
    };

    template<typename R>
    SelectRecord(R&) -> SelectRecord<std::remove_cv_t<R>, std::reference_wrapper<std::remove_cv_t<R> const>>;

    template<typename R>
    SelectRecord(R&&) -> SelectRecord<std::remove_cvref_t<R>>;

    /**
     * @brief DataFrame pipeline adaptor selecting one record with field keys.
     *
     * Storage and lifetime semantics match `SelectField`.
     *
     * @see SelectRecord
     * @tparam R       Record key type.
     * @tparam Storage Compile-time borrowed or owning storage policy.
     */
    template<typename R, typename Storage = R>
    struct SelectRecordIndexed : internal::SelectorValue<R, Storage>
    {
        using internal::SelectorValue<R, Storage>::SelectorValue;
    };

    template<typename R>
    SelectRecordIndexed(R&) -> SelectRecordIndexed<std::remove_cv_t<R>, std::reference_wrapper<std::remove_cv_t<R> const>>;

    template<typename R>
    SelectRecordIndexed(R&&) -> SelectRecordIndexed<std::remove_cvref_t<R>>;

} // namespace lugizmo

#endif // LUGIZMO_DF_SELECTOR_H
