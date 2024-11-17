// Filename: DataFrame.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_DATAFRAME_H
#define LUGIZMO_DF_DATAFRAME_H

#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <iostream>
#include <mdspan>
#include <memory>
#include <memory_resource>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <ostream>

#include "memory/Memory.h"

#include "container/Concepts.h"

#include "dataframe/Index.h"
#include "dataframe/Layout.h"
#include "dataframe/View.h"
#include "dataframe/ViewIndexed.h"

namespace lugizmo {

    /**
     *  TODO extend/update documentation.
     *  @brief Dataframe of a single type using indices to access individual values
     *         or views into the data. The dataframe uses contiguous data as backend.
     *  @tparam T            Type stored in Dataframe.
     *  @tparam FldIndex     Index type of field (column)
     *  @tparam RecIndex     Index type of record (row)
     *  @tparam LayoutPolicy Underlying layout of storage to use.
     */
    template <typename T, typename FldIndex, typename RecIndex, typename LayoutPolicy = std::layout_right>
    class DataFrame
    {
        // data types
        static_assert(std::is_same_v<LayoutPolicy, std::layout_right>, "Currently only layout_right is supported");
        static_assert(std::is_default_constructible_v<T>, "Currently only default constructable values are supported");

        using Layout = std::conditional_t<std::is_same_v<LayoutPolicy, std::layout_right>, DFRowMajor<T>, void>;

        using Data = T*;                                                            // stored view into data
        using MemR = std::shared_ptr<std::pmr::memory_resource>;                    // backing memory resource type

        // field/record index & view types
        using FldI = DFHashIndex<FldIndex>;                                         // index for field values
        using RecI = DFHashIndex<RecIndex>;                                         // index for record values
        using Flds = typename FldI::KeyView;                                        // stored view into field indices
        using Recs = typename RecI::KeyView;                                        // stored view into record indices

        template<typename MDT>
        using RecsData = std::mdspan<MDT, std::dextents<size_t, 2>, LayoutPolicy>;  // stored view into data;

        // data section
        MemR        backingRes;     // memory resource to use
        size_t      capacity;       // capacity of data
        Data        data;           // pointer to allocated memory
        RecsData<T> recsData;       // view into whole stored data

        // indices/view section
        FldI fldIndex;              // index for fields
        RecI recIndex;              // index for records

    public:

        // ======== CONSTRUCTION ===========================================================================================================

        /**
         *  @brief   Default constructor that creates an empty
         *           Dataframe that does not allocate any data
         *           (except for the essentials).
         *  @details Uses the default memory resource of the system.
         */
        explicit DataFrame() noexcept;

        /**
         * @brief Empty Dataframe optionally reserving memory and using a backing
         *        memory resource.
         *
         * @param reservedValues Number of values to reserve. Number should be neither
         *                       record nor column count but the multiple of both.
         * @param res            Backing memory resource to use (defaults to system-default).
         */
        explicit DataFrame(size_t reservedValues, MemR res = BackingResDefault()) noexcept;

        /**
         * @brief Empty Dataframe with field definitions optionally reserving memory
         *        and using a backing memory resource.
         *
         * @param fields          Span of fields to initialize the vector with.
         * @param reservedValues  Optional capacity to allocate memory for.
         * @param res             Optional backing memory resource to use.
         *
         * @return Dataframe initialized with given fields.
         */
        static auto FromFields(std::span<FldIndex const> fields, size_t reservedValues = 0, MemR res = BackingResDefault()) noexcept -> DataFrame;

        /**
         * @brief Empty Dataframe with field definitions optionally reserving memory
         *        and using a backing memory resource.
         *
         * @param fields          Span of fields to initialize the vector with.
         * @param reservedValues  Optional capacity to allocate memory for.
         * @param res             Optional backing memory resource to use.
         *
         * @return Dataframe initialized with given fields.
         */
        static auto FromFields(std::initializer_list<FldIndex> fields, size_t reservedValues = 0, MemR res = BackingResDefault()) noexcept -> DataFrame;

