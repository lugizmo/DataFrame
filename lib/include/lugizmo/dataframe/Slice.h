// Filename: Slice.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_SLICE_H
#define LUGIZMO_DF_SLICE_H

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <mdspan>
#include <memory>
#include <memory_resource>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "lugizmo/Assert.h"
#include "SliceConcepts.h"
#include "IndexUnique.h"

namespace lugizmo {

    namespace internal {

        /** @brief Storage-order random-access iterator over a two-dimensional slice. */
        template<typename T, typename Layout>
        class DFSliceIterator
        {
            T*                 data;
            std::size_t        recordCount;
            std::size_t        fieldCount;
            std::ptrdiff_t     recordStride;
            std::ptrdiff_t     fieldStride;
            std::size_t const* selectedRecords;
            std::size_t const* selectedFields;
            std::ptrdiff_t     position;

            /// @return Physical element offset for a flattened storage-order position.
            [[nodiscard]] constexpr auto Offset(std::ptrdiff_t const flatPosition) const noexcept -> std::ptrdiff_t
            {
                if constexpr(std::is_same_v<Layout, std::layout_right>)
                {
                    auto const recordPosition = static_cast<std::size_t>(flatPosition / static_cast<std::ptrdiff_t>(fieldCount));
                    auto const fieldPosition  = static_cast<std::size_t>(flatPosition % static_cast<std::ptrdiff_t>(fieldCount));
                    auto const records = selectedRecords == nullptr ? recordPosition : selectedRecords[recordPosition];
                    auto const fields  = selectedFields == nullptr ? fieldPosition : selectedFields[fieldPosition];
                    return static_cast<std::ptrdiff_t>(records) * recordStride + static_cast<std::ptrdiff_t>(fields) * fieldStride;
                }
                else
                {
                    static_assert(std::is_same_v<Layout, std::layout_left>, "DFSlice supports layout_right and layout_left.");
                    auto const fieldPosition  = static_cast<std::size_t>(flatPosition / static_cast<std::ptrdiff_t>(recordCount));
                    auto const recordPosition = static_cast<std::size_t>(flatPosition % static_cast<std::ptrdiff_t>(recordCount));
                    auto const fields  = selectedFields == nullptr ? fieldPosition : selectedFields[fieldPosition];
                    auto const records = selectedRecords == nullptr ? recordPosition : selectedRecords[recordPosition];
                    return static_cast<std::ptrdiff_t>(records) * recordStride + static_cast<std::ptrdiff_t>(fields) * fieldStride;
                }
            }

            /// @return True when both iterators describe the same data and axis mappings.
            [[nodiscard]] constexpr auto SameSlice(DFSliceIterator const& other) const noexcept -> bool
            {
                return data == other.data && recordCount == other.recordCount && fieldCount == other.fieldCount && recordStride == other.recordStride &&
                       fieldStride == other.fieldStride && selectedRecords == other.selectedRecords && selectedFields == other.selectedFields;
            }

        public:

            // NOLINTBEGIN(readability-identifier-naming)
            using iterator_category = std::random_access_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = std::remove_const_t<T>;
            using pointer           = T*;
            using reference         = T&;
            // NOLINTEND(readability-identifier-naming)

            /// @brief Constructs a singular iterator.
            constexpr DFSliceIterator() noexcept :
                data(nullptr), recordCount(0), fieldCount(0), recordStride(0), fieldStride(0), selectedRecords(nullptr), selectedFields(nullptr), position(0)
            {
            }

            /// @brief Constructs an iterator at `position` within a resolved slice mapping.
            constexpr DFSliceIterator(T* const data, std::size_t const records, std::size_t const fields, std::ptrdiff_t const recStride, std::ptrdiff_t const fldStride,
                                      std::size_t const* const selectedRecords, std::size_t const* const selectedFields,
                                      std::ptrdiff_t const position = 0) noexcept :
                data(data), recordCount(records), fieldCount(fields), recordStride(recStride), fieldStride(fldStride), selectedRecords(selectedRecords),
                selectedFields(selectedFields), position(position)
            {
            }

            constexpr DFSliceIterator(DFSliceIterator const&) noexcept                    = default;
            constexpr DFSliceIterator(DFSliceIterator&&) noexcept                         = default;
            constexpr auto operator=(DFSliceIterator const&) noexcept -> DFSliceIterator& = default;
            constexpr auto operator=(DFSliceIterator&&) noexcept -> DFSliceIterator&      = default;
            constexpr ~DFSliceIterator() noexcept                                         = default;

            /// @return Reference to the value at the current logical position.
            [[nodiscard]] constexpr auto operator*() const noexcept -> reference { return data[Offset(position)]; }

            /// @return Pointer to the value at the current logical position.
            [[nodiscard]] constexpr auto operator->() const noexcept -> pointer { return data + Offset(position); }

            /// @return Reference to the value `n` logical positions from this iterator.
            [[nodiscard]] constexpr auto operator[](difference_type const n) const noexcept -> reference { return data[Offset(position + n)]; }

            /// @brief Advances to the next logical position.
            constexpr auto operator++() noexcept -> DFSliceIterator&
            {
                ++position;
                return *this;
            }

            /// @brief Advances to the next logical position and returns the previous iterator.
            constexpr auto operator++(int) noexcept -> DFSliceIterator
            {
                auto copy = *this;
                ++(*this);
                return copy;
            }

