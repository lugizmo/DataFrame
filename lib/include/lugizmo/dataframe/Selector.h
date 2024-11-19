// Filename: Selector.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_SELECTOR_H
#define LUGIZMO_DF_SELECTOR_H

namespace lugizmo {

    /**
     *  @brief   Type to denote that a field index gets passed.
     *  @details Useful on function overloading when records and fields
     *           use the same index types.
     *  @tparam F type of the field (index).
     */
    template <typename F>
    struct SelectField
    {
        F const& val;
        constexpr explicit SelectField(F const& v) noexcept: val(v) {}

        constexpr SelectField(SelectField const&) noexcept = delete;
        constexpr SelectField(SelectField &&) noexcept     = default;
        constexpr ~SelectField() noexcept                  = default;

        constexpr auto operator=(SelectField const&) noexcept -> SelectField = delete;
        constexpr auto operator=(SelectField &&) noexcept -> SelectField&    = default;
    };

    /**
     *  @brief   Type to denote that a field index gets passed.
     *           This should make the operator/function return an indexed version
     *           were the values are pared with the record index.
     *  @details Useful on function overloading when records and fields
     *           use the same index types.
     *
     *  @see SelectField if only field values should be returned.
     *  @tparam F type of the field (index).
     */
    template <typename F>
    struct SelectFieldIndexed
    {
        F const& val;
        constexpr explicit SelectFieldIndexed(F const& v) noexcept: val(v) {}

        constexpr SelectFieldIndexed(SelectFieldIndexed const&) noexcept = delete;
        constexpr SelectFieldIndexed(SelectFieldIndexed &&) noexcept     = default;
        constexpr ~SelectFieldIndexed() noexcept                         = default;

        constexpr auto operator=(SelectFieldIndexed const&) noexcept -> SelectFieldIndexed = delete;
        constexpr auto operator=(SelectFieldIndexed &&) noexcept -> SelectFieldIndexed&    = default;
    };

    /**
     *  @brief   Type to denote that a record index gets passed.
     *  @details Useful on function overloading when records and fields
     *           use the same index types.
     *  @tparam R type of the record (index).
     */
    template <typename R>
    struct SelectRecord
    {
        R const& val;
        constexpr explicit SelectRecord(R const& v) noexcept: val(v) {}

        constexpr SelectRecord(SelectRecord const&) noexcept = delete;
        constexpr SelectRecord(SelectRecord &&) noexcept     = default;
        constexpr ~SelectRecord() noexcept                   = default;

        constexpr auto operator=(SelectRecord const&) noexcept -> SelectRecord = delete;
        constexpr auto operator=(SelectRecord &&) noexcept -> SelectRecord&    = default;
    };

    /**
     *  @brief   Type to denote that a record index gets passed.
     *           This should make the operator/function return an indexed version
     *           were the values are pared with the field index.
     *  @details Useful on function overloading when records and fields
     *           use the same index types.
     *
     *  @see SelectRecord if only record values should be returned.
     *  @tparam F type of the record (index).
     */
    template <typename F>
    struct SelectRecordIndexed
    {
        F const& val;
        constexpr explicit SelectRecordIndexed(F const& v) noexcept: val(v) {}

        constexpr SelectRecordIndexed(SelectRecordIndexed const&) noexcept = delete;
        constexpr SelectRecordIndexed(SelectRecordIndexed &&) noexcept     = default;
        constexpr ~SelectRecordIndexed() noexcept                          = default;

        constexpr auto operator=(SelectRecordIndexed const&) noexcept -> SelectRecordIndexed = delete;
        constexpr auto operator=(SelectRecordIndexed &&) noexcept -> SelectRecordIndexed&    = default;
    };
}

#endif // LUGIZMO_DF_SELECTOR_H
