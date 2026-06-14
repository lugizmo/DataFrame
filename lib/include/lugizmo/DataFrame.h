// Filename: DataFrame.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_DATAFRAME_H
#define LUGIZMO_DF_DATAFRAME_H

#include <cstddef>
#include <concepts>
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

#include "Assert.h"
#include "memory/Memory.h"

#include "support/Compiler.h"
#include "meta/DeducingThis.h"
#include "container/Concepts.h"
#include "container/OptionalRef.h"

#include "dataframe/IndexBase.h"
#include "dataframe/IndexUnique.h"
#include "dataframe/IndexRange.h"
#include "dataframe/Selector.h"
#include "dataframe/LayoutRowMajor.h"
#include "dataframe/View.h"
#include "dataframe/ViewIndexed.h"

namespace lugizmo {

    using RowMajor = std::layout_right;
    using ColMajor = std::layout_left;

    /**
     *  TODO extend/update documentation.
     *  @brief Dataframe of a single type using indices to access individual values
     *         or views into the data. The dataframe uses contiguous data as backend.
     *
     *  @tparam T Type stored in Dataframe.
     *  @tparam F Index type of field (column)
     *  @tparam R Index type of record (row)
     *  @tparam L Underlying layout of storage to use.
     */
    template<typename T, typename F, typename R, typename L = RowMajor>
    struct DataFrame
    {
        // dataframe basic options and types
        static_assert(std::is_same_v<L, RowMajor>, "Currently only layout_right is supported");
        static_assert(std::is_default_constructible_v<T>, "Currently only default constructable values are supported");
        static_assert(std::is_copy_constructible_v<T>, "DataFrame currently requires copy-constructible element types.");
        static_assert(std::is_copy_assignable_v<T>, "DataFrame currently requires copy-assignable element types.");

        static_assert(DFRngIndex<F> || requires(F const& field) {{ std::hash<F>{}(field) } -> std::convertible_to<std::size_t>;},
                     "DataFrame requires std::hash support for value-indexed field types.");

        static_assert(DFRngIndex<R> || requires(R const& record) {{ std::hash<R>{}(record) } -> std::convertible_to<std::size_t>;},
                     "DataFrame requires std::hash support for value-indexed record types.");

        using Layout          = std::conditional_t<std::is_same_v<L, RowMajor>, DFRowMajor<T>, void>;
        using MemRsc          = std::shared_ptr<std::pmr::memory_resource>;
        using DataMatrix      = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, L>;
        using ConstDataMatrix = std::mdspan<T const, std::dextents<std::ptrdiff_t, 2>, L>;

    private:

        static constexpr bool IS_FLD_RNG = DFRngIndex<F>;
        static constexpr bool IS_REC_RNG = DFRngIndex<R>;

        // field/record index and view types
        using FldI = std::conditional_t<IS_FLD_RNG, F, DFUniqueIndex<F>>;   // index for field values
        using RecI = std::conditional_t<IS_REC_RNG, R, DFUniqueIndex<R>>;   // index for record values

    public:

        // ======== TYPES ==========================================================================================================================================================

        using FldT = FldI::KeyType;     // underlying type of field index
        using RecT = RecI::KeyType;     // underlying type of record index
        using Flds = FldI::KeyView;     // stored view into field indices  TODO introduce IndexView?
        using Recs = RecI::KeyView;     // stored view into record indices TODO introduce IndexView?

        /// @brief Value type generated from self distinguish const category.
        template<typename Self>
        using Value = meta::ThisValueT<Self, T>;

        /// @brief     Optional value type generated from self distinguish const category.
        /// @attention The stored reference is required to be valid if the optional has a value,
        ///            but mutating the dataframe shape will invalidate the reference!
        template<typename Self>
        using OptValue = OptionalRef<Value<Self>>;

        /// @brief View over values either on record or a field.
        template<typename Self, typename Index>
        using ValueView = DFView<Value<Self>, Index>;

        /// @brief View over values with their index value either on record or a field.
        ///        If viewing a field, you get record indices and vise versa.
        template<typename Self, typename Index>
        using IndexedValueView = DFViewIndexed<Value<Self>, Index const>;

        // ======== CONSTRUCTION ===================================================================================================================================================

        /**
         *  @brief   Default constructor that creates an empty
         *           Dataframe that does not allocate any data
         *           (except for the essentials).
         *  @details Uses the default memory resource of the system.
         */
        explicit DataFrame(MemRsc res = internal::BackingResDefault()) noexcept;

        /**
         * @brief Empty Dataframe optionally reserving memory and using a backing
         *        memory resource.
         *
         * @param reservedValues Number of values to reserve. Number should be neither
         *                       record nor column count but the multiple of both.
         * @param res            Backing memory resource to use (defaults to system-default).
         */
        explicit DataFrame(size_t reservedValues, MemRsc res = internal::BackingResDefault()) noexcept;

        // ======== CONSTRUCTION FUNCTIONS =========================================================================================================================================

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
        static auto FromFields(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::span<FldT const>> fields, size_t reservedValues = 0,
                               MemRsc res = internal::BackingResDefault()) noexcept -> DataFrame;

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
                               MemRsc res = internal::BackingResDefault()) noexcept -> DataFrame requires DFUnqIndex<FldI>;

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
        static auto FromFieldsAndRecord(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::span<F const>> fldIndices,
                                        std::conditional_t<IS_REC_RNG, DFRangeIndexBounds<RecT>, std::span<R const>> recIndices,
                                        std::span<T const> recValues = {},
                                        size_t             capacity  = 0,
                                        MemRsc             res       = internal::BackingResDefault()) noexcept -> DataFrame;

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
        static auto FromFieldsAndRecords(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::span<F const>> fldIndices,
                                         std::conditional_t<IS_REC_RNG, DFRangeIndexBounds<RecT>, std::span<R const>> recIndices,
                                         IterableOfIterable auto const& recValues,
                                         size_t             capacity  = 0,
                                         MemRsc             res       = internal::BackingResDefault()) noexcept -> DataFrame;

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
        static auto FromFieldsAndRecords(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::initializer_list<F const>> fldIndices,
                                         std::conditional_t<IS_REC_RNG, DFRangeIndexBounds<RecT>, std::initializer_list<R const>> recIndices,
                                         std::initializer_list<std::initializer_list<T>> recValues = {},
                                         size_t capacity = 0,
                                         MemRsc res      = internal::BackingResDefault()) noexcept -> DataFrame;