            /// @brief Moves to the previous logical position.
            constexpr auto operator--() noexcept -> DFSliceIterator&
            {
                --position;
                return *this;
            }

            /// @brief Moves to the previous logical position and returns the previous iterator.
            constexpr auto operator--(int) noexcept -> DFSliceIterator
            {
                auto copy = *this;
                --(*this);
                return copy;
            }

            /// @brief Advances by `n` logical positions.
            constexpr auto operator+=(difference_type const n) noexcept -> DFSliceIterator&
            {
                position += n;
                return *this;
            }

            /// @brief Moves backward by `n` logical positions.
            constexpr auto operator-=(difference_type const n) noexcept -> DFSliceIterator&
            {
                position -= n;
                return *this;
            }

            /// @return Iterator advanced by `n` logical positions.
            [[nodiscard]] constexpr auto operator+(difference_type const n) const noexcept -> DFSliceIterator
            {
                auto copy  = *this;
                copy      += n;
                return copy;
            }

            /// @return Iterator moved backward by `n` logical positions.
            [[nodiscard]] constexpr auto operator-(difference_type const n) const noexcept -> DFSliceIterator
            {
                auto copy  = *this;
                copy      -= n;
                return copy;
            }

            /// @return Logical distance from `other`; both iterators must belong to the same slice.
            [[nodiscard]] constexpr auto operator-(DFSliceIterator const& other) const noexcept -> difference_type
            {
                LUGIZMO_ASSERT(SameSlice(other), "DFSlice iterator difference requires iterators from the same slice.");
                return position - other.position;
            }

            /// @return True when both iterators belong to the same slice and position.
            [[nodiscard]] constexpr auto operator==(DFSliceIterator const& other) const noexcept -> bool { return SameSlice(other) && position == other.position; }

            /// @return Negation of `operator==`.
            [[nodiscard]] constexpr auto operator!=(DFSliceIterator const& other) const noexcept -> bool { return !(*this == other); }

            /// @return Whether this position precedes `other`; both iterators must belong to the same slice.
            [[nodiscard]] constexpr auto operator<(DFSliceIterator const& other) const noexcept -> bool
            {
                LUGIZMO_ASSERT(SameSlice(other), "DFSlice iterator ordering requires iterators from the same slice.");
                return position < other.position;
            }

            /// @return Whether this position does not follow `other`.
            [[nodiscard]] constexpr auto operator<=(DFSliceIterator const& other) const noexcept -> bool { return !(other < *this); }

            /// @return Whether this position follows `other`.
            [[nodiscard]] constexpr auto operator>(DFSliceIterator const& other) const noexcept -> bool { return other < *this; }

            /// @return Whether this position does not precede `other`.
            [[nodiscard]] constexpr auto operator>=(DFSliceIterator const& other) const noexcept -> bool { return !(*this < other); }

            /// @return Iterator advanced by `n` logical positions.
            friend constexpr auto operator+(difference_type const n, DFSliceIterator const& iterator) noexcept -> DFSliceIterator { return iterator + n; }
        };

    } // namespace internal

    /**
     * @brief Two-dimensional selection of dataframe values.
     *
     * A slice stores one base pointer, two extents, and the physical stride of
     * each dataframe axis. Arbitrary key selections additionally own their resolved
     * physical positions. Flattened iteration follows storage order: fields vary
     * fastest for `layout_right`, while records vary fastest for `layout_left`.
     * Positional two-dimensional access always uses `(record, field)` order.
     *
     * `Contiguous` is a guarantee made by the slice type. `IsContiguous()` also
     * recognizes instances of a conservatively strided slice that happen to form
     * one contiguous region. `IsFieldConsecutive()`, `IsRecordConsecutive()`, and
     * `IsConsecutive()` instead describe whether the selected physical index
     * positions contain gaps; consecutive selections may still have memory gaps
     * between rows or columns. None of these operations copies or rearranges values.
     *
     * The values and both indices are borrowed; selected-position mappings are
     * owned by the slice. Their memory resources are borrowed and must outlive the
     * slice and its copies. Structural dataframe changes invalidate the slice and
     * everything obtained from it. A selected slice is not a borrowed range because
     * its iterators refer to that owned mapping. A type-guaranteed contiguous slice
     * uses pointer iterators and remains a borrowed range.
     *
     * @tparam T      Element type, including its const qualification.
     * @tparam F      Field index type.
     * @tparam R      Record index type.
     * @tparam Layout Physical dataframe layout (`layout_right` or `layout_left`).
     * @tparam C      Whether every instance of this specialization is contiguous.
     */
    template<typename T, typename F, typename R, typename Layout, bool C = false>
    class DFSlice: public std::ranges::view_interface<DFSlice<T, F, R, Layout, C>>
    {
        template<typename, typename, typename, typename, bool>
        friend class DFSlice;

        static_assert(std::is_same_v<Layout, std::layout_right> || std::is_same_v<Layout, std::layout_left>, "DFSlice supports layout_right and layout_left.");

        using FieldIndex  = F const*;
        using RecordIndex = R const*;
        using FieldKey    = F::KeyType;
        using RecordKey   = R::KeyType;
        using Extents     = std::dextents<std::ptrdiff_t, 2>;
        using Matrix      = std::mdspan<T, Extents, Layout>;
        using Positions   = std::pmr::vector<std::size_t>;