        // TODO add option to move fields (check if really worth it)
        //static auto FromFields(Iterable auto&& fields,
        //                       size_t const capacity = 0,
        //                       MemR res = BackingResDefault()) noexcept -> DataFrame;

        /**
         *  @brief Add new fields and records; all records are
         *         initialized with the same span of values.
         *
         *  @param fldIndices fields to add to the dataframe.
         *  @param recIndices records to add to the dataframe.
         *  @param recValues  values to add to the dataframe (or Empty then default initialized)
         *  @param capacity   Optional capacity to allocate memory for. (Values are stored on that as well)
         *  @param res        Optional backing memory resource to use.
         *
         *  @return Dataframe with initialized fields and records (values).
         */
        static auto FromFieldsAndRecord(std::span<FldIndex const> fldIndices,
                                        std::span<RecIndex const> recIndices,
                                        std::span<T const>        recValues = {},
                                        size_t capacity = 0,
                                        MemR   res      = BackingResDefault()) noexcept -> DataFrame;

        /**
         *  @brief Add new fields and records; all records are
         *         initialized with the values given.
         *
         *  @param fldIndices fields to add to the dataframe.
         *  @param recIndices records to add to the dataframe.
         *  @param recValues  values to add to the dataframe (or Empty then default initialized)
         *  @param capacity   Optional capacity to allocate memory for. (Values are stored on that as well)
         *  @param res        Optional backing memory resource to use.
         *
         *  @return Dataframe with initialized fields and records (values).
         */
        static auto FromFieldsAndRecords(std::span<FldIndex const>      fldIndices,
                                         std::span<RecIndex const>      recIndices,
                                         IterableOfIterable auto const& recValues,
                                         size_t capacity = 0,
                                         MemR res        = BackingResDefault()) noexcept -> DataFrame;

        /**
         *  @brief Add new fields and records; all records are
         *         initialized with the values given.
         *
         *  @param fldIndices fields to add to the dataframe.
         *  @param recIndices records to add to the dataframe.
         *  @param recValues  values to add to the dataframe (or Empty then default initialized)
         *  @param capacity   Optional capacity to allocate memory for. (Values are stored on that as well)
         *  @param res        Optional backing memory resource to use.
         *
         *  @return Dataframe with initialized fields and records (values).
         */
        static auto FromFieldsAndRecords(std::initializer_list<FldIndex const>           fldIndices,
                                         std::initializer_list<RecIndex const>           recIndices,
                                         std::initializer_list<std::initializer_list<T>> recValues = {},
                                         size_t capacity = 0,
                                         MemR   res      = BackingResDefault()) noexcept -> DataFrame;

        // TODO dataframe needs a way to decide if "iota" is appropriate to add as record indices
        //      but this makes only sense when adding of new records later allows for that.
        //static auto FromFieldsAndRecords(std::pair<FldIndex const, std::initializer_list<T const>> fieldToValues,
        //                                 size_t capacity = 0,
        //                                 MemR   res      = BackingResDefault()) noexcept -> DataFrame;

        // ======== COPY, MOVE & DELETE ====================================================================================================

        DataFrame(DataFrame const&) noexcept = delete;      // TODO figure out how to do it!?
        auto operator=(DataFrame const&) noexcept = delete; // TODO figure out how to do it!?

        DataFrame(DataFrame &&other) noexcept;
        auto operator=(DataFrame&& other) noexcept -> DataFrame&;

        ~DataFrame() noexcept;

        // ======== MANIPULATION ===========================================================================================================

        /**
         * @brief Add a new field to the dataframe.
         * TODO replace or ignore?
         *
         * @param index         field name.
         * @param defaultValue  value to put in new field values if records present.
         * @return              success indicator.
         */
        // TODO think about making it replace if already in
        auto AddField(FldIndex index, T const& defaultValue = T()) -> bool;

        auto AddFields(std::span<FldIndex const> const indices, T const& defaultValue = T()) -> bool
        {
            // add fields to index
            auto const added = fldIndex.AddMultiple(indices);
            if(not added) return false; // TODO see TODO at last return of this function

            Layout::AddColumn(data, capacity, *backingRes.get(), recsData, indices.size(), defaultValue);

            // TODO this return is bad, better to switch returning an iterator to fields added? Then user can check on != end
            return true;
        }