        // ======== COPY, MOVE & DELETE ============================================================================================================================================

        DataFrame(DataFrame const&) noexcept;
        auto operator=(DataFrame const&) noexcept -> DataFrame&;

        DataFrame(DataFrame &&other) noexcept;
        auto operator=(DataFrame&& other) noexcept -> DataFrame&;

        ~DataFrame() noexcept;

        // ======== MANIPULATION UNIQUE INDEX ======================================================================================================================================

        /**
         * @brief Add a new field to the dataframe.
         *        If the field already exists, nothing happens.
         *
         * @param index         field name.
         * @param defaultValue  value to put in new field values if records present.
         * @return              true if the field wasn't present before.
         */
        auto AddField(F index, T const& defaultValue = T{}) -> bool requires DFUnqIndex<FldI>;

        /**
         * @brief Adding new fields to the dataframe.
         *        Fields already in are skipped.
         * @param indices      field names to add.
         * @param defaultValue value to put in new fields if records are present.
         * @return             count of fields added.
         */
        auto AddFields(std::span<F const> indices, T const& defaultValue = T{}) -> std::size_t requires DFUnqIndex<FldI>;

        /**
         * @brief Add a new record to the dataframe.
         *        If the record already exists, nothing happens.
         * @param index        record name.
         * @param defaultValue value to put in all fields.
         * @return             true if the record wasn't present before.
         */
        auto AddRecord(R index, T const& defaultValue = T{}) -> bool requires DFUnqIndex<RecI>;

        /**
         * @brief Add new records to the dataframe.
         *        If the record already exists, nothing happens.
         * @param indices      record names to add.
         * @param defaultValue value to put in all fields.
         * @return             count of records added.
         */
        auto AddRecords(std::span<R const> indices, T const& defaultValue = T{}) -> std::size_t requires DFUnqIndex<RecI>;

        // TODO think about making it replace if already in
        // TODO add tests, do not use until now
        auto AddRecordPopulated(R index, std::span<T const> records) -> bool requires DFUnqIndex<RecI>;

        // ======== MANIPULATION RANGE INDEX =======================================================================================================================================

        auto SetFieldRange(std::optional<FldT> lower, std::optional<FldT> upper, T const& defaultVal = T{}) noexcept -> bool requires DFRngIndex<FldI>;