        T*             data;
        FieldIndex     fieldIndex;
        RecordIndex    recordIndex;
        std::size_t    fieldOffset;
        std::size_t    recordOffset;
        std::size_t    fieldCount;
        std::size_t    recordCount;
        std::ptrdiff_t fieldStride;
        std::ptrdiff_t recordStride;
        std::size_t    fieldPositionStep;
        std::size_t    recordPositionStep;
        Positions      selectedFields;
        Positions      selectedRecords;

        /// @return Sorted local positions for a key selection, or no value when resolution fails.
        template<typename Index, typename Selection, typename Resolver>
        requires DFSliceSelectionFor<Selection, Index>
        [[nodiscard]] static auto ResolveSelection(Selection&& selection, std::pmr::memory_resource* const resource,
                                                   Resolver&& resolver) noexcept -> std::optional<Positions>
        {
            if constexpr(DFSliceRangeSelectionFor<Selection, Index>)
            {
                using Difference = typename Index::DiffType;
                if(selection.lower > selection.upper || selection.step <= Difference{0}) return std::nullopt;
            }

            auto positions = Positions(resource);
            if constexpr(std::ranges::sized_range<Selection>) positions.reserve(std::ranges::size(selection));
            for(auto&& key : selection)
            {
                auto const position = resolver(key);
                if(!position.has_value()) return std::nullopt;
                positions.push_back(*position);
            }

            std::ranges::sort(positions);
            auto const duplicate = std::ranges::adjacent_find(positions);
            LUGIZMO_ASSERT(duplicate == positions.end(), "DFSlice selections must not contain duplicate keys.");
            if(duplicate != positions.end()) return std::nullopt;
            return positions;
        }

        /// @return Absolute position in the source field index for a local selected position.
        [[nodiscard]] constexpr auto SourceFieldPosition(std::size_t const position) const noexcept -> std::size_t
        {
            return fieldOffset + (selectedFields.empty() ? position * fieldPositionStep : selectedFields[position]);
        }

        /// @return Absolute position in the source record index for a local selected position.
        [[nodiscard]] constexpr auto SourceRecordPosition(std::size_t const position) const noexcept -> std::size_t
        {
            return recordOffset + (selectedRecords.empty() ? position * recordPositionStep : selectedRecords[position]);
        }

        /// @return Field position relative to the slice base, resolving a gathered mapping when present.
        [[nodiscard]] constexpr auto FieldPosition(std::size_t const position) const noexcept -> std::size_t
        {
            return selectedFields.empty() ? position : selectedFields[position];
        }

        /// @return Record position relative to the slice base, resolving a gathered mapping when present.
        [[nodiscard]] constexpr auto RecordPosition(std::size_t const position) const noexcept -> std::size_t
        {
            return selectedRecords.empty() ? position : selectedRecords[position];
        }

        /// @return Physical element offset for a flattened storage-order position.
        [[nodiscard]] constexpr auto FlatOffset(std::size_t const position) const noexcept -> std::ptrdiff_t
        {
            if constexpr(std::is_same_v<Layout, std::layout_right>)
            {
                auto const record = RecordPosition(position / fieldCount);
                auto const field  = FieldPosition(position % fieldCount);
                return static_cast<std::ptrdiff_t>(record) * recordStride + static_cast<std::ptrdiff_t>(field) * fieldStride;
            }
            else
            {
                auto const field  = FieldPosition(position / recordCount);
                auto const record = RecordPosition(position % recordCount);
                return static_cast<std::ptrdiff_t>(record) * recordStride + static_cast<std::ptrdiff_t>(field) * fieldStride;
            }
        }

        /// @return Local selected-field position for `key`, or no value when unavailable or unselected.
        [[nodiscard]] constexpr auto LocalFieldPosition(FieldKey const& key) const noexcept -> std::optional<std::size_t>
        {
            if(fieldIndex == nullptr) return std::nullopt;
            auto const position = fieldIndex->Position(key);
            if(!position.has_value() || *position < fieldOffset) return std::nullopt;
            auto const localPosition = *position - fieldOffset;
            if(selectedFields.empty())
            {
                if(localPosition % fieldPositionStep != 0) return std::nullopt;
                auto const selectedPosition = localPosition / fieldPositionStep;
                if(selectedPosition >= fieldCount) return std::nullopt;
                return selectedPosition;
            }

            auto const found = std::ranges::lower_bound(selectedFields, localPosition);
            if(found == selectedFields.end() || *found != localPosition) return std::nullopt;
            return static_cast<std::size_t>(std::distance(selectedFields.begin(), found));
        }

        /// @return Local selected-record position for `key`, or no value when unavailable or unselected.
        [[nodiscard]] constexpr auto LocalRecordPosition(RecordKey const& key) const noexcept -> std::optional<std::size_t>
        {
            if(recordIndex == nullptr) return std::nullopt;
            auto const position = recordIndex->Position(key);
            if(!position.has_value() || *position < recordOffset) return std::nullopt;
            auto const localPosition = *position - recordOffset;
            if(selectedRecords.empty())
            {
                if(localPosition % recordPositionStep != 0) return std::nullopt;
                auto const selectedPosition = localPosition / recordPositionStep;
                if(selectedPosition >= recordCount) return std::nullopt;
                return selectedPosition;
            }

            auto const found = std::ranges::lower_bound(selectedRecords, localPosition);
            if(found == selectedRecords.end() || *found != localPosition) return std::nullopt;
            return static_cast<std::size_t>(std::distance(selectedRecords.begin(), found));
        }