        // TODO think about making it replace if already in
        auto AddRecord(RecIndex index, T const& defaultValue = T()) -> bool;

        // TODO think about making it replace if already in
        auto AddRecordPopulated(RecIndex index, std::span<T const> records) -> bool;

        [[nodiscard]]
        auto GetValue(FldIndex const& field, RecIndex const& record) const -> std::optional<std::reference_wrapper<T const>>
        {
            auto const fldPos = fldIndex.Position(field);
            auto const recPos = recIndex.Position(record);

            if(fldPos && recPos)
            {
                // TODO does this work with column major?
                return recsData[*recPos, *fldPos];
                //return data[flds.size() * *recordIndex + *fieldIndex];
            }

            return std::nullopt;
        }

        [[nodiscard]]
        auto GetValue(FldIndex const& field, RecIndex const& record) -> std::optional<std::reference_wrapper<T>>
        {
            auto const fldPos = fldIndex.Position(field);
            auto const recPos = recIndex.Position(record);

            if(fldPos && recPos)
            {
                // TODO does this work with column major?
                return recsData[*recPos, *fldPos];
                //return data[flds.size() * *recordIndex + *fieldIndex];
            }

            return std::nullopt;
        }

        [[nodiscard]]
        auto operator[](FldIndex const& field, RecIndex const& record) -> T&
        {
            auto const fldPos = fldIndex.Position(field);
            auto const recPos = recIndex.Position(record);

            assert(fldPos.has_value() && recPos.has_value());
            return recsData[*recPos, *fldPos];
        }

        [[nodiscard]]
        auto operator[](FldIndex const& field, RecIndex const& record) const -> T const&
        {
            auto const fldPos = fldIndex.Position(field);
            auto const recPos = recIndex.Position(record);

            assert(fldPos.has_value() && recPos.has_value());
            return recsData[*recPos, *fldPos];
        }

        auto SetValue(FldIndex const& field, RecIndex const& record, T const& value) -> bool
        {
            if(auto get = GetValue(field, record); get.has_value())
            {
                T& g = *get;
                g = value;
                return true;
            }

            return false;
        }

        // ======== DROP ===================================================================================================================

        auto DropField(FldIndex const& index) -> bool;

        auto DropRecord(RecIndex const& index) -> bool;

        // ======== VIEWS ==================================================================================================================