        auto SetFieldRange(DFRangeIndexBounds<FldT>, T const& defaultVal = T{}) noexcept -> bool requires DFRngIndex<FldI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, T const& defaultVal = T{}) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, T const& defaultVal = T{}) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::span<T const> records) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, std::span<T const> records) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, IterableOfIterable auto const& records) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, IterableOfIterable auto const& records) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::initializer_list<std::initializer_list<T>> records) noexcept -> bool requires DFRngIndex<RecI>;

        auto SetRecordRange(DFRangeIndexBounds<RecT>, std::initializer_list<std::initializer_list<T>> records) noexcept -> bool requires DFRngIndex<RecI>;

        // ======== CHECKS =========================================================================================================================================================

        /**
         *  @param field The field to check for.
         *  @return True if the dataframe contains the given field.
         */
        template<typename C>
        [[nodiscard]] auto HasField(C const& field) const noexcept -> bool;

        /**
         *  @param record The record to check for.
         *  @return True if the dataframe contains the given record.
         */
        template<typename C>
        [[nodiscard]] auto HasRecord(C const& record) const noexcept -> bool;

        // ======== GETTERS ========================================================================================================================================================

        /**
         * TODO doc
         * @param field
         * @param record
         * @return
         */
        template<typename FieldLookup, typename RecordLookup>
        [[nodiscard]]
        auto GetValue(this auto& self, FieldLookup const& field, RecordLookup const& record) -> OptValue<decltype(self)>;

        /**
         * TODO doc
         * @param field
         * @param record
         * @return
         */
        template<typename FieldLookup, typename RecordLookup>
        [[nodiscard]]
        auto operator[](this auto& self, FieldLookup const& field, RecordLookup const& record) -> Value<decltype(self)>&;

        // ======== SETTERS ========================================================================================================================================================

        /**
         * @brief Replace the value in the dataframe for field/record
         *        if the value already exists else does nothing.
         *
         * @param field  The field name to check for.
         * @param record The record name to check for.
         * @param value  The value to replace with.
         *
         * @return       True if value was replaced.
         */
        template<typename U>
        requires std::constructible_from<T, U&&>
        auto AssignValue(FldT const& field, RecT const& record, U&& value) -> bool;

        /**
         * @brief Replace all values in a field if the field exists and the value
         *        count matches the record count.
         *
         * @note Try to use U == T as if U is a different type, it gets constructed
         *       and this can prevent mem-cpy.
         *
         * @param field  The field to replace.
         * @param values The values to assign. Must have size RecordSize().
         *
         * @return       True if the field existed and all values were assigned.
         */
        template<typename U, std::size_t Extent>
        requires std::constructible_from<T, U const&>
        auto AssignFieldValues(FldT const& field, std::span<U const, Extent> values) -> bool;

        /**
         * @note    Internally still const/no mutation.
         * @copydoc AssignFieldValues(FldT const& field, std::span<U const, Extent> values).
         */
        template<typename U, std::size_t Extent>
        requires std::constructible_from<T, U const&>
        auto AssignFieldValues(FldT const& field, std::span<U, Extent> values) -> bool;

        /**
         * @brief Replace all values in a record if the record exists and the value
         *        count matches the field count.
         *
         * @param record The record to replace.
         * @param values The values to assign. Must have size FieldSize().
         *
         * @return       True if the record existed and all values were assigned.
         */
        template<typename U, std::size_t Extent>
        requires std::constructible_from<T, U const&>
        auto AssignRecordValues(RecT const& record, std::span<U const, Extent> values) -> bool;

        /**
         * @note    Internally still const/no mutation.
         * @copydoc AssignRecordValues(RecT const& record, std::span<U const, Extent> values).
         */
        template<typename U, std::size_t Extent>
        requires std::constructible_from<T, U const&>
        auto AssignRecordValues(RecT const& record, std::span<U, Extent> values) -> bool;

        /**
        * @brief Adds the value or replaces it if already present.
        *        If field and/or record are not present yet, they are added and the value is set.
        *        If already present value gets replaced.
        *
        * @param field  The field name to insert if not present yet.
        * @param record The record name to insert if not present yet.
        * @param value  The value to assign.
        */
        template<typename U>
        requires std::constructible_from<T, U&&>
        void UpsertValue(FldT const& field, RecT const& record, U&& value) requires DFUnqIndices<FldI, RecI>;

        // ======== DROP ===========================================================================================================================================================

        /**
         *  @brief Drop a field index from the dataframe.
         *  @see   SetFieldRange to shrink the dataframe when the range index is used.
         *
         *  @param index The index to drop from the dataframe.
         *  @return      True, when the index was removed otherwise, such an index wasn't present in the dataframe.
         */
        auto DropField(F const& index) -> bool requires DFUnqIndex<FldI>;

        /**
         *  @brief Drop a record index from the dataframe.
         *  @see   SetRecordRange to shrink the dataframe when the range index is used.
         *
         *  @param index The index to drop from the dataframe.
         *  @return      True, when the index was removed otherwise, such an index wasn't present in the dataframe.
         */
        auto DropRecord(R const& index) -> bool requires DFUnqIndex<RecI>;

        // ======== SORT ===========================================================================================================================================================

        /**
         *  @brief Sort field indices and reorder the stored columns to match the new order.
         *  @param comp comparator used to order fields.
         */
        template<typename Compare = std::less<FldT>>
        void SortFields(Compare comp = {});

        /**
         *  @brief Sort record indices and reorder the stored rows to match the new order.
         *  @param comp comparator used to order records.
         */
        template<typename Compare = std::less<RecT>>
        void SortRecords(Compare comp = {});

        // ======== VIEWS ==========================================================================================================================================================

        /**
         * @return      View into a field (handling layout) if field found in dataframe.
         * @param index field index to try getting data for.
         */
        template<typename FieldLookup>
        [[nodiscard]]
        auto ViewField(this auto& self, FieldLookup const& index) noexcept -> ValueView<decltype(self), RecI>;

        /**
         * @return      View into a field (handling layout) if field found in dataframe.
         *              Row index is available while iterating.
         * @param index field index to try getting data for.
         */
        template<typename FieldLookup>
        [[nodiscard]]
        auto ViewFieldIndexed(this auto& self, FieldLookup const& index) noexcept -> IndexedValueView<decltype(self), RecI>;

        /**
         * @return      View into a record (handling layout) if record found in dataframe.
         * @param index record index to try getting data for.
         */
        template<typename RecordLookup>
        [[nodiscard]]
        auto ViewRecord(this auto& self, RecordLookup const& index) noexcept -> ValueView<decltype(self), FldI> requires DFUnqIndex<FldI>;

        /**
         * @return      View into a record (handling layout) if record found in dataframe.
         *              Field index is available while iterating.
         * @param index record index to try getting data for.
         */
        template<typename RecordLookup>
        [[nodiscard]]
        auto ViewRecordIndexed(this auto& self, RecordLookup const& index) noexcept -> IndexedValueView<decltype(self), FldI>;

        /**
         *  @brief   Alternative syntax for ViewField().
         *  @copydoc ViewField
         */
        auto operator|(this auto& self, SelectField<F> const& index) noexcept -> ValueView<decltype(self), RecI> requires DFUnqIndex<FldI>
        {
            return self.ViewField(index.val);
        }

        /**
         * @brief   Alternative syntax for ViewRecord()
         * @copydoc ViewRecord
         */
        auto operator|(this auto& self, SelectRecord<R> const& index) noexcept -> ValueView<decltype(self), FldI> requires DFUnqIndex<RecI>
        {
            return self.ViewRecord(index.val);
        }

        /**
         * @brief   Alternative syntax for ViewFieldIndexed()
         * @copydoc ViewFieldIndexed
         */
        auto operator|(this auto& self, SelectFieldIndexed<F> const& index) noexcept -> IndexedValueView<decltype(self), RecI> requires DFUnqIndex<FldI>
        {
            return self.ViewFieldIndexed(index.val);
        }

        /**
         * @brief   Alternative syntax for GetRecordIndexed()
         * @copydoc ViewRecordIndexed
         */
        auto operator|(this auto& self, SelectRecordIndexed<R> const& index) noexcept -> IndexedValueView<decltype(self), FldI> requires DFUnqIndex<RecI>
        {
            return self.ViewRecordIndexed(index.val);
        }

        // ======== FUNCTIONAL =====================================================================================================================================================

        /**
         * @brief       Apply a function on each value in a field.
         * @tparam Func Type of the function to apply on each value in a field.
         *
         * @param index Field index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a field.
         * @return For chaining the view the function was applied on.
         */
        template<typename Func>
        auto ForEachOnField(this auto& self, F const& index, Func&& func) -> ValueView<decltype(self), RecI> requires DFUnqIndex<FldI>;

        /**
         * @brief       Apply a function on each value in a record.
         * @tparam Func Type of the function to apply on each value in a record.
         *
         * @param index Record index to look for. If not found function returns empty view.
         * @param func  Function to apply on each value in a record.
         * @return For chaining the view the function was applied on.
         */
        template<typename Func>
        auto ForEachOnRecord(this auto& self, R const& index, Func&& func) -> ValueView<decltype(self), FldI> requires DFUnqIndex<RecI>;

        // ======== PRINT ==========================================================================================================================================================

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

        // ======== STATS ==========================================================================================================================================================

        /**
         * @return The size of all elements stored in the dataframe.
         */
        [[nodiscard]] auto Size() const noexcept -> size_t { return recsData.size(); }

        /**
         * @return The count of fields.
         */
        [[nodiscard]] auto FieldSize() const noexcept -> size_t { return fldIndex.Size(); }

        /**
         * @return The count of records.
         */
        [[nodiscard]] auto RecordSize() const noexcept -> size_t { return recIndex.Size(); }

        /**
         * @attention It's a view so can be invalidated when adding/removing fields/records.
         * @return A view into the current fields (indices) stored in the dataframe.
         */
        [[nodiscard]] auto Fields() const noexcept -> Flds { return fldIndex.Keys(); }

        /**
         * @attention It's a view so can be invalidated when adding/removing fields/records.
         * @return A view into the current records (indices) stored in the dataframe.
         */
        [[nodiscard]] auto Records() const noexcept -> Recs { return recIndex.Keys(); }

        /**
         * @attention It's a pointer so can be invalidated when adding/removing fields/records.
         *            Only keep this pointer alive as long as this dataframe wasn't mutated.
         * @attention Pointer can be null!
         * @return    Pointer to the currently stored dataframe->data.
         */
        [[nodiscard]] auto Data() const noexcept -> T const* { return data; }

        /**
         * @attention It's a pointer so can be invalidated when adding/removing fields/records.
         *            Only keep this pointer alive as long as this dataframe wasn't mutated.
         * @return    View as std::mdspan into the data.
         */
        [[nodiscard]] auto MDSpan() const noexcept -> ConstDataMatrix { return recsData; };

        /**
         * @attention It's a view so can be invalidated when adding/removing fields/records.
         * @return    A view into the current records (indices) stored in the dataframe.
         */
        [[nodiscard]] auto Values(this auto& self) noexcept -> std::span<Value<decltype(self)>> { return std::span(self.data, self.recsData.size()); }

        /**
         * @return True when no values are stored in the dataframe.
         */
        [[nodiscard]] auto Empty() const noexcept -> bool { return recsData.empty(); }

        /**
         * @return Shared reference to the memory resource used by the dataframe.
         * @note   No assumptions about the resource. E.g., no thread safety.
         */
        [[nodiscard]] auto MemoryResource() const noexcept -> std::shared_ptr<std::pmr::memory_resource> { return backingRes; }

        /**
         * @return Default memory resource as used by the Dataframe if non is passed.
         * @note   Not thread safe.
         * @see    MemoryResource() to get a reference to the current used resource.
         */
        [[nodiscard]] static auto DefaultMemoryResource() noexcept -> MemRsc { return internal::BackingResDefault(); }

    private:

        static_assert(std::is_trivially_copyable_v<Flds>, "Fields() returns this.");
        static_assert(std::is_trivially_copyable_v<Recs>, "Records() returns this.");

        // data section
        MemRsc     backingRes;     // memory resource to use
        size_t     capacity;       // capacity of data
        T*         data;           // pointer to allocated memory
        DataMatrix recsData;       // view into whole stored data

        // indices/view section
        FldI fldIndex;              // index for fields
        RecI recIndex;              // index for records
    };

    // ======== CONSTRUCTION =======================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame(MemRsc res) noexcept :
        backingRes(std::move(res)),
        capacity(0),
        data(nullptr),
        recsData(data, 0, 0)
    {
        if constexpr (DFUnqIndex<FldI>) fldIndex = FldI{backingRes.get(), 0};
        else                              fldIndex = FldI{};

        if constexpr (DFUnqIndex<RecI>) recIndex = RecI{backingRes.get(), 0};
        else                              recIndex = RecI{};
    }

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame(size_t const reservedValues, MemRsc res) noexcept :
        backingRes(std::move(res)),
        capacity(reservedValues),
        data(reservedValues == 0 ? nullptr : static_cast<T*>(backingRes->allocate(reservedValues * sizeof(T), internal::Alignment<T>()))),
        recsData(data, 0, 0)
    {
        if constexpr (DFUnqIndex<FldI>) fldIndex = FldI{backingRes.get(), 0};
        else                              fldIndex = FldI{};

        if constexpr (DFUnqIndex<RecI>) recIndex = RecI{backingRes.get(), 0};
        else                              recIndex = RecI{};
    }

    // ======== CONSTRUCTION FUNCTIONS =============================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFields(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::span<FldT const>> const fields, size_t const reservedValues, MemRsc res) noexcept -> DataFrame
    {
        // TODO change this to Use function X with default value
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");
        auto df = DataFrame{reservedValues, std::move(res)};

        // add new fields to empty df
        if constexpr(IS_FLD_RNG) df.SetFieldRange(fields, T());
        else df.AddFields(fields);

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFields(std::initializer_list<FldT const> const fields, size_t const reservedValues, MemRsc res) noexcept -> DataFrame requires DFUnqIndex<FldI>
    {
        // TODO change this to Use function X with default value
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        auto reserve = std::max(fields.size(), reservedValues);
        auto df = DataFrame(reserve, std::move(res));
        df.AddFields(fields);

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecord(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::span<F const>> const fldIndices,
                                                    std::conditional_t<IS_REC_RNG, DFRangeIndexBounds<RecT>, std::span<R const>> const recIndices,
                                                    std::span<T const> const recValues,
                                                    size_t const             capacity,
                                                    MemRsc                   res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        size_t const fldIndicesSize = fldIndices.size();
        size_t const recIndicesSize = recIndices.size();
        LUGIZMO_ASSERT(recIndicesSize == recValues.size() || recValues.empty(), "FromFieldsAndRecord expects either no record values or exactly one record-size value span.");

        auto reserve = std::max<size_t>(fldIndicesSize * recIndicesSize, capacity);
        auto df      = DataFrame(reserve, std::move(res));

        // add fields to dataframe
        if constexpr(IS_FLD_RNG) df.SetFieldRange(fldIndices);
        else                   df.AddFields(fldIndices);

        // populate records in dataframe
        if(not recValues.empty())
        {
            if constexpr(IS_REC_RNG) df.SetRecordRange(recIndices, recValues);
            else                   for(auto rec : recIndices) df.AddRecordPopulated(rec, recValues); // TODO add function to add multiple records with same values
        }
        else
        {
            if constexpr(IS_REC_RNG) df.SetRecordRange(recIndices);
            else                   for(auto rec : recIndices) df.AddRecord(rec);                     // TODO add function to add multiple records with same values
        }

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecords(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::span<F const>> const fldIndices,
                                                    std::conditional_t<IS_REC_RNG, DFRangeIndexBounds<RecT>, std::span<R const>> const recIndices,
                                                    IterableOfIterable auto const& recValues,
                                                    size_t const                   capacity,
                                                    MemRsc                         res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        size_t const fldIndicesSize = fldIndices.size();
        size_t const recIndicesSize = recIndices.size();
        LUGIZMO_ASSERT(recIndicesSize == recValues.size() || recValues.empty(), "FromFieldsAndRecords expects record count to match provided row-value count.");

        auto reserve = std::max<size_t>(fldIndicesSize *recIndicesSize, capacity);
        auto df      = DataFrame(reserve, std::move(res));

        // add fields to dataframe
        if constexpr(IS_FLD_RNG) df.SetFieldRange(fldIndices);
        else                  df.AddFields(fldIndices);

        // populate records in dataframe
        size_t valueIndex = 0;
        if constexpr(IS_REC_RNG) df.SetRecordRange(recIndices, recValues);
        else                  for(auto rec : recIndices) df.AddRecordPopulated(rec, recValues[valueIndex++]); // TODO add function to init. in "one go"

        return df;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::FromFieldsAndRecords(std::conditional_t<IS_FLD_RNG, DFRangeIndexBounds<FldT>, std::initializer_list<F const>> const fldIndices,
                                                     std::conditional_t<IS_REC_RNG, DFRangeIndexBounds<RecT>, std::initializer_list<R const>> const recIndices,
                                                     std::initializer_list<std::initializer_list<T>> const recValues,
                                                     size_t const capacity,
                                                     MemRsc       res) noexcept -> DataFrame
    {
        // TODO this should not be required because records are passed
        static_assert(std::is_default_constructible_v<T>, "Currently only default_constructible is supported");

        size_t const fldIndicesSize = fldIndices.size();
        size_t const recIndicesSize = recIndices.size();
        LUGIZMO_ASSERT(recIndicesSize == recValues.size() || recValues.size() == 0, "FromFieldsAndRecords expects record count to match initializer-list row count.");

        auto reserve = std::max<size_t>(fldIndicesSize * recIndicesSize, capacity);
        auto df      = DataFrame(reserve, std::move(res));

        // add fields to dataframe
        if constexpr(IS_FLD_RNG) df.SetFieldRange(fldIndices);
        else                   df.AddFields(fldIndices);

        // populate records in dataframe
        if(recValues.size() != 0)
        {
            if constexpr(IS_REC_RNG)
            {
                df.SetRecordRange(recIndices, recValues);
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
            if constexpr(IS_REC_RNG) df.SetRecordRange(recIndices);
            else                   for(auto rec : recIndices) df.AddRecord(rec);                     // TODO add function to add multiple records with same values
        }

        return df;
    }

    // ======== COPY, MOVE & DELETE ================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    DataFrame<T, F, R, L>::DataFrame(DataFrame const& other) noexcept :
        backingRes(other.backingRes != nullptr ? other.backingRes : internal::BackingResDefault()),
        capacity(0),
        data(nullptr),
        recsData(data, 0, 0),
        fldIndex([&]() -> FldI
        {
            if constexpr(DFUnqIndex<FldI>) return FldI(other.fldIndex, backingRes.get());
            else                           return FldI(other.fldIndex);
        }()),
        recIndex([&]() -> RecI
        {
            if constexpr(DFUnqIndex<RecI>) return RecI(other.recIndex, backingRes.get());
            else                           return RecI(other.recIndex);
        }())
    {
        auto const rowCount    = other.RecordSize();
        auto const colCount    = other.FieldSize();
        auto const activeCount = other.Size();

        if(other.capacity > 0)
        {
            data = static_cast<T*>(backingRes->allocate(other.capacity * sizeof(T), internal::Alignment<T>()));
            capacity = other.capacity;
        }

        if(activeCount > 0)
        {
            if constexpr(std::is_trivially_copyable_v<T>) std::memcpy(data, other.data, activeCount * sizeof(T));
            else                                          std::uninitialized_copy_n(other.data, activeCount, data);
        }

        recsData = DataMatrix{data, rowCount, colCount};
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::operator=(DataFrame const& other) noexcept -> DataFrame&
    {
        if(this == &other) return *this;

        auto copy = DataFrame(other);
        *this = std::move(copy);

        return *this;
    }

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
            if(data != nullptr)
            {
                LUGIZMO_ASSERT_TRACE(backingRes != nullptr, "DataFrame move assignment cannot release data without a memory resource.");
                Layout::Free(data, capacity, recsData, *backingRes.get());
            }
            else
            {
                capacity = 0;
                recsData = {};
            }

            backingRes = std::move(other.backingRes);
            capacity   = other.capacity;
            data       = other.data;
            recsData   = other.recsData;
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
        LUGIZMO_ASSERT_TRACE(not (data != nullptr && capacity == 0), "DataFrame invariant failed: non-null data with zero capacity.");
        if(data != nullptr)
        {
            LUGIZMO_ASSERT_TRACE(backingRes != nullptr, "DataFrame cannot release data without a memory resource.");
            Layout::Free(data, capacity, recsData, *backingRes.get());
        }
    }

    // ======== MANIPULATION UNIQUE INDEX ==========================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddField(F index, T const& defaultValue) -> bool requires DFUnqIndex<FldI>
    {
        // add field to the index
        auto const added = fldIndex.Add(std::move(index));
        if(not added) return false;

        // if added to index add new columns to data
        Layout::ResizeCols(data, capacity, *backingRes.get(), recsData, 0, 1, defaultValue);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddFields(std::span<F const> const indices, T const& defaultValue) -> std::size_t requires DFUnqIndex<FldI>
    {
        // add fields to the index
        auto const added = fldIndex.AddMultiple(indices);
        if(added == 0) return 0;

        // add fields to storage
        Layout::ResizeCols(data, capacity, *backingRes.get(), recsData, 0, static_cast<ssize_t>(added), defaultValue);
        return added;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecord(R index, T const& defaultValue) -> bool requires DFUnqIndex<RecI>
    {
        // add row to the index
        auto const added = recIndex.Add(std::move(index));
        if(not added) return false;

        // add row to storage
        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, 0, 1, defaultValue);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecords(std::span<R const> indices, T const& defaultValue) -> std::size_t requires DFUnqIndex<RecI>
    {
        // add rows to the index
        auto const added = recIndex.AddMultiple(indices);
        if(added == 0) return 0;

        // add rows to storage
        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, 0, static_cast<ssize_t>(added), defaultValue);
        return added;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::AddRecordPopulated(R index, std::span<T const> records) -> bool requires DFUnqIndex<RecI>
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

    // ======== MANIPULATION RANGE INDEX ===========================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetFieldRange(std::optional<FldT> const lower, std::optional<FldT> const upper, T const& defaultVal) noexcept -> bool requires DFRngIndex<FldI>
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
    auto DataFrame<T, F, R, L>::SetFieldRange(DFRangeIndexBounds<FldT> bounds, T const& defaultVal) noexcept -> bool requires DFRngIndex<FldI>
    {
        if(!fldIndex.SetStep(bounds.step)) return false;
        return SetFieldRange(bounds.lower, bounds.upper, defaultVal);
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower, std::optional<RecT> const upper, T const& defaultVal) noexcept -> bool requires DFRngIndex<RecI>
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
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, T const& defaultVal) noexcept -> bool requires DFRngIndex<RecI>
    {
        if(!recIndex.SetStep(bounds.step)) return false;
        return SetRecordRange(bounds.lower, bounds.upper, defaultVal);
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower, std::optional<RecT> const upper, std::span<T const> const records) noexcept -> bool requires DFRngIndex<RecI>
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
        if(records.size() != static_cast<size_t>(fldIndex.Size())) // TODO this should not be needed as DFRangeIndexBounds should not allow negative sizes
        {
            recIndex.SetLowerBound(currentLower);
            recIndex.SetUpperBound(currentUpper);
            return false;
        }

        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, records);
        return true;
    }

    template<typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, std::span<T const> const records) noexcept -> bool requires DFRngIndex<RecI>
    {
        if(!recIndex.SetStep(bounds.step)) return false;
        return SetRecordRange(bounds.lower, bounds.upper, records);
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower,
                                               std::optional<RecT> const upper,
                                               IterableOfIterable auto const& records) noexcept -> bool requires DFRngIndex<RecI>
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
        if(records.size() != static_cast<size_t>(recIndex.Size())) // TODO this should not be needed as DFRangeIndexBounds should not allow negative sizes
        {
            recIndex.SetLowerBound(currentLower);
            recIndex.SetUpperBound(currentUpper);
            return false;
        }

        Layout::ResizeRows(data, capacity, *backingRes.get(), recsData, lowerChange ? lowerChange.value() : 0, upperChange ? upperChange.value() : 0, records);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(DFRangeIndexBounds<RecT> const bounds, IterableOfIterable auto const& records) noexcept -> bool requires DFRngIndex<RecI>
    {
        if(!recIndex.SetStep(bounds.step)) return false;
        return SetRecordRange(bounds.lower, bounds.upper, records);
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::SetRecordRange(std::optional<RecT> const lower,
                                               std::optional<RecT> const upper,
                                               std::initializer_list<std::initializer_list<T>> const records) noexcept -> bool requires DFRngIndex<RecI>
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
        if(records.size() != static_cast<size_t>(recIndex.Size())) // TODO this should not be needed as DFRangeIndexBounds should not allow negative sizes
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
        requires DFRngIndex<RecI>
    {
        if(!recIndex.SetStep(bounds.step)) return false;
        return SetRecordRange(bounds.lower, bounds.upper, records);
    }

    // ======== CHECKS =============================================================================================================================================================

    template<typename T, typename F, typename R, typename L>
    template<typename C>
    auto DataFrame<T, F, R, L>::HasField(C const& field) const noexcept -> bool
    {
        static_assert(requires(FldI const& index, C const& lookup) {{ index.Has(lookup) } -> std::convertible_to<bool>;},
                      "DataFrame field lookup requires the exact key type or transparent hash/equality support.");

        return fldIndex.Has(field);
    }

    template<typename T, typename F, typename R, typename L>
    template<typename C>
    auto DataFrame<T, F, R, L>::HasRecord(C const& record) const noexcept -> bool
    {
        static_assert(requires(RecI const& index, C const& lookup) {{ index.Has(lookup) } -> std::convertible_to<bool>;},
                      "DataFrame record lookup requires the exact key type or transparent hash/equality support.");

        return recIndex.Has(record);
    }

    // ======== GETTERS ============================================================================================================================================================

    template<typename T, typename F, typename R, typename L>
    template<typename FieldLookup, typename RecordLookup>
    auto DataFrame<T, F, R, L>::GetValue(this auto& self, FieldLookup const& field, RecordLookup const& record) -> OptValue<decltype(self)>
    {
        static_assert(requires(FldI const& index, FieldLookup const& lookup) { index.Position(lookup); },
                      "DataFrame field lookup requires the exact key type or transparent hash/equality support.");

        static_assert(requires(RecI const& index, RecordLookup const& lookup) { index.Position(lookup); },
                      "DataFrame record lookup requires the exact key type or transparent hash/equality support.");

        auto const fldPos = self.fldIndex.Position(field);
        auto const recPos = self.recIndex.Position(record);

        if(not (fldPos && recPos)) return {};
        return self.recsData[*recPos, *fldPos];
    }

    template<typename T, typename F, typename R, typename L>
    template<typename FieldLookup, typename RecordLookup>
    auto DataFrame<T, F, R, L>::operator[](this auto& self, FieldLookup const& field, RecordLookup const& record) -> Value<decltype(self)>&
    {
        static_assert(requires(FldI const& index, FieldLookup const& lookup) { index.Position(lookup); },
                      "DataFrame field lookup requires the exact key type or transparent hash/equality support.");

        static_assert(requires(RecI const& index, RecordLookup const& lookup) { index.Position(lookup); },
                      "DataFrame record lookup requires the exact key type or transparent hash/equality support.");

        auto const fldPos = self.fldIndex.Position(field);
        auto const recPos = self.recIndex.Position(record);

        LUGIZMO_ASSERT(fldPos.has_value() && recPos.has_value(), "DataFrame::operator[] requires existing field and record.");
        return self.recsData[*recPos, *fldPos];
    }

    // ======== SETTERS ============================================================================================================================================================

    template<typename T, typename F, typename R, typename L>
    template<typename U>
    requires std::constructible_from<T, U&&>
    auto DataFrame<T, F, R, L>::AssignValue(FldT const& field, RecT const& record, U&& value) -> bool
    {
        if(auto get = GetValue(field, record); get)
        {
            T& g = *get;
            if constexpr(std::assignable_from<T&, U&&>) g = std::forward<U>(value);
            else                                        g = T(std::forward<U>(value));
            return true;
        }

        return false;
    }

    template<typename T, typename F, typename R, typename L>
    template<typename U, size_t Extent>
    requires std::constructible_from<T, U const&>
    auto DataFrame<T, F, R, L>::AssignFieldValues(FldT const& field, std::span<U const, Extent> const values) -> bool
    {
        auto const fldPos = fldIndex.Position(field);

        if(not fldPos) return false;
        if(values.size() != RecordSize()) return false;

        if constexpr(std::same_as<U, T> && std::is_trivially_copyable_v<T> && std::is_same_v<L, ColMajor>)
        {
            auto* dst = data + static_cast<size_t>(*fldPos) * RecordSize();
            std::memcpy(dst, values.data(), values.size_bytes());
        }
        else
        {
            for(size_t recPos = 0U; recPos < values.size(); ++recPos)
            {
                auto& g = recsData[static_cast<std::ptrdiff_t>(recPos), *fldPos];

                if constexpr(std::assignable_from<T&, U const&>) g = values[recPos];
                else g = T(values[recPos]);
            }
        }

        return true;
    }

    template<typename T, typename F, typename R, typename L>
    template<typename U, std::size_t Extent> requires std::constructible_from<T, U const&>
    auto DataFrame<T, F, R, L>::AssignFieldValues(FldT const& field, std::span<U, Extent> values) -> bool
    {
        return AssignFieldValues(field, std::span<U const, Extent>(values));
    }

    template<typename T, typename F, typename R, typename L>
    template<typename U, size_t Extent>
    requires std::constructible_from<T, U const&>
    auto DataFrame<T, F, R, L>::AssignRecordValues(RecT const& record, std::span<U const, Extent> const values) -> bool
    {
        auto const recPos = recIndex.Position(record);

        if(not recPos) return false;
        if(values.size() != FieldSize()) return false;

        if constexpr(std::same_as<U, T> && std::is_trivially_copyable_v<T> && std::is_same_v<L, RowMajor>)
        {
            auto* dst = data + static_cast<size_t>(*recPos) * FieldSize();
            std::memcpy(dst, values.data(), values.size_bytes());
        }
        else
        {
            for(size_t fldPos = 0U; fldPos < values.size(); ++fldPos)
            {
                auto& g = recsData[*recPos, static_cast<std::ptrdiff_t>(fldPos)];

                if constexpr(std::assignable_from<T&, U const&>) g = values[fldPos];
                else g = T(values[fldPos]);
            }
        }

        return true;
    }

    template<typename T, typename F, typename R, typename L>
    template<typename U, std::size_t Extent> requires std::constructible_from<T, U const&>
    auto DataFrame<T, F, R, L>::AssignRecordValues(RecT const& record, std::span<U, Extent> values) -> bool
    {
        return AssignRecordValues(record, std::span<U const, Extent>(values));
    }

    template <typename T, typename F, typename R, typename L>
    template <typename U>
    requires std::constructible_from<T, U&&>
    void DataFrame<T, F, R, L>::UpsertValue(FldT const& field, RecT const& record, U&& value) requires DFUnqIndices<FldI, RecI>
    {
        auto fldPos = fldIndex.Position(field);
        auto recPos = recIndex.Position(record);

        // fast path present
        if(fldPos && recPos)
        {
            T& cell = recsData[*recPos, *fldPos];

            if constexpr(std::assignable_from<T&, U&&>) cell = std::forward<U>(value);
            else cell = T(std::forward<U>(value));

            return;
        }

        if(not fldPos)
        {
            [[maybe_unused]] auto const fieldAdded = AddField(field);
            LUGIZMO_ASSERT_TRACE(fieldAdded, "UpsertValue expected AddField to succeed.");

            fldPos = fldIndex.Position(field);
            LUGIZMO_ASSERT_TRACE(fldPos.has_value(), "UpsertValue expected field position after AddField.");
        }

        if(not recPos)
        {
            [[maybe_unused]] auto const recordAdded = AddRecord(record);
            LUGIZMO_ASSERT_TRACE(recordAdded, "UpsertValue expected AddRecord to succeed.");

            recPos = recIndex.Position(record);
            LUGIZMO_ASSERT_TRACE(recPos.has_value(), "UpsertValue expected record position after AddRecord.");
        }

        T& cell = recsData[*recPos, *fldPos];

        if constexpr(std::assignable_from<T&, U&&>) cell = std::forward<U>(value);
        else cell = T(std::forward<U>(value));
    }

    // ======== DROP ===============================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::DropField(F const& index) -> bool requires DFUnqIndex<FldI>
    {
        // drop record from index
        auto const dropped = fldIndex.Drop(index);
        if(not dropped.has_value()) return false;

        // drop row from storage
        Layout::DropColumn(data, capacity, *backingRes.get(), recsData, *dropped);
        return true;
    }

    template <typename T, typename F, typename R, typename L>
    auto DataFrame<T, F, R, L>::DropRecord(R const& index) -> bool requires DFUnqIndex<RecI>
    {
        // drop record from index
        auto const dropped = recIndex.Drop(index);
        if(not dropped.has_value()) return false;

        // drop row from storage
        Layout::DropRow(data, capacity, *backingRes.get(), recsData, *dropped);
        return true;
    }

    // ======== SORT ===============================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    template<typename Compare>
    void DataFrame<T, F, R, L>::SortFields(Compare comp)
    {
        if constexpr(DFUnqIndex<FldI>)
        {
            auto const permutation = fldIndex.Sort(std::move(comp));
            Layout::ReorderColumns(data, capacity, *backingRes.get(), recsData, permutation);
        }
    }

    template <typename T, typename F, typename R, typename L>
    template<typename Compare>
    void DataFrame<T, F, R, L>::SortRecords(Compare comp)
    {
        if constexpr(DFUnqIndex<RecI>)
        {
            auto const permutation = recIndex.Sort(std::move(comp));
            Layout::ReorderRows(data, capacity, *backingRes.get(), recsData, permutation);
        }
    }

    // ======== VIEWS ==============================================================================================================================================================

    template<typename T, typename F, typename R, typename L>
    template<typename FieldLookup>
    auto DataFrame<T, F, R, L>::ViewField(this auto& self, FieldLookup const& index) noexcept -> ValueView<decltype(self), RecI>
    {
        static_assert(requires(FldI const& fieldIndex, FieldLookup const& lookup) { fieldIndex.Position(lookup); },
                      "DataFrame field lookup requires the exact key type or transparent hash/equality support.");

        auto const pos = self.fldIndex.Position(index);
        if(not pos.has_value()) return ValueView<decltype(self), RecI>{};

        if constexpr (DFUnqIndex<RecI>)
        {
            if constexpr (meta::IsConstThis<decltype(self)>())
            {
                return ValueView<decltype(self), RecI>::FieldView(static_cast<ConstDataMatrix>(self.recsData), &self.recIndex, *pos);
            }
            else
            {
                return ValueView<decltype(self), RecI>::FieldView(self.recsData, &self.recIndex, *pos);
            }
        }
        else
        {
            if constexpr (meta::IsConstThis<decltype(self)>())
            {
                return ValueView<decltype(self), RecI>::FieldView(static_cast<ConstDataMatrix>(self.recsData),
                                                             &self.recIndex,
                                                             *pos,
                                                             self.recIndex.LowerBoundPosition(),
                                                             self.recIndex.UpperBoundPosition());
            }
            else
            {
                return ValueView<decltype(self), RecI>::FieldView(self.recsData,
                                                             &self.recIndex,
                                                             *pos,
                                                             self.recIndex.LowerBoundPosition(),
                                                             self.recIndex.UpperBoundPosition());
            }
        }
    }

    template<typename T, typename F, typename R, typename L>
    template<typename FieldLookup>
    auto DataFrame<T, F, R, L>::ViewFieldIndexed(this auto& self, FieldLookup const& index) noexcept -> IndexedValueView<decltype(self), RecI>
    {
        static_assert(requires(FldI const& fieldIndex, FieldLookup const& lookup) { fieldIndex.Position(lookup); },
                      "DataFrame field lookup requires the exact key type or transparent hash/equality support.");

        auto const pos = self.fldIndex.Position(index);
        if(not pos.has_value()) return IndexedValueView<decltype(self), RecI>{};

        if constexpr (DFUnqIndex<RecI>)
        {
            if constexpr (meta::IsConstThis<decltype(self)>())
            {
                return IndexedValueView<decltype(self), RecI>::template FieldView<>(static_cast<ConstDataMatrix>(self.recsData), &self.recIndex, *pos, self.recIndex.Keys());
            }
            else
            {
                return IndexedValueView<decltype(self), RecI>::FieldView(self.recsData, &self.recIndex, *pos, self.recIndex.Keys());
            }
        }
        else
        {
            if constexpr (meta::IsConstThis<decltype(self)>())
            {
                return IndexedValueView<decltype(self), RecI>::FieldView(static_cast<ConstDataMatrix>(self.recsData), &self.recIndex, *pos, self.recIndex.Bounds());
            }
            else
            {
                return IndexedValueView<decltype(self), RecI>::FieldView(self.recsData, &self.recIndex, *pos, self.recIndex.Bounds());
            }
        }
    }

    template<typename T, typename F, typename R, typename L>
    template<typename RecordLookup>
    auto DataFrame<T, F, R, L>::ViewRecord(this auto& self, RecordLookup const& index) noexcept -> ValueView<decltype(self), FldI> requires DFUnqIndex<FldI>
    {
        static_assert(requires(RecI const& recordIndex, RecordLookup const& lookup) { recordIndex.Position(lookup); },
                      "DataFrame record lookup requires the exact key type or transparent hash/equality support.");

        auto const pos = self.recIndex.Position(index);
        if(not pos.has_value()) return ValueView<decltype(self), FldI>{};

        if constexpr(meta::IsConstThis<decltype(self)>())
        {
            return ValueView<decltype(self), FldI>::RecordView(static_cast<ConstDataMatrix>(self.recsData), &self.fldIndex, *pos);
        }
        else
        {
            return ValueView<decltype(self), FldI>::RecordView(self.recsData, &self.fldIndex, *pos);
        }
    }

    template<typename T, typename F, typename R, typename L>
    template<typename RecordLookup>
    auto DataFrame<T, F, R, L>::ViewRecordIndexed(this auto& self, RecordLookup const& index) noexcept -> IndexedValueView<decltype(self), FldI>
    {
        static_assert(requires(RecI const& recordIndex, RecordLookup const& lookup) { recordIndex.Position(lookup); },
                      "DataFrame record lookup requires the exact key type or transparent hash/equality support.");

        auto const pos = self.recIndex.Position(index);
        if(not pos.has_value()) return IndexedValueView<decltype(self), FldI>{};

        if constexpr (DFUnqIndex<FldI>)
        {
            if constexpr (meta::IsConstThis<decltype(self)>())
            {
                return IndexedValueView<decltype(self), FldI>::template RecordView<>(static_cast<ConstDataMatrix>(self.recsData), &self.fldIndex, *pos, self.fldIndex.Keys());
            }
            else
            {
                return IndexedValueView<decltype(self), FldI>::template RecordView<>(self.recsData, &self.fldIndex, *pos, self.fldIndex.Keys());
            }
        }
        else
        {
            if constexpr (meta::IsConstThis<decltype(self)>())
            {
                return IndexedValueView<decltype(self), FldI>::RecordView(static_cast<ConstDataMatrix>(self.recsData), &self.fldIndex, *pos, self.fldIndex.Bounds());
            }
            else
            {
                return IndexedValueView<decltype(self), FldI>::RecordView(self.recsData, &self.fldIndex, *pos, self.fldIndex.Bounds());
            }
        }
    }

    // ======== FUNCTIONAL =========================================================================================================================================================

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnField(this auto& self, F const& index, Func&& func) -> ValueView<decltype(self), RecI> requires DFUnqIndex<FldI>
    {
        auto view = self.ViewField(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    template <typename T, typename F, typename R, typename L>
    template <typename Func>
    auto DataFrame<T, F, R, L>::ForEachOnRecord(this auto& self, R const& index, Func&& func) -> ValueView<decltype(self), FldI> requires DFUnqIndex<RecI>
    {
        auto view = self.ViewRecord(index);
        std::ranges::for_each(view, std::forward<Func>(func));

        return view;
    }

    // ======== PRINT ==============================================================================================================================================================

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

        // Print header: first column "Rec. Index"
        stream << std::format("{:<15}", "Rec. Index");

        // Print the field header by each field's key. Works for any index type and step, since
        // Key(position) maps a physical column back to its label.
        for (size_t pos = 0; pos < colCount; ++pos)
        {
            auto const fld = fldIndex.Key(pos);
            LUGIZMO_ASSERT_TRACE(fld.has_value(), "Print expected a valid field key for every field position.");
            stream << std::format("{:<15}", *fld);
        }
        stream << "\n";

        if (rowCount == 0)
        {
            stream << "Empty\n";
            return;
        }

        // Print each record: label by its key, values addressed by physical position [i, j].
        for (size_t i = 0; i < rowCount; ++i)
        {
            auto const recKey = recIndex.Key(i);
            LUGIZMO_ASSERT_TRACE(recKey.has_value(), "Print expected a valid record key for every record position.");
            stream << std::format("{:<15}", *recKey);

            for (size_t j = 0; j < colCount; ++j)
            {
                stream << std::format("{:<15} ", recsData[i, j]);
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

} // namespace lugizmo

#endif // LUGIZMO_DF_DATAFRAME_H