        /// @return True when the resolved mappings form one uninterrupted region in storage order.
        [[nodiscard]] constexpr auto ComputeContiguous() const noexcept -> bool
        {
            if(Size() <= 1) return true;

            if constexpr(std::is_same_v<Layout, std::layout_right>)
            {
                if(fieldCount > 1 && (fieldStride != 1 || FieldPosition(fieldCount - 1) - FieldPosition(0) != fieldCount - 1)) return false;
                if(recordCount <= 1) return true;
                if(RecordPosition(recordCount - 1) - RecordPosition(0) != recordCount - 1) return false;
                return static_cast<std::ptrdiff_t>(RecordPosition(1)) * recordStride -
                               static_cast<std::ptrdiff_t>(FieldPosition(fieldCount - 1)) * fieldStride ==
                       1;
            }
            else
            {
                if(recordCount > 1 && (recordStride != 1 || RecordPosition(recordCount - 1) - RecordPosition(0) != recordCount - 1)) return false;
                if(fieldCount <= 1) return true;
                if(FieldPosition(fieldCount - 1) - FieldPosition(0) != fieldCount - 1) return false;
                return static_cast<std::ptrdiff_t>(FieldPosition(1)) * fieldStride -
                               static_cast<std::ptrdiff_t>(RecordPosition(recordCount - 1)) * recordStride ==
                       1;
            }
        }

        /// @brief Constructs a slice from validated, base-relative axis mappings.
        constexpr DFSlice(T* const data, FieldIndex const fields, RecordIndex const records, std::size_t const fldOffset, std::size_t const recOffset, std::size_t const fldCount,
                          std::size_t const recCount, std::ptrdiff_t const fldStride, std::ptrdiff_t const recStride,
                          std::size_t const fldPositionStep = 1, std::size_t const recPositionStep = 1,
                          Positions selectedFields = {}, Positions selectedRecords = {}) noexcept
            : data(data), fieldIndex(fields), recordIndex(records), fieldOffset(fldOffset), recordOffset(recOffset), fieldCount(fldCount), recordCount(recCount),
              fieldStride(fldStride), recordStride(recStride), fieldPositionStep(fldPositionStep), recordPositionStep(recPositionStep),
              selectedFields(std::move(selectedFields)), selectedRecords(std::move(selectedRecords))
        {
            LUGIZMO_ASSERT(!C || ComputeContiguous(), "Contiguous DFSlice specialization requires contiguous values.");
        }

    public:
        // ======== TYPES AND PROPERTIES ==========================================================================================================================================

        /// @brief True when this specialization guarantees one uninterrupted memory region.
        static constexpr bool Contiguous = C; // NOLINT(readability-identifier-naming)

        /// @brief Storage-order random-access iterator, or a pointer for guaranteed-contiguous slices.
        using Iterator = std::conditional_t<C, T*, internal::DFSliceIterator<T, Layout>>;

        /// @brief Reverse iterator over the same storage-order sequence.
        using reverse_iterator = std::reverse_iterator<Iterator>; // NOLINT(readability-identifier-naming)

        // ======== CONSTRUCTION ===================================================================================================================================================

        /**
         * @brief Constructs an empty slice.
         * @post `Empty()` is true and `begin() == end()`.
         */
        constexpr DFSlice() noexcept
            : data(nullptr), fieldIndex(nullptr), recordIndex(nullptr), fieldOffset(0), recordOffset(0), fieldCount(0), recordCount(0), fieldStride(0), recordStride(0),
              fieldPositionStep(1), recordPositionStep(1), selectedFields(), selectedRecords()
        {
        }

        /**
         * @brief   Copies a slice and any owned position mappings.
         * @details Each mapping retains the memory resource used by `other`.
         */
        DFSlice(DFSlice const& other) noexcept
            : DFSlice(other.data, other.fieldIndex, other.recordIndex, other.fieldOffset, other.recordOffset,
                      other.fieldCount, other.recordCount, other.fieldStride, other.recordStride,
                      other.fieldPositionStep, other.recordPositionStep,
                      Positions(other.selectedFields, other.selectedFields.get_allocator()),
                      Positions(other.selectedRecords, other.selectedRecords.get_allocator()))
        {
        }

        /// @brief Moves a slice together with its owned position mappings.
        DFSlice(DFSlice&&) noexcept = default;

        /// @copydoc DFSlice(DFSlice const&)
        auto operator=(DFSlice const& other) noexcept -> DFSlice&
        {
            if(this == &other) return *this;
            std::destroy_at(this);
            std::construct_at(this, other);
            return *this;
        }

        /// @brief Move-assigns a slice together with its owned position mappings.
        auto operator=(DFSlice&& other) noexcept -> DFSlice&
        {
            if(this == &other) return *this;
            std::destroy_at(this);
            std::construct_at(this, std::move(other));
            return *this;
        }

        ~DFSlice() = default;