        /**
         * @return      View into a field (handling layout) if field found in dataframe.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewField(FldIndex const& index) noexcept -> DFView<T>;

        /**
         * @return      View into a field (handling layout) if field found in (const) dataframe.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewField(FldIndex const& index) const noexcept -> DFView<T const>;

        /**
         * @return      View into a field (handling layout) if field found in dataframe.
         *              Row index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewFieldIndexed(FldIndex const& index) noexcept -> DFViewIndexed<T, RecIndex const>;

        /**
         * @return      View into a field (handling layout) if field found in (const) dataframe.
         *              Row index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewFieldIndexed(FldIndex const& index) const noexcept -> DFViewIndexed<T const, RecIndex const>;

        /**
         * @return      View into a record (handling layout) if record found in dataframe.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecord(RecIndex const& index) noexcept -> DFView<T>;

        /**
         * @return      View into a record (handling layout) if record found in (const) dataframe.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecord(RecIndex const& index) const noexcept -> DFView<T const>;

        /**
         * @return      View into a record (handling layout) if record found in dataframe.
         *              Field index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecordIndexed(RecIndex const& index) noexcept -> DFViewIndexed<T, FldIndex const>;

        /**
         * @return      View into a record (handling layout) if record found in (const) dataframe.
         *              Field index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecordIndexed(RecIndex const& index) const noexcept -> DFViewIndexed<T const, FldIndex const>;

        /// @brief Alternative syntax for GetField()
        auto operator|(SelectField<FldIndex> const& index) noexcept -> DFView<T> { return ViewField(index.val); }

        /// @brief Alternative syntax for GetField() const
        auto operator|(SelectField<FldIndex> const& index) const noexcept -> DFView<T const> { return ViewField(index.val); }

        /// @brief Alternative syntax for GetRecord()
        auto operator|(SelectRecord<RecIndex> const& index) noexcept -> DFView<T> { return ViewRecord(index.val); }

        /// @brief Alternative syntax for GetRecord() const
        auto operator|(SelectRecord<RecIndex> const& index) const noexcept -> DFView<T const> { return ViewRecord(index.val); }

        /// @brief Alternative syntax for GetFieldIndexed()
        auto operator|(SelectFieldIndexed<FldIndex> const& index) noexcept -> DFViewIndexed<T, RecIndex const> { return ViewFieldIndexed(index.val); }

         /// @brief Alternative syntax for GetFieldIndexed() const
        auto operator|(SelectFieldIndexed<FldIndex> const& index) const noexcept -> DFViewIndexed<T const, RecIndex const> { return ViewFieldIndexed(index.val); }

         /// @brief Alternative syntax for GetRecordIndexed()
        auto operator|(SelectRecordIndexed<RecIndex> const& index) noexcept -> DFViewIndexed<T, RecIndex const> { return ViewRecordIndexed(index.val); }

         /// @brief Alternative syntax for GetRecordIndexed() const
        auto operator|(SelectRecordIndexed<RecIndex> const& index) const noexcept -> DFViewIndexed<T const, RecIndex const> { return ViewRecordIndexed(index.val); }

        // ======== FUNCTIONAL =============================================================================================================

        /**
         * @brief       Apply a function on each value in a field.
         * @tparam Func Type of the function to apply on each value in a field.
         *
         * @param index Field index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a field.
         * @return For chaining the view the function was applied on.
         */
        template <typename Func>
        auto ForEachOnField(FldIndex const& index, Func&& func) -> DFView<T>;

        /**
         * @brief       Apply a function on each value in a field on constant DataFrame.
         * @tparam Func Type of the function to apply on each value in a field.
         *
         * @param index Field index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a field.
         * @return For chaining the view the function was applied on.
         */
        template <typename Func>
        auto ForEachOnField(FldIndex const& index, Func&& func) const -> DFView<T const>;

        /**
         * @brief       Apply a function on each value in a record.
         * @tparam Func Type of the function to apply on each value in a record.
         *
         * @param index Record index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a record.
         * @return For chaining the view the function was applied on.
         */
        template<typename Func>
        auto ForEachOnRecord(RecIndex const& index, Func&& func) -> DFView<T>;

        /**
         * @brief       Apply a function on each value in a record on constant DataFrame.
         * @tparam Func Type of the function to apply on each value in a record.
         *
         * @param index Record index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a record.
         * @return For chaining the view the function was applied on.
         */
        template<typename Func>
        auto ForEachOnRecord(RecIndex const& index, Func&& func) const -> DFView<T const>;

        // ======== PRINT ==================================================================================================================

        /**
         *  @brief Prints content of the dataframe to std-out.
         *  TODO make more usable for big dataframes + add some optional print options.
         */
        void Print() const;

        /**
         *  @brief Prints content of the dataframe to given stream.
         *  TODO make more usable for big dataframes + add some optional print options.
         *
         *  @param stream Character stream to print the values to.
         */
        void PrintTo(std::ostream& stream) const;

        /**
         *  @brief Prints content of the dataframe to given stream in a csv format.
         *  TODO make more usable for big dataframes + add some optional print options.
         *  TODO add more csv options like quoting, decimal "point" etc.
         *
         *  @param stream Character stream to print the values to.
         *  @param sep    Seperator to use in dataframe.
         */
        void PrintCSV(std::ostream& stream, char sep = ',') const;

        /// @return The size of all elements stored in the dataframe.
        [[nodiscard]] auto Size() const noexcept -> size_t { return recsData.size(); }

        /// @return A view into the current fields (indices) stored in the dataframe.
        [[nodiscard]] auto Fields() const noexcept -> Flds { return fldIndex.Keys(); }

