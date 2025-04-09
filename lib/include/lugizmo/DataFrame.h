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

#include "dataframe/IndexBase.h"
#include "dataframe/IndexUnique.h"
#include "dataframe/IndexRange.h"
#include "dataframe/Selector.h"
#include "dataframe/LayoutRowMajor.h"
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
    struct DataFrame
    {
        // dataframe basic options & types
        static_assert(std::is_same_v<LayoutPolicy, std::layout_right>, "Currently only layout_right is supported");
        static_assert(std::is_default_constructible_v<T>, "Currently only default constructable values are supported");

        using Layout = std::conditional_t<std::is_same_v<LayoutPolicy, std::layout_right>, DFRowMajor<T>, void>;

    private:

        static constexpr bool IsFldSeq = DFSeqIndex<FldIndex>;
        static constexpr bool IsRecSeq = DFSeqIndex<RecIndex>;

        using DataPt = T*;                                                              // stored view into data
        using MemRsc = std::shared_ptr<std::pmr::memory_resource>;                      // backing memory resource type

        // field/record index & view types
        using FldI = std::conditional_t<IsFldSeq, FldIndex, DFUniqueIndex<FldIndex>>;   // index for field values
        using RecI = std::conditional_t<IsRecSeq, RecIndex, DFUniqueIndex<RecIndex>>;   // index for record values
        using FldT = typename FldI::KeyType;
        using RecT = typename RecI::KeyType;
        using Flds = typename FldI::KeyView;                                            // stored view into field indices
        using Recs = typename RecI::KeyView;                                            // stored view into record indices

    public:
        // returned types should be public

        using DFViewFld      = DFView<T, RecI>;
        using DFViewRec      = DFView<T, FldI>;
        using DFViewConstFld = DFView<T const, RecI>;
        using DFViewConstRec = DFView<T const, FldI>;

    private:

        template<typename MDT>
        using RecsData = std::mdspan<MDT, std::dextents<size_t, 2>, LayoutPolicy>;      // stored view into data;

        // data section
        MemRsc      backingRes;     // memory resource to use
        size_t      capacity;       // capacity of data
        DataPt      data;           // pointer to allocated memory
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
        explicit DataFrame(size_t reservedValues, MemRsc res = BackingResDefault()) noexcept;

        // ======== CONSTRUCTION FUNCTIONS =================================================================================================

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
        static auto FromFields(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::span<FldT const>> fields, size_t reservedValues = 0,
                               MemRsc res = BackingResDefault()) noexcept -> DataFrame;

        /**
         * @brief Empty Dataframe with field definitions optionally reserving memory
         *        and using a backing memory resource.
         *
         * @param fields          Initializer list of fields to initialize the vector with.
         * @param reservedValues  Optional capacity to allocate memory for.
         * @param res             Optional backing memory resource to use.
         *
         * @return Dataframe initialized with given fields.
         */
        static auto FromFields(std::initializer_list<FldT const> fields, size_t reservedValues = 0,
                               MemRsc res = BackingResDefault()) noexcept -> DataFrame requires DFValIndex<FldI>;

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
        static auto FromFieldsAndRecord(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::span<FldIndex const>> fldIndices,
                                        std::conditional_t<IsRecSeq, DFRangeIndexBounds<RecT>, std::span<RecIndex const>> recIndices,
                                        std::span<T const> recValues = {},
                                        size_t             capacity  = 0,
                                        MemRsc             res       = BackingResDefault()) noexcept -> DataFrame;

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
        static auto FromFieldsAndRecords(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::span<FldIndex const>> fldIndices,
                                         std::conditional_t<IsRecSeq, DFRangeIndexBounds<RecT>, std::span<RecIndex const>> recIndices,
                                         IterableOfIterable auto const& recValues,
                                         size_t             capacity  = 0,
                                         MemRsc             res       = BackingResDefault()) noexcept -> DataFrame;

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
        static auto FromFieldsAndRecords(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::initializer_list<FldIndex const>> fldIndices,
                                         std::conditional_t<IsRecSeq, DFRangeIndexBounds<RecT>, std::initializer_list<RecIndex const>> recIndices,
                                         std::initializer_list<std::initializer_list<T>> recValues = {},
                                         size_t capacity = 0,
                                         MemRsc res      = BackingResDefault()) noexcept -> DataFrame;

        // ======== COPY, MOVE & DELETE ====================================================================================================

        DataFrame(DataFrame const&) noexcept = delete;      // TODO or should I !?
        auto operator=(DataFrame const&) noexcept = delete; // TODO or should I !?

        DataFrame(DataFrame &&other) noexcept;
        auto operator=(DataFrame&& other) noexcept -> DataFrame&;

        ~DataFrame() noexcept;

        // ======== MANIPULATION UNIQUE INDEX ==============================================================================================

        /**
         * @brief Add a new field to the dataframe.
         * TODO replace or ignore?
         *
         * @param index         field name.
         * @param defaultValue  value to put in new field values if records present.
         * @return              success indicator.
         */
        // TODO think about making it replace if already in
        auto AddField(FldIndex index, T const& defaultValue = T()) -> bool requires DFValIndex<FldI>;

        auto AddFields(std::span<FldIndex const> const indices, T const& defaultValue = T()) -> bool requires DFValIndex<FldI>
        {
            // add fields to index
            auto const added = fldIndex.AddMultiple(indices);
            if(not added) return false; // TODO see TODO at last return of this function

            Layout::ResizeCols(data, capacity, *backingRes.get(), recsData, 0, indices.size(), defaultValue);

            // TODO this return is bad, better to switch returning an iterator to fields added? Then user can check on != end
            return true;
        }

        // TODO think about making it replace if already in
        auto AddRecord(RecIndex index, T const& defaultValue = T()) -> bool requires DFValIndex<RecI>;

        // TODO think about making it replace if already in
        auto AddRecordPopulated(RecIndex index, std::span<T const> records) -> bool requires DFValIndex<RecI>;

        // ======== MANIPULATION SEQUENCE INDEX ============================================================================================

        auto SetFieldRange(std::optional<FldT> lower, std::optional<FldT> upper, T const& defaultVal = T()) noexcept -> bool requires DFSeqIndex<FldI>;

        auto SetFieldRange(DFRangeIndexBounds<FldT>, T const& defaultVal = T()) noexcept -> bool requires DFSeqIndex<FldI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, T const& defaultVal = T()) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, T const& defaultVal = T()) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::span<T const> records) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, std::span<T const> records) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, IterableOfIterable auto const& records) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, IterableOfIterable auto const& records) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::initializer_list<std::initializer_list<T>> records) noexcept -> bool requires DFSeqIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, std::initializer_list<std::initializer_list<T>> records) noexcept -> bool requires DFSeqIndex<RecI>;

        // ======== CHECKS =========================================================================================================================================================

        template<typename C>
        [[nodiscard]] auto HasField(C const& field) const noexcept -> bool;

        template<typename C>
        [[nodiscard]] auto HasRecord(C const& record) const noexcept -> bool;

        // ======== ACCESSORS UNIQUE INDEX =========================================================================================================================================

        [[nodiscard]]
        auto GetValue(FldT const& field, RecT const& record) const -> std::optional<std::reference_wrapper<T const>>
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
        auto GetValue(FldT const& field, RecT const& record) -> std::optional<std::reference_wrapper<T>>
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
        auto operator[](FldT const& field, RecT const& record) -> T&
        {
            auto const fldPos = fldIndex.Position(field);
            auto const recPos = recIndex.Position(record);

            assert(fldPos.has_value() && recPos.has_value());
            return recsData[*recPos, *fldPos];
        }

        [[nodiscard]]
        auto operator[](FldT const& field, RecT const& record) const -> T const&
        {
            auto const fldPos = fldIndex.Position(field);
            auto const recPos = recIndex.Position(record);

            assert(fldPos.has_value() && recPos.has_value());
            return recsData[*recPos, *fldPos];
        }

        auto SetValue(FldT const& field, RecT const& record, T const& value) -> bool
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

        /**
         *  @brief Drop a field index from the dataframe.
         *  @see   SetFieldRange to shrink the dataframe when range index is used.
         *
         *  @param index the index to drop from the dataframe.
         *  @return true when index was removed otherwise such index wasn't present in the dataframe.
         */
        auto DropField(FldIndex const& index) -> bool requires DFValIndex<FldI>;

        /**
         *  @brief Drop a record index from the dataframe.
         *  @see   SetRecordRange to shrink the dataframe when range index is used.
         *
         *  @param index the index to drop from the dataframe.
         *  @return true when index was removed otherwise such index wasn't present in the dataframe.
         */
        auto DropRecord(RecIndex const& index) -> bool requires DFValIndex<RecI>;

        // ======== VIEWS ==================================================================================================================

        /**
         * @return      View into a field (handling layout) if field found in dataframe.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewField(FldIndex const& index) noexcept -> DFViewFld requires DFValIndex<RecI>;

        /**
         * @return      View into a field (handling layout) if field found in (const) dataframe.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewField(FldIndex const& index) const noexcept -> DFViewConstFld requires DFValIndex<RecI>;

        /**
         * TODO doc
         * @param index
         * @return
         */
        [[nodiscard]]
        auto ViewField(FldIndex const& index) noexcept -> DFViewFld requires DFSeqIndex<RecI>;

        /**
         * TODO doc
         * @param index
         * @return
         */
        [[nodiscard]]
        auto ViewField(FldIndex const& index) const noexcept -> DFViewConstFld requires DFSeqIndex<RecI>;

        /**
         * @return      View into a field (handling layout) if field found in dataframe.
         *              Row index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewFieldIndexed(FldIndex const& index) noexcept -> DFViewIndexed<T, RecI const> requires DFValIndex<RecI>;

        /**
         * @return      View into a field (handling layout) if field found in (const) dataframe.
         *              Row index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index field index to try getting data for.
         */
        [[nodiscard]]
        auto ViewFieldIndexed(FldIndex const& index) const noexcept -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI>;

        //[[nodiscard]]
        //auto ViewFieldIndexed(FldIndex const& index) noexcept -> DFViewIndexed<T, RecIndex const> requires DFSeqIndex<RecI>;

        /**
         * @return      View into a record (handling layout) if record found in dataframe.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecord(RecIndex const& index) noexcept -> DFViewRec requires DFValIndex<RecI>;

        /**
         * @return      View into a record (handling layout) if record found in (const) dataframe.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecord(RecIndex const& index) const noexcept -> DFViewConstRec requires DFValIndex<RecI>;

        /**
         * @return      View into a record (handling layout) if record found in dataframe.
         *              Field index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecordIndexed(RecIndex const& index) noexcept -> DFViewIndexed<T, FldI const> requires DFValIndex<RecI>;

        /**
         * @return      View into a record (handling layout) if record found in (const) dataframe.
         *              Field index is available while iterating.
         * TODO as mentioned in the README this interface is currently not 100% as expected. Changes my come.
         * @param index record index to try getting data for.
         */
        [[nodiscard]]
        auto ViewRecordIndexed(RecIndex const& index) const noexcept -> DFViewIndexed<T const, FldI const> requires DFValIndex<RecI>;

        /// @brief Alternative syntax for GetField()
        auto operator|(SelectField<FldIndex> const& index) noexcept -> DFViewFld requires DFValIndex<FldI> { return ViewField(index.val); }

        /// @brief Alternative syntax for GetField() const
        auto operator|(SelectField<FldIndex> const& index) const noexcept -> DFViewConstFld requires DFValIndex<FldI> { return ViewField(index.val); }

        /// @brief Alternative syntax for GetRecord()
        auto operator|(SelectRecord<RecIndex> const& index) noexcept -> DFViewRec requires DFValIndex<RecI> { return ViewRecord(index.val); }

        /// @brief Alternative syntax for GetRecord() const
        auto operator|(SelectRecord<RecIndex> const& index) const noexcept -> DFViewConstRec requires DFValIndex<RecI> { return ViewRecord(index.val); }

        /// @brief Alternative syntax for GetFieldIndexed()
        auto operator|(SelectFieldIndexed<FldIndex> const& index) noexcept -> DFViewIndexed<T, RecI const> requires DFValIndex<FldI> { return ViewFieldIndexed(index.val); }

         /// @brief Alternative syntax for GetFieldIndexed() const
        auto operator|(SelectFieldIndexed<FldIndex> const& index) const noexcept -> DFViewIndexed<T const, RecI const> requires DFValIndex<FldI> { return ViewFieldIndexed(index.val); }

         /// @brief Alternative syntax for GetRecordIndexed()
        auto operator|(SelectRecordIndexed<RecIndex> const& index) noexcept -> DFViewIndexed<T, RecI const> requires DFValIndex<RecI> { return ViewRecordIndexed(index.val); }

         /// @brief Alternative syntax for GetRecordIndexed() const
        auto operator|(SelectRecordIndexed<RecIndex> const& index) const noexcept -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI> { return ViewRecordIndexed(index.val); }

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
        auto ForEachOnField(FldIndex const& index, Func&& func) -> DFViewFld requires DFValIndex<FldI>;

        /**
         * @brief       Apply a function on each value in a field on constant DataFrame.
         * @tparam Func Type of the function to apply on each value in a field.
         *
         * @param index Field index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a field.
         * @return For chaining the view the function was applied on.
         */
        template <typename Func>
        auto ForEachOnField(FldIndex const& index, Func&& func) const -> DFViewConstFld requires DFValIndex<FldI>;

        /**
         * @brief       Apply a function on each value in a record.
         * @tparam Func Type of the function to apply on each value in a record.
         *
         * @param index Record index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a record.
         * @return For chaining the view the function was applied on.
         */
        template<typename Func>
        auto ForEachOnRecord(RecIndex const& index, Func&& func) -> DFViewRec requires DFValIndex<RecI>;

        /**
         * @brief       Apply a function on each value in a record on constant DataFrame.
         * @tparam Func Type of the function to apply on each value in a record.
         *
         * @param index Record index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a record.
         * @return For chaining the view the function was applied on.
         */
        template<typename Func>
        auto ForEachOnRecord(RecIndex const& index, Func&& func) const -> DFViewConstRec requires DFValIndex<RecI>;

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

        /// @return The count of fields.
        [[nodiscard]] auto FieldSize() const noexcept -> size_t { return fldIndex.Size(); }

        /// @return The count of records.
        [[nodiscard]] auto RecordSize() const noexcept -> size_t { return recIndex.Size(); }

        /// @attention It's a view so can be invalidated when adding/removing fields/records.
        /// @return A view into the current fields (indices) stored in the dataframe.
        [[nodiscard]] auto Fields() const noexcept -> Flds { return fldIndex.Keys(); }

        /// @attention It's a view so can be invalidated when adding/removing fields/records.
        /// @return A view into the current records (indices) stored in the dataframe.
        [[nodiscard]] auto Records() const noexcept -> Recs { return recIndex.Keys(); }

        /// @attention It's a pointer so can be invalidated when adding/removing fields/records.
        ///            Only keep this pointer alive as long as this dataframe wasn't mutated.
        /// @return    Pointer to the currently stored dataframe->data.
        [[nodiscard]] auto Data() const noexcept -> T const* { return data; }

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
        recsData(data, 0, 0)
    {
        if constexpr (DFValIndex<FldI>) fldIndex = FldI{backingRes.get(), 0};
        else                              fldIndex = FldI{};

        if constexpr (DFValIndex<RecI>) recIndex = RecI{backingRes.get(), 0};
        else                              recIndex = RecI{};
    }

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame(size_t const reservedValues, MemRsc res) noexcept :
        backingRes(std::move(res)),
        capacity(reservedValues),
        data(static_cast<T*>(backingRes->allocate(reservedValues * sizeof(T), alignof(T)))),
        recsData(data, 0, 0)
    {
        if constexpr (DFValIndex<FldI>) fldIndex = FldI{backingRes.get(), 0};
        else                              fldIndex = FldI{};

        if constexpr (DFValIndex<RecI>) recIndex = RecI{backingRes.get(), 0};
        else                              recIndex = RecI{};
    }

    // ======== CONSTRUCTION FUNCTIONS =====================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFields(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::span<FldT const>> const fields, size_t const reservedValues, MemRsc res) noexcept -> DataFrame
    {
        // TODO change this to Use function X with default value
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        auto reserve = std::max<size_t>(fields.size(), reservedValues);
        auto df = DataFrame(reserve, std::move(res));

        // add new fields to empty df
        if constexpr(IsFldSeq) df.SetFieldRange(fields.lower, fields.upper, T());
        else                   df.AddFields(fields);

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFields(std::initializer_list<FldT const> const fields, size_t const reservedValues, MemRsc res) noexcept -> DataFrame requires DFValIndex<FldI>
    {
        // TODO change this to Use function X with default value
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        auto reserve = std::max(fields.size(), reservedValues);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fields);

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecord(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::span<F const>> const fldIndices,
                                                    std::conditional_t<IsRecSeq, DFRangeIndexBounds<RecT>, std::span<R const>> const recIndices,
                                                    std::span<T const> const recValues,
                                                    size_t const             capacity,
                                                    MemRsc                   res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        assert(fldIndices.size() == recIndices.size());
        assert(recIndices.size() == recValues.size() || recValues.empty());

        auto reserve = std::max<size_t>(fldIndices.size() * recIndices.size(), capacity);
        auto df      = DataFrame(reserve, std::move(res));

        // add fields to dataframe
        if constexpr(IsFldSeq) df.SetFieldRange(fldIndices.lower, fldIndices.upper);
        else                   df.AddFields(fldIndices);

        // populate records in dataframe
        if(not recValues.empty())
        {
            if constexpr(IsRecSeq) df.SetRecordRange(recIndices.lower, recIndices.upper, recValues);
            else                   for(auto rec : recIndices) df.AddRecordPopulated(rec, recValues); // TODO add function to add multiple records with same values
        }
        else
        {
            if constexpr(IsRecSeq) df.SetRecordRange(recIndices.lower, recIndices.upper);
            else                   for(auto rec : recIndices) df.AddRecord(rec);                     // TODO add function to add multiple records with same values
        }

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecords(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::span<F const>> const fldIndices,
                                                    std::conditional_t<IsRecSeq, DFRangeIndexBounds<RecT>, std::span<R const>> const recIndices,
                                                    IterableOfIterable auto const& recValues,
                                                    size_t const                   capacity,
                                                    MemRsc                         res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        assert(fldIndices.size() == recIndices.size());
        assert(recIndices.size() == recValues.size() || recValues.empty());

        auto reserve = std::max<size_t>(fldIndices.size() * recIndices.size(), capacity);
        auto df      = DataFrame(reserve, std::move(res));

        // add fields to dataframe
        if constexpr(IsFldSeq) df.SetFieldRange(fldIndices.lower, fldIndices.upper);
        else                   df.AddFields(fldIndices);

        // populate records in dataframe
        auto valueIndex = 0;
        if constexpr(IsRecSeq) df.SetRecordRange(recIndices.lower, recIndices.upper, recValues);
        else                   for(auto rec : recIndices) df.AddRecordPopulated(rec, recValues[valueIndex++]); // TODO add function to init. in "one go"

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecords(std::conditional_t<IsFldSeq, DFRangeIndexBounds<FldT>, std::initializer_list<F const>> const fldIndices,
                                                     std::conditional_t<IsRecSeq, DFRangeIndexBounds<RecT>, std::initializer_list<R const>> const recIndices,
                                                     std::initializer_list<std::initializer_list<T>> const recValues,
                                                     size_t const capacity,
                                                     MemRsc       res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        assert(fldIndices.size() == recIndices.size());
        assert(recIndices.size() == recValues.size() || recValues.size() == 0);

        auto reserve = std::max<size_t>(fldIndices.size() * recIndices.size(), capacity);
        auto df      = DataFrame(reserve, std::move(res));

        // add fields to dataframe
        if constexpr(IsFldSeq) df.SetFieldRange(fldIndices.lower, fldIndices.upper);
        else                   df.AddFields(fldIndices);

        // populate records in dataframe
        if(recValues.size() != 0)
        {
            if constexpr(IsRecSeq)
            {
                df.SetRecordRange(recIndices.lower, recIndices.upper, recValues);
            }
            else
            {
                // TODO add function to add multiple records with same values
                auto recBegin = std::begin(recIndices);
                for(auto& vals : recValues)
                {
                    auto const& rec = *recBegin;
                    df.AddRecordPopulated(rec, vals);

                    ++recBegin;
                }
            }

        }
        else
        {
            if constexpr(IsRecSeq) df.SetRecordRange(recIndices.lower, recIndices.upper);
            else                   for(auto rec : recIndices) df.AddRecord(rec);                     // TODO add function to add multiple records with same values
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

    // ======== MANIPULATION UNIQUE INDEX ==================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddField(F index, T const& defaultValue) -> bool requires DFValIndex<FldI>
    {
        // add field to index
        auto const added = fldIndex.Add(std::move(index));
        if(not added) return false;

        // if added to index add new columns to data
        Layout::ResizeCols(data, capacity, *backingRes.get(), recsData, 0, 1, defaultValue);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecord(R index, T const& defaultValue) -> bool requires DFValIndex<RecI>
    {
        // add row to index
        auto const added = recIndex.Add(std::move(index));
        if(not added) return false;

        // add row to storage
        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, 0, 1, defaultValue);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecordPopulated(R index, std::span<T const> records) -> bool requires DFValIndex<RecI>
    {
        // this function only excepts full records
        if(records.size() != fldIndex.Size()) return false;

        // add row to index
        auto const added = recIndex.Add(std::move(index));
        if(not added) return false;

        // add row to storage
        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, 0, 1, records);
        return true;
    }

    // ======== MANIPULATION SEQUENCE INDEX ================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetFieldRange(std::optional<FldT> const lower, std::optional<FldT> const upper, T const& defaultVal) noexcept -> bool requires DFSeqIndex<FldI>
    {
        // compute and adjust index
        auto [lowerChange, upperChange] = fldIndex.SetLowerUpperBound(lower, upper);

        // check if both bounds are not changed
        if(not lowerChange and not upperChange) return false;

        // if lower bound is
        Layout::ResizeCols(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, defaultVal);

        return true;
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetFieldRange(DFRangeIndexBounds<FldT> bounds, T const& defaultVal) noexcept -> bool requires DFSeqIndex<FldI>
    {
        return SetFieldRange(bounds.lower, bounds.upper, defaultVal);
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower, std::optional<RecT> const upper, T const& defaultVal) noexcept -> bool requires DFSeqIndex<RecI>
    {
        // compute and adjust index
        auto [lowerChange, upperChange] = recIndex.SetLowerUpperBound(lower, upper);

        // check if both bounds are not changed
        if(not lowerChange and not upperChange) return false;

        // if lower bound is
        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, defaultVal);
        return true;
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, T const& defaultVal) noexcept -> bool requires DFSeqIndex<RecI>
    {
        return SetRecordRange(bounds.lower, bounds.upper, defaultVal);
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower, std::optional<RecT> const upper, std::span<T const> const records) noexcept -> bool requires DFSeqIndex<RecI>
    {
        // store current bounds in case need to recover
        auto const currentLower = recIndex.LowerBound();
        auto const currentUpper = recIndex.UpperBound();

        // compute and adjust index
        auto [lowerChange, upperChange] = recIndex.SetLowerUpperBound(lower, upper);

        // check if both bounds are not changed
        if(not lowerChange and not upperChange) return false;

        // check if one of new bounds is not valid
        // and restore previous state
        if(records.size() != fldIndex.Size())
        {
            recIndex.SetLowerBound(currentLower);
            recIndex.SetUpperBound(currentUpper);
            return false;
        }

        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, records);
        return true;
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, std::span<T const> const records) noexcept -> bool requires DFSeqIndex<RecI>
    {
        return SetRecordRange(bounds.lower, bounds.upper, records);
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower,
                                               std::optional<RecT> const upper,
                                               IterableOfIterable auto const& records) noexcept -> bool requires DFSeqIndex<RecI>
    {
        // store current bounds in case need to recover
        auto const currentLower = recIndex.LowerBound();
        auto const currentUpper = recIndex.UpperBound();

        // compute and adjust index
        auto [lowerChange, upperChange] = recIndex.SetLowerUpperBound(lower, upper);

        // check if both bounds are not changed
        if(not lowerChange and not upperChange) return false;

        // check if one of new bounds is not valid
        // and restore previous state
        if(records.size() != fldIndex.Size())
        {
            recIndex.SetLowerBound(currentLower);
            recIndex.SetUpperBound(currentUpper);
            return false;
        }

        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, records);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, IterableOfIterable auto const& records) noexcept -> bool requires DFSeqIndex<RecI>
    {
        return SetRecordRange(bounds.lower, bounds.upper, records);
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower,
                                               std::optional<RecT> const upper,
                                               std::initializer_list<std::initializer_list<T>> const records) noexcept -> bool requires DFSeqIndex<RecI>
    {
        // store current bounds in case need to recover
        auto const currentLower = recIndex.LowerBound();
        auto const currentUpper = recIndex.UpperBound();

        // compute and adjust index
        auto [lowerChange, upperChange] = recIndex.SetLowerUpperBound(lower, upper);

        // check if both bounds are not changed
        if(not lowerChange and not upperChange) return false;

        // check if one of new bounds is not valid
        // and restore previous state
        if(records.size() != recIndex.Size())
        {
            recIndex.SetLowerBound(currentLower);
            recIndex.SetUpperBound(currentUpper);
            return false;
        }

        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, records);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, std::initializer_list<std::initializer_list<T>> const records) noexcept -> bool
        requires DFSeqIndex<RecI>
    {
        return SetRecordRange(bounds.lower, bounds.upper, records);
    }

    // ======== CHECKS =============================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    template <typename C>
    auto DataFrame<T, F,  R, L>::HasField(C const& field) const noexcept -> bool
    {
        static_assert(ComparableType<FldT, C>, "Given field is not comparable with type of fields!");
        return fldIndex.Has(field);
    }

    template <typename T, typename F, typename R, typename L>
    template<typename C>
    auto DataFrame<T, F, R, L>::HasRecord(C const& record) const noexcept -> bool
    {
        static_assert(ComparableType<RecT, C>, "Given record is not comparable with type of records!");
        return recIndex.Has(record);
    }

    // ======== DROP ===============================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::DropField(F const& index) -> bool requires DFValIndex<FldI>
    {
        // drop record from index
        auto const dropped = fldIndex.Drop(index);
        if(not dropped.has_value()) return false;

        // drop row from storage
        Layout::DropColumn(data, capacity, *backingRes.get(), recsData, *dropped);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::DropRecord(R const& index) -> bool requires DFValIndex<RecI>
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
    auto DataFrame<T, F, R, L>::ViewField(F const& index) noexcept -> DFViewFld requires DFValIndex<RecI>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFViewFld(); }

        return DFViewFld::FieldView(recsData, &recIndex, pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewField(F const& index) const noexcept -> DFViewConstFld requires DFValIndex<RecI>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFViewConstFld(); }

        return DFViewConstFld::FieldView(static_cast<RecsData<T const>>(recsData), &recIndex, pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewField(F const& index) noexcept -> DFViewFld requires DFSeqIndex<RecI>
    {
        auto const pos = fldIndex.Position(index);
        if(!pos.has_value()) { return DFViewFld(); }

        return DFViewFld::FieldView(recsData, &recIndex, *pos, recIndex.LowerBoundPosition(), recIndex.UpperBoundPosition());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewField(F const& index) const noexcept -> DFViewConstFld requires DFSeqIndex<RecI>
    {
        auto const pos = fldIndex.Position(index);
        if(!pos.has_value()) { return DFViewConstFld(); }

        return DFViewConstFld::FieldView(static_cast<RecsData<T const>>(recsData), &recIndex, *pos, recIndex.LowerBoundPosition(), recIndex.UpperBoundPosition());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewFieldIndexed(F const& index) noexcept -> DFViewIndexed<T, RecI const> requires DFValIndex<RecI>
    {
        auto const pos = fldIndex.Position(index);
        if (!pos.has_value()) { return DFViewIndexed<T, RecI const>(); }

        return DFViewIndexed<T, RecI const>::FieldView(recsData, &recIndex, pos.value(), recIndex.Keys());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewFieldIndexed(F const& index) const noexcept -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI>
    {
        auto const pos = fldIndex.Position(index);
        if(!pos.has_value()) { return DFViewIndexed<T const, RecI const>(); }

        return DFViewIndexed<T const, RecI const>::template FieldView<>(static_cast<RecsData<T const>>(recsData), &recIndex, pos.value(), recIndex.Keys());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecord(R const& index) noexcept -> DFViewRec requires DFValIndex<RecI>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFViewRec();

        return DFViewRec::RecordView(recsData, &fldIndex, pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecord(R const& index) const noexcept -> DFViewConstRec requires DFValIndex<RecI>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFViewConstRec();

        return DFViewConstRec::RecordView(static_cast<RecsData<T const>>(recsData), &fldIndex, pos.value());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecordIndexed(R const& index) noexcept -> DFViewIndexed<T, FldI const> requires DFValIndex<RecI>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFViewIndexed<T, FldI const>();

        return DFViewIndexed<T, FldI const>::template RecordView<>(recsData, &fldIndex, pos.value(), fldIndex.Keys());
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::ViewRecordIndexed(R const& index) const noexcept -> DFViewIndexed<T const, FldI const> requires DFValIndex<RecI>
    {
        auto pos = recIndex.Position(index);
        if(not pos.has_value()) return DFViewIndexed<T const, FldI const>();

        return DFViewIndexed<T const, FldI const>::template RecordView<>(static_cast<RecsData<T const>>(recsData), &fldIndex, pos.value(), fldIndex.Keys());
    }

    // ======== FUNCTIONAL =================================================================================================================

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnField(F const& index, Func&& func) -> DFViewFld requires DFValIndex<FldI>
    {
        auto view = ViewField(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnField(F const& index, Func&& func) const -> DFViewConstFld requires DFValIndex<FldI>
    {
        auto const view = ViewField(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnRecord(R const& index, Func&& func) -> DFViewRec requires DFValIndex<RecI>
    {
        auto view = ViewRecord(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnRecord(R const& index, Func&& func) const -> DFViewConstRec requires DFValIndex<RecI>
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
        auto const& extents   = recsData.extents();
        size_t const colCount = extents.extent(1);
        size_t const rowCount = extents.extent(0);

        // Print header: first column "Rec. Index"
        stream << std::format("{:<15}", "Rec. Index");

        // Print field header using independent branch for field index type.
        if constexpr (IsFldSeq)
        {
            // For sequential (range) field indices, use the logical range.
            ssize_t const fldLower = fldIndex.LowerBound();
            ssize_t const fldUpper = fldLower + static_cast<ssize_t>(colCount);
            for (ssize_t pos = fldLower; pos < fldUpper; ++pos)
            {
                stream << std::format("{:<15}", pos);
            }
        }
        else
        {
            // For non-sequential field indices, use the key for each field.
            for (size_t pos = 0; pos < colCount; ++pos)
            {
                auto fld = fldIndex.Key(pos);
                assert(fld.has_value());
                stream << std::format("{:<15}", *fld);
            }
        }
        stream << "\n";

        // Print records (rows) using independent branch for record index type.
        if constexpr (IsRecSeq)
        {
            // When records are sequential, use their logical lower bound.
            if (recIndex.Empty())
            {
                stream << "Empty\n";
                return;
            }
            ssize_t const recLower = recIndex.LowerBound();
            ssize_t const recUpper = recLower + static_cast<ssize_t>(rowCount);
            for (ssize_t i = recLower; i < recUpper; ++i)
            {
                // Print record header (logical record index)
                stream << std::format("{:<15}", i);

                // For the columns, branch on field index type.
                if constexpr (IsFldSeq)
                {
                    ssize_t const fldLower = fldIndex.LowerBound();
                    ssize_t const fldUpper = fldLower + static_cast<ssize_t>(colCount);
                    for (ssize_t j = fldLower; j < fldUpper; ++j)
                    {
                        // The underlying mdspan is 0-based, so subtract the lower bound.
                        stream << std::format("{:<15} ", recsData[static_cast<size_t>(i - recLower), static_cast<size_t>(j - fldLower)]);
                    }
                }
                else
                {
                    // Non-sequential field indices: the mdspan columns remain 0-based.
                    for (size_t j = 0; j < colCount; ++j)
                    {
                        stream << std::format("{:<15} ", recsData[static_cast<size_t>(i - recLower), j]);
                    }
                }
                stream << "\n";
            }
        }
        else
        {
            // When record indices are non-sequential, use keys.
            for (size_t i = 0; i < rowCount; ++i)
            {
                auto recKey = recIndex.Key(i);
                assert(recKey.has_value());
                stream << std::format("{:<15}", *recKey);

                if constexpr (IsFldSeq)
                {
                    ssize_t fldLower = fldIndex.LowerBound();
                    ssize_t fldUpper = fldLower + static_cast<ssize_t>(colCount);
                    for (ssize_t j = fldLower; j < fldUpper; ++j)
                    {
                        stream << std::format("{:<15} ", recsData[i, static_cast<size_t>(j - fldLower)]);
                    }
                }
                else
                {
                    for (size_t j = 0; j < colCount; ++j)
                    {
                        stream << std::format("{:<15} ", recsData[i, j]);
                    }
                }
                stream << "\n";
            }
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