        /**
         * @brief Creates a half-open positional rectangle `[recBegin, recEnd) × [fldBegin, fldEnd)`.
         *
         * @param original Source matrix whose first axis is records and the second axis is fields.
         * @param fields   Field index borrowed for keyed access, or null to disable field-key lookup.
         * @param records  Record index borrowed for keyed access, or null to disable record-key lookup.
         * @param fldBegin First included field position.
         * @param fldEnd   One-past-last included field position.
         * @param recBegin First included record position.
         * @param recEnd   One-past-last included record position.
         * @param resource Resource retained by the empty axis mappings and any derived slices.
         *
         * @pre `fldBegin <= fldEnd <= original.extent(1)`.
         * @pre `recBegin <= recEnd <= original.extent(0)`.
         * @pre When `C` is true, the rectangle occupies one uninterrupted memory region.
         *
         * @return A non-owning slice over the requested rectangle, or an empty slice for invalid bounds when assertions are disabled.
         */
        [[nodiscard]] static constexpr auto View(Matrix original, FieldIndex const fields, RecordIndex const records, std::size_t const fldBegin, std::size_t const fldEnd,
                                                 std::size_t const recBegin, std::size_t const recEnd,
                                                 std::pmr::memory_resource* const resource = std::pmr::get_default_resource()) noexcept -> DFSlice
        {
            auto const allRecords = static_cast<std::size_t>(original.extent(0));
            auto const allFields  = static_cast<std::size_t>(original.extent(1));
            LUGIZMO_ASSERT(fldBegin <= fldEnd && fldEnd <= allFields, "DFSlice received invalid field bounds.");
            LUGIZMO_ASSERT(recBegin <= recEnd && recEnd <= allRecords, "DFSlice received invalid record bounds.");

            if(fldBegin > fldEnd || fldEnd > allFields || recBegin > recEnd || recEnd > allRecords) return {};

            auto* base = original.data_handle();
            if(base != nullptr)
            {
                base += static_cast<std::ptrdiff_t>(recBegin) * original.mapping().stride(0) + static_cast<std::ptrdiff_t>(fldBegin) * original.mapping().stride(1);
            }

            return DFSlice(base, fields, records, fldBegin, recBegin, fldEnd - fldBegin, recEnd - recBegin,
                           original.mapping().stride(1), original.mapping().stride(0), 1, 1,
                           Positions(resource), Positions(resource));
        }

        /**
         * @brief Creates a slice over selected field positions and all records.
         *
         * @param original  Source matrix.
         * @param fields    Field index borrowed for keyed access.
         * @param records   Record index borrowed for keyed access.
         * @param positions Sorted, unique, absolute field positions; the slice retains its memory resource.
         *
         * @pre Every position is smaller than `original.extent(1)`.
         *
         * @return An empty slice when `positions` is empty; otherwise the selected Cartesian region.
         */
        [[nodiscard]] static auto SelectedFields(Matrix original, FieldIndex const fields, RecordIndex const records,
                                                 Positions positions) noexcept -> DFSlice
        {
            static_assert(!C, "A runtime field selection cannot guarantee contiguous storage in its type.");
            if(positions.empty()) return {};

            [[maybe_unused]] auto const allFields  = static_cast<std::size_t>(original.extent(1));
            auto const allRecords = static_cast<std::size_t>(original.extent(0));
            LUGIZMO_ASSERT(std::ranges::is_sorted(positions) && positions.back() < allFields, "DFSlice selected field positions must be sorted and in bounds.");
            LUGIZMO_ASSERT(std::ranges::adjacent_find(positions) == positions.end(), "DFSlice selected field positions must be unique.");

            auto const fieldOffset = positions.front();
            for(auto& position : positions) position -= fieldOffset;

            auto* base = original.data_handle();
            if(base != nullptr) base += static_cast<std::ptrdiff_t>(fieldOffset) * original.mapping().stride(1);

            auto* const resource = positions.get_allocator().resource();
            return DFSlice(base, fields, records, fieldOffset, 0, positions.size(), allRecords,
                           original.mapping().stride(1), original.mapping().stride(0), 1, 1,
                           std::move(positions), Positions(resource));
        }

        /**
         * @brief Creates a slice over all fields and selected record positions.
         *
         * @param original  Source matrix.
         * @param fields    Field index borrowed for keyed access.
         * @param records   Record index borrowed for keyed access.
         * @param positions Sorted, unique, absolute record positions; the slice retains its memory resource.
         *
         * @pre Every position is smaller than `original.extent(0)`.
         *
         * @return An empty slice when `positions` is empty; otherwise the selected Cartesian region.
         */
        [[nodiscard]] static auto SelectedRecords(Matrix original, FieldIndex const fields, RecordIndex const records,
                                                  Positions positions) noexcept -> DFSlice
        {
            static_assert(!C, "A runtime record selection cannot guarantee contiguous storage in its type.");
            if(positions.empty()) return {};

            auto const allFields = static_cast<std::size_t>(original.extent(1));
            [[maybe_unused]] auto const allRecords = static_cast<std::size_t>(original.extent(0));
            LUGIZMO_ASSERT(std::ranges::is_sorted(positions) && positions.back() < allRecords, "DFSlice selected record positions must be sorted and in bounds.");
            LUGIZMO_ASSERT(std::ranges::adjacent_find(positions) == positions.end(), "DFSlice selected record positions must be unique.");

            auto const recordOffset = positions.front();
            for(auto& position : positions) position -= recordOffset;

            auto* base = original.data_handle();
            if(base != nullptr) base += static_cast<std::ptrdiff_t>(recordOffset) * original.mapping().stride(0);

            auto* const resource = positions.get_allocator().resource();
            return DFSlice(base, fields, records, 0, recordOffset, allFields, positions.size(),
                           original.mapping().stride(1), original.mapping().stride(0), 1, 1,
                           Positions(resource), std::move(positions));
        }