        /// @return A view into the current records (indices) stored in the dataframe.
        [[nodiscard]] auto Records() const noexcept -> Recs { return recIndex.Keys(); }

        /// @attention It's a view so can be invalidated when adding/removing fields/records.
        /// @return    A span over the values as natural 2D view
        [[nodiscard]] [[deprecated("No Test")]] auto Values() const noexcept -> RecsData<T> { return recsData; }

        /// @attention It's a view so can be invalidated when adding/removing fields/records.
        /// @return    A view into the current records (indices) stored in the dataframe.
        [[nodiscard]] auto ValuesSpan() const noexcept -> std::span<T> { return std::span(data, recsData.size()); }

        /// @return True when no values (no records) are stored in the dataframe.
        [[nodiscard]] auto Empty() const noexcept -> bool { return recIndex.Empty(); }

        static_assert(std::is_trivially_copyable_v<Flds>, "Fields() returns this.");
    };

    // ======== CONSTRUCTION ===============================================================================================================

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame() noexcept :
        backingRes(BackingResDefault()),
        capacity(0),
        data(nullptr),
        recsData(data, 0, 0),
        fldIndex(backingRes.get(), 0),
        recIndex(backingRes.get(), 0)
    {
    }

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame(size_t const reservedValues, MemR res) noexcept :
        backingRes(std::move(res)),
        capacity(reservedValues),
        data(static_cast<T*>(backingRes->allocate(reservedValues * sizeof(T), alignof(T)))),
        recsData(data, 0, 0),
        fldIndex(backingRes.get(), 0),
        recIndex(backingRes.get(), 0)
    {
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFields(std::span<F const> const fields, size_t const reservedValues, MemR res) noexcept -> DataFrame
    {
        // TODO change this to Use function X with default value
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        auto reserve = std::max(fields.size(), reservedValues);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fields); // default value not needed if default constructable

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFields(std::initializer_list<F> fields, size_t const reservedValues, MemR res) noexcept -> DataFrame
    {
        // TODO change this to Use function X with default value
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        auto reserve = std::max(fields.size(), reservedValues);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fields); // default value not needed if default constructable

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecord(std::span<F const> const fldIndices,
                                                    std::span<R const> const recIndices,
                                                    std::span<T const> const recValues,
                                                    size_t const capacity,
                                                    MemR res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        assert(fldIndices.size() == recIndices.size());
        assert(recIndices.size() == recValues.size() || recValues.Empty());

        auto reserve = std::max(fldIndices.size() * recIndices.size(), capacity);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fldIndices); // default value not needed if default constructable

        if(not recValues.empty())
        {
            // TODO add function to add multiple records with same values
            for(auto rec : recIndices) df.AddRecordPopulated(rec, recValues);
        }
        else
        {
            for(auto rec : recIndices) df.AddRecord(rec);
        }

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecords(std::span<F const> const       fldIndices,
                                                     std::span<R const> const       recIndices,
                                                     IterableOfIterable auto const& recValues,
                                                     size_t const capacity,
                                                     MemR res) noexcept -> DataFrame
    {
        // TODO add check if IterableOfIterable stores T's
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        assert(fldIndices.size() == recIndices.size());
        assert(recIndices.size() == recValues.size() || recValues.Empty());

        auto reserve = std::max(fldIndices.size() * recIndices.size(), capacity);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fldIndices); // default value not needed if default constructable

        // TODO add function to add multiple records with same values
        auto valueIndex = 0;
        for(auto rec : recIndices) df.AddRecordPopulated(rec, recValues[valueIndex++]);

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecords(std::initializer_list<F const> const            fldIndices,
                                                     std::initializer_list<R const> const            recIndices,
                                                     std::initializer_list<std::initializer_list<T>> recValues,
                                                     size_t const capacity,
                                                     MemR res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        assert(fldIndices.size() == recIndices.size());
        assert(recIndices.size() == recValues.size());

        auto reserve = std::max(fldIndices.size() * recIndices.size(), capacity);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fldIndices); // default value not needed if default constructable

        if(recValues.size() != 0)
        {
            // TODO add function to add multiple records with same values
            auto recBegin = std::begin(recIndices);
            for(auto& vals : recValues)
            {
                // TODO add function to add multiple records with same values
                auto const& rec = *recBegin;
                df.AddRecordPopulated(rec, vals);
                ++recBegin;
            }
        }
        else
        {
            for(auto rec : recIndices) df.AddRecord(rec);
        }

        return df;
    }