        /**
         * @brief   Creates the Cartesian product of selected field and record positions.
         * @details Both mappings retain their respective memory resources.
         *
         * @param original        Source matrix.
         * @param fields          Field index borrowed for keyed access.
         * @param records         Record index borrowed for keyed access.
         * @param fieldPositions  Sorted, unique, absolute field positions.
         * @param recordPositions Sorted, unique, absolute record positions.
         *
         * @pre Field positions are smaller than `original.extent(1)` and record positions are smaller than `original.extent(0)`.
         *
         * @return An empty slice when either mapping is empty; otherwise the selected Cartesian region.
         */
        [[nodiscard]] static auto Selected(Matrix original, FieldIndex const fields, RecordIndex const records,
                                           Positions fieldPositions, Positions recordPositions) noexcept -> DFSlice
        {
            static_assert(!C, "A runtime field/record selection cannot guarantee contiguous storage in its type.");
            if(fieldPositions.empty() || recordPositions.empty()) return {};

            [[maybe_unused]] auto const allFields  = static_cast<std::size_t>(original.extent(1));
            [[maybe_unused]] auto const allRecords = static_cast<std::size_t>(original.extent(0));
            LUGIZMO_ASSERT(std::ranges::is_sorted(fieldPositions) && fieldPositions.back() < allFields, "DFSlice selected field positions must be sorted and in bounds.");
            LUGIZMO_ASSERT(std::ranges::is_sorted(recordPositions) && recordPositions.back() < allRecords, "DFSlice selected record positions must be sorted and in bounds.");
            LUGIZMO_ASSERT(std::ranges::adjacent_find(fieldPositions) == fieldPositions.end(), "DFSlice selected field positions must be unique.");
            LUGIZMO_ASSERT(std::ranges::adjacent_find(recordPositions) == recordPositions.end(), "DFSlice selected record positions must be unique.");

            auto const fieldOffset  = fieldPositions.front();
            auto const recordOffset = recordPositions.front();
            for(auto& position : fieldPositions) position -= fieldOffset;
            for(auto& position : recordPositions) position -= recordOffset;

            auto* base = original.data_handle();
            if(base != nullptr)
            {
                base += static_cast<std::ptrdiff_t>(recordOffset) * original.mapping().stride(0) + static_cast<std::ptrdiff_t>(fieldOffset) * original.mapping().stride(1);
            }

            return DFSlice(base, fields, records, fieldOffset, recordOffset, fieldPositions.size(), recordPositions.size(),
                           original.mapping().stride(1), original.mapping().stride(0), 1, 1,
                           std::move(fieldPositions), std::move(recordPositions));
        }

        /**
         * @brief   Creates a slice from already-resolved regular or gathered axis mappings.
         *
         * @details Position steps are folded into the physical strides once here. Iteration
         *          therefore performs the same address calculation for unit and non-unit ranges
         *          without separately evaluating `first + localPosition * step`.
         *
         * @param original        Source matrix.
         * @param fields          Field index borrowed for keyed access.
         * @param records         Record index borrowed for keyed access.
         * @param fldBegin        First absolute field position.
         * @param fldCount        Number of selected fields.
         * @param fldStep         Distance between regular field positions.
         * @param recBegin        First absolute record position.
         * @param recCount        Number of selected records.
         * @param recStep         Distance between regular record positions.
         * @param fieldPositions  Optional sorted field positions relative to `fldBegin`; overrides the regular field mapping.
         * @param recordPositions Optional sorted record positions relative to `recBegin`; overrides the regular record mapping.
         *
         * @pre Both steps are positive and every resolved position is within `original`.
         *
         * @return An empty slice when either count is zero; otherwise the resolved Cartesian region.
         */
        [[nodiscard]] static auto Mapped(Matrix original, FieldIndex const fields, RecordIndex const records,
                                         std::size_t const fldBegin, std::size_t const fldCount, std::size_t const fldStep,
                                         std::size_t const recBegin, std::size_t const recCount, std::size_t const recStep,
                                         Positions fieldPositions = {}, Positions recordPositions = {}) noexcept -> DFSlice
        {
            static_assert(!C, "Runtime slice mappings cannot guarantee contiguous storage in their type.");
            LUGIZMO_ASSERT(fldStep > 0 && recStep > 0, "DFSlice mapping steps must be positive.");
            if(fldCount == 0 || recCount == 0) return {};

            auto* base = original.data_handle();
            if(base != nullptr)
            {
                base += static_cast<std::ptrdiff_t>(recBegin) * original.mapping().stride(0) +
                        static_cast<std::ptrdiff_t>(fldBegin) * original.mapping().stride(1);
            }

            auto const physicalFieldStride = original.mapping().stride(1) * static_cast<std::ptrdiff_t>(fldStep);
            auto const physicalRecordStride = original.mapping().stride(0) * static_cast<std::ptrdiff_t>(recStep);
            return DFSlice(base, fields, records, fldBegin, recBegin, fldCount, recCount,
                           physicalFieldStride, physicalRecordStride, fldStep, recStep,
                           std::move(fieldPositions), std::move(recordPositions));
        }

        // ======== SUBSLICES =====================================================================================================================================================

        /**
         * @brief Creates a smaller slice from field and record keys selected by this slice.
         * @details Selection order is normalized to the source dataframe's physical order. The
         *          composed position mappings use the same PMR resources as their parent axes.
         * @return An empty slice when a selection is empty, contains duplicates, or requests a key
         *         that is unavailable or not selected by this slice.
         */
        template<typename FieldSelection, typename RecordSelection>
        requires DFSliceSelectionFor<FieldSelection, F> && DFSliceSelectionFor<RecordSelection, R>
        [[nodiscard]] auto Slice(FieldSelection&& fields, RecordSelection&& records) const noexcept -> DFSlice<T, F, R, Layout, false>
        {
            using Result = DFSlice<T, F, R, Layout, false>;

            auto fieldPositions  = ResolveSelection<F>(std::forward<FieldSelection>(fields), selectedFields.get_allocator().resource(), [this](auto const& key) { return LocalFieldPosition(key); });
            auto recordPositions = ResolveSelection<R>(std::forward<RecordSelection>(records), selectedRecords.get_allocator().resource(), [this](auto const& key) { return LocalRecordPosition(key); });
            if(!fieldPositions.has_value() || !recordPositions.has_value() || fieldPositions->empty() || recordPositions->empty()) return Result{};

            auto const firstFieldLocal  = fieldPositions->front();
            auto const firstRecordLocal = recordPositions->front();
            auto const firstFieldSource  = SourceFieldPosition(firstFieldLocal);
            auto const firstRecordSource = SourceRecordPosition(firstRecordLocal);

            for(auto& position : *fieldPositions) position = SourceFieldPosition(position) - firstFieldSource;
            for(auto& position : *recordPositions) position = SourceRecordPosition(position) - firstRecordSource;

            auto* base = data;
            if(base != nullptr)
            {
                base += static_cast<std::ptrdiff_t>(FieldPosition(firstFieldLocal)) * fieldStride +
                        static_cast<std::ptrdiff_t>(RecordPosition(firstRecordLocal)) * recordStride;
            }

            auto const sourceFieldStride = selectedFields.empty() ? fieldStride / static_cast<std::ptrdiff_t>(fieldPositionStep) : fieldStride;
            auto const sourceRecordStride = selectedRecords.empty() ? recordStride / static_cast<std::ptrdiff_t>(recordPositionStep) : recordStride;
            return Result(base, fieldIndex, recordIndex, firstFieldSource, firstRecordSource,
                          fieldPositions->size(), recordPositions->size(), sourceFieldStride, sourceRecordStride,
                          1, 1, std::move(*fieldPositions), std::move(*recordPositions));
        }

        /// @copydoc Slice(FieldSelection&&, RecordSelection&&)
        [[nodiscard]] auto Slice(std::initializer_list<FieldKey> const fields, std::initializer_list<RecordKey> const records) const noexcept
                -> DFSlice<T, F, R, Layout, false>
        {
            return Slice<std::initializer_list<FieldKey> const&, std::initializer_list<RecordKey> const&>(fields, records);
        }

        // ======== SHAPE AND LAYOUT ===============================================================================================================================================

        /// @return Number of selected fields.
        [[nodiscard]] constexpr auto FieldSize() const noexcept -> std::size_t { return fieldCount; }

        /// @return Number of selected records.
        [[nodiscard]] constexpr auto RecordSize() const noexcept -> std::size_t { return recordCount; }

        /// @return Number of selected values, equal to `FieldSize() * RecordSize()`.
        [[nodiscard]] constexpr auto Size() const noexcept -> std::size_t { return fieldCount * recordCount; }

        /// @return True when either selected axis is empty.
        [[nodiscard]] constexpr auto Empty() const noexcept -> bool { return fieldCount == 0 || recordCount == 0; }

        /// @return Physical element stride between consecutive positions of the regular field mapping.
        [[nodiscard]] constexpr auto FieldStride() const noexcept -> std::ptrdiff_t { return fieldStride; }

        /// @return Physical element stride between consecutive positions of the regular record mapping.
        [[nodiscard]] constexpr auto RecordStride() const noexcept -> std::ptrdiff_t { return recordStride; }

        /// @return True when the selected physical field positions contain no gaps.
        [[nodiscard]] constexpr auto IsFieldConsecutive() const noexcept -> bool
        {
            if(fieldCount <= 1) return true;
            if(selectedFields.empty()) return fieldPositionStep == 1;
            return selectedFields.back() == fieldCount - 1;
        }

        /// @return True when the selected physical record positions contain no gaps.
        [[nodiscard]] constexpr auto IsRecordConsecutive() const noexcept -> bool
        {
            if(recordCount <= 1) return true;
            if(selectedRecords.empty()) return recordPositionStep == 1;
            return selectedRecords.back() == recordCount - 1;
        }

        /// @return True when both field and record positions contain no gaps.
        [[nodiscard]] constexpr auto IsConsecutive() const noexcept -> bool
        {
            return IsFieldConsecutive() && IsRecordConsecutive();
        }

        /// @return True when all selected values occupy one uninterrupted memory region in iteration order.
        [[nodiscard]] constexpr auto IsContiguous() const noexcept -> bool { return C || ComputeContiguous(); }

        // ======== POSITIONAL ACCESS ==============================================================================================================================================

        /**
         * @brief Returns a value by its zero-based flattened storage-order position.
         * @pre `position < Size()`; checked only by `LUGIZMO_ASSERT`.
         */
        [[nodiscard]] constexpr auto operator[](std::size_t const position) const noexcept -> T&
        {
            LUGIZMO_ASSERT(position < Size(), "DFSlice flattened position is out of bounds.");
            return data[FlatOffset(position)];
        }