    // ======== COPY, MOVE & DELETE ========================================================================================================

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame(DataFrame &&other) noexcept
    {
        if(this != &other)
        {
            backingRes = std::move(other.backingRes);
            capacity   = other.capacity;
            data       = std::move(other.data);
            recsData   = std::move(other.recsData);
            fldIndex   = std::move(other.fldIndex);
            recIndex   = std::move(other.recIndex);

            other.backingRes = nullptr;
            other.capacity   = 0;
            other.data       = nullptr;
            other.recsData   = {};
            other.fldIndex   = FldI();
            other.recIndex   = RecI();
        }
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::operator=(DataFrame&& other) noexcept -> DataFrame&
    {
        if(this != &other)
        {
            backingRes = std::move(other.backingRes);
            capacity   = other.capacity;
            data       = std::move(other.data);
            recsData   = std::move(other.recsData);
            fldIndex   = std::move(other.fldIndex);
            recIndex   = std::move(other.recIndex);

            other.backingRes = nullptr;
            other.capacity   = 0;
            other.data       = nullptr;
            other.recsData   = {};
            other.fldIndex   = FldI();
            other.recIndex   = RecI();
        }

        return *this;
    }

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::~DataFrame() noexcept
    {
        assert(not (data != nullptr && capacity == 0));
        if(data != nullptr && capacity) backingRes->deallocate(data, capacity, alignof(T));
    }

    // ======== MANIPULATION ===============================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddField(F index, T const& defaultValue) -> bool
    {
        // add field to index
        auto const added = fldIndex.Add(std::move(index));
        if(not added) return false;

        // if added to index add new columns to data
        Layout::AddColumn(data, capacity, *backingRes.get(), recsData, 1, defaultValue);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecord(R index, T const& defaultValue) -> bool
    {
        // add row to index
        auto const added = recIndex.Add(std::move(index));
        if(not added) return false;

        // add row to storage
        Layout::AddRowWithDefault(data, capacity, *backingRes.get(), recsData, defaultValue);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecordPopulated(R index, std::span<T const> records) -> bool
    {
        // this function only excepts full records
        if(records.size() != fldIndex.Size()) return false;

        // add row to index
        auto const added = recIndex.Add(std::move(index));
        if(not added) return false;

        // add row to storage
        Layout::AddRowWithValues(data, capacity, *backingRes.get(), recsData, records);
        return true;
    }

    // ======== DROP =======================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::DropField(F const& index) -> bool
    {
        // drop record from index
        auto const dropped = fldIndex.Drop(index);
        if(not dropped.has_value()) return false;

        // drop row from storage
        Layout::DropColumn(data, capacity, *backingRes.get(), recsData, *dropped);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::DropRecord(R const& index) -> bool
    {
        // drop record from index
        auto const dropped = recIndex.Drop(index);
        if(not dropped.has_value()) return false;

        // drop row from storage
        Layout::DropRow(data, capacity, *backingRes.get(), recsData, *dropped);
        return true;
    }

    // ======== VIEWS ======================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewField(F const& index) noexcept -> DFView<T>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFView<T>(); }

        return DFView<T>::FieldView(recsData, pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewField(F const& index) const noexcept -> DFView<T const>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFView<T const>(); }

        return DFView<T const>::FieldView(static_cast<RecsData<T const>>(recsData), pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewFieldIndexed(F const& index) noexcept -> DFViewIndexed<T, R const>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFViewIndexed<T, R const>(); }

        return DFViewIndexed<T, R const>::FieldView(recsData, pos.value(), recIndex.Keys());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewFieldIndexed(F const& index) const noexcept -> DFViewIndexed<T const, R const>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFViewIndexed<T const, R const>(); }

        return DFViewIndexed<T const, R const>::template FieldView(static_cast<RecsData<T const>>(recsData), pos.value(), recIndex.Keys());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecord(R const& index) noexcept -> DFView<T>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFView<T>();

        return DFView<T>::RecordView(recsData, pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecord(R const& index) const noexcept -> DFView<T const>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFView<T const>();

        return DFView<T const>::RecordView(static_cast<RecsData<T const>>(recsData), pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecordIndexed(R const& index) noexcept -> DFViewIndexed<T, F const>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFViewIndexed<T, F const>();

        return DFViewIndexed<T, F const>::template RecordView(recsData, pos.value(), fldIndex.Keys());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecordIndexed(R const& index) const noexcept -> DFViewIndexed<T const, F const>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFViewIndexed<T const, F const>();

        return DFViewIndexed<T const, F const>::template RecordView(static_cast<RecsData<T const>>(recsData), pos.value(), fldIndex.Keys());
    }

    // ======== FUNCTIONAL =================================================================================================================

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnField(F const& index, Func&& func) -> DFView<T>
    {
        auto view = ViewField(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnField(F const& index, Func&& func) const -> DFView<T const>
    {
        auto const view = ViewField(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnRecord(R const& index, Func&& func) -> DFView<T>
    {
        auto view = ViewRecord(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnRecord(R const& index, Func&& func) const -> DFView<T const>
    {
        auto view = ViewRecord(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    // ======== PRINT ======================================================================================================================

    template <typename T, typename F, typename R, typename L>
    void DataFrame<T, F, R, L>::Print() const
    {
        PrintTo(std::cout);
    }

    template <typename T, typename F, typename R, typename L>
    void DataFrame<T, F, R, L>::PrintTo(std::ostream& stream) const
    {
        auto const& extents = recsData.extents();
        auto const colCount = extents.extent(1);
        auto const rowCount = extents.extent(0);

        // get the maximum amount of fields & records stored in the dataframe
        auto const opMaxFields  = fldIndex.MaxPosition();
        auto const opMaxRecords = recIndex.MaxPosition();
        if(not opMaxFields or not opMaxRecords)
        {
            stream << "Empty\n";
            return;
        }

        auto const maxFields  = opMaxFields.value();
        auto const maxRecords = opMaxFields.value();

        // print columns/fields and record index column
        stream << std::format("{:<{}}", "Rec. Index", 15);
        for(auto pos = 0; pos <= maxFields; pos++)
        {
            auto fld = fldIndex.Key(pos);
            assert(fld.has_value());

            stream << std::format("{:<{}}", *fld, 15);
        }

        stream << '\n';

        // print records to stream
        for(auto i = 0; i <= maxRecords; ++i)
        {
            auto const recIndexStr = recIndex.Key(i);
            assert(recIndexStr.has_value());

            stream << std::format("{:<{}}", *recIndexStr, 15);
            for(size_t j = 0; j < colCount; ++j)
            {
                stream << std::format("{:<{}} ", recsData[i, j], 15);
            }
            stream << "\n";
        }
    }

    template <typename T, typename F, typename R, typename L>
    void DataFrame<T, F, R, L>::PrintCSV(std::ostream& stream, char const sep) const
    {
        auto const& extents = recsData.extents();
        auto const colCount = extents.extent(1);
        auto const rowCount = extents.extent(0);

        stream << "Row Index" << sep;
        auto fldCount = 0;
        auto const maxFields = *fldIndex.MaxPosition();
        for(auto const& fld : Fields())
        {
            stream << fld;
            if(fldCount++; fldCount != maxFields + 1) stream << sep;
            else                                      stream << '\n';
        }

        auto const recs = Records();
        for(size_t i = 0; i < rowCount; ++i)
        {
            fldCount = 0;
            stream << recs[i] << sep;
            for(size_t j = 0; j < colCount; ++j)
            {
                stream << recsData[i, j];
                if(fldCount++; fldCount != colCount) stream << sep;
                else                                 stream << '\n';
            }
        }
    }
}

#endif // LUGIZMO_DF_DATAFRAME_H