        /**
         * @brief Returns a value by its zero-based `(record, field)` position.
         * @pre `record < RecordSize()` and `field < FieldSize()`; checked only by `LUGIZMO_ASSERT`.
         */
        [[nodiscard]] constexpr auto operator[](std::size_t const record, std::size_t const field) const noexcept -> T&
        {
            LUGIZMO_ASSERT(record < recordCount && field < fieldCount, "DFSlice record or field position is out of bounds.");
            return data[static_cast<std::ptrdiff_t>(RecordPosition(record)) * recordStride +
                        static_cast<std::ptrdiff_t>(FieldPosition(field)) * fieldStride];
        }

        /// @return Pointer to `(record, field)`, or null when either local position is outside the slice.
        [[nodiscard]] constexpr auto operator()(std::size_t const record, std::size_t const field) const noexcept -> T*
        {
            if(record >= recordCount || field >= fieldCount) return nullptr;
            return data + static_cast<std::ptrdiff_t>(RecordPosition(record)) * recordStride +
                   static_cast<std::ptrdiff_t>(FieldPosition(field)) * fieldStride;
        }

        // ======== KEY ACCESS =====================================================================================================================================================

        /// @return True when both keys are attached to the slice and their positions are selected.
        [[nodiscard]] constexpr auto Contains(FieldKey const& field, RecordKey const& record) const noexcept -> bool
        { return LocalFieldPosition(field).has_value() && LocalRecordPosition(record).has_value(); }

        /// @return Pointer to the value at `(record, field)`, or null when either key is unavailable or unselected.
        [[nodiscard]] constexpr auto At(FieldKey const& field, RecordKey const& record) const noexcept -> T*
        {
            auto const fldPosition = LocalFieldPosition(field);
            auto const recPosition = LocalRecordPosition(record);
            if(!fldPosition.has_value() || !recPosition.has_value()) return nullptr;
            return (*this)(*recPosition, *fldPosition);
        }

        // ======== ITERATORS ======================================================================================================================================================
        // NOLINTBEGIN(readability-identifier-naming)

        /// @return Iterator to the first value in the physical storage order.
        [[nodiscard]] constexpr auto begin() const noexcept -> Iterator
        {
            if constexpr(C) return data;
            else return Iterator(data, recordCount, fieldCount, recordStride, fieldStride,
                                 selectedRecords.empty() ? nullptr : selectedRecords.data(),
                                 selectedFields.empty() ? nullptr : selectedFields.data());
        }

        /// @return Iterator one past the final value in the physical storage order.
        [[nodiscard]] constexpr auto end() const noexcept -> Iterator
        {
            if constexpr(C) return Empty() ? data : data + Size();
            else return Iterator(data, recordCount, fieldCount, recordStride, fieldStride,
                                 selectedRecords.empty() ? nullptr : selectedRecords.data(),
                                 selectedFields.empty() ? nullptr : selectedFields.data(), static_cast<std::ptrdiff_t>(Size()));
        }

        /// @return Iterator to the first value; element constness continues to follow `T`.
        [[nodiscard]] constexpr auto cbegin() const noexcept -> Iterator { return begin(); }

        /// @return Iterator one past the final value; element constness continues to follow `T`.
        [[nodiscard]] constexpr auto cend() const noexcept -> Iterator { return end(); }

        /// @return Reverse iterator to the final value in the physical storage order.
        [[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator { return reverse_iterator(end()); }

        /// @return Reverse iterator one past the first value in the physical storage order.
        [[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator { return reverse_iterator(begin()); }

        /// @copydoc rbegin()
        [[nodiscard]] constexpr auto crbegin() const noexcept -> reverse_iterator { return rbegin(); }

        /// @copydoc rend()
        [[nodiscard]] constexpr auto crend() const noexcept -> reverse_iterator { return rend(); }

        // NOLINTEND(readability-identifier-naming)
    };

} // namespace lugizmo

// ======== STANDARD RANGE CUSTOMIZATION ===========================================================================================================================================

template<typename T, typename F, typename R, typename Layout, bool C>
inline constexpr bool std::ranges::enable_borrowed_range<lugizmo::DFSlice<T, F, R, Layout, C>> = C; // NOLINT(readability-identifier-naming)

namespace lugizmo::internal {

    using DFSliceValidationIndex      = DFUniqueIndex<int>;
    using DFSliceValidation           = DFSlice<int, DFSliceValidationIndex, DFSliceValidationIndex, std::layout_right, false>;
    using DFSliceContiguousValidation = DFSlice<int, DFSliceValidationIndex, DFSliceValidationIndex, std::layout_right, true>;

    static_assert(std::ranges::random_access_range<DFSliceValidation>);
    static_assert(!std::ranges::contiguous_range<DFSliceValidation>);
    static_assert(std::ranges::contiguous_range<DFSliceContiguousValidation>);
    static_assert(std::ranges::sized_range<DFSliceValidation>);
    static_assert(std::ranges::common_range<DFSliceValidation>);
    static_assert(std::ranges::view<DFSliceValidation>);
    static_assert(!std::ranges::borrowed_range<DFSliceValidation>);
    static_assert(std::ranges::borrowed_range<DFSliceContiguousValidation>);

} // namespace lugizmo::internal

#endif // LUGIZMO_DF_SLICE_H
