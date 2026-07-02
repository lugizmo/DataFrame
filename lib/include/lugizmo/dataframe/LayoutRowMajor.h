// Filename: LayoutRowMajor.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_LAYOUT_ROW_MAJOR_H
#define LUGIZMO_DF_LAYOUT_ROW_MAJOR_H

#include <algorithm>
#include <memory>
#include <memory_resource>
#include <iterator>
#include <limits>
#include <mdspan>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "lugizmo/Assert.h"
#include "lugizmo/memory/Memory.h"
#include "lugizmo/container/Concepts.h"

namespace lugizmo {

    /**
     * @class DFRowMajor
     * @brief Row-major storage backend for `DataFrame`.
     *
     * @details
     * `DFRowMajor` manages a contiguous `T*` buffer interpreted as a 2D matrix
     * (`rowCount × colCount`) in row-major order (`layout_right`):
     *
     *   - row 0: `data[0 ... colCount-1]`
     *   - row 1: `data[colCount ... 2*colCount-1]`
     *   - ...
     *
     * The accompanying `MDSpan` is the authoritative shape (`rows`, `cols`), while
     * `capacity` is the number of allocated elements in the backing buffer.
     *
     * @par Lifetime model
     *   - Elements in the active range (`rows * cols`) are fully constructed.
     *   - Resize/reallocate paths construct destination elements first.
     *   - Removed/moved-from active elements are destroyed before deallocation.
     *   - For trivially destructible types, destruction loops compile out.
     *
     * @par Supported operations
     *   - Add/remove rows from begin/end (`ResizeRows`).
     *   - Add/remove columns from begin/end (`ResizeCols`).
     *   - Populate inserted rows with one value set (`span`/iterators) or row-by-row input.
     *   - Drop one row/column in-place.
     *   - Explicit free/reset of storage.
     *
     * @tparam T element type stored in the layout.
     *
     * @note
     *   - Exception-free design, guarded by `LUGIZMO_ASSERT(_TRACE)`.
     *   - Not thread-safe for concurrent mutation.
     *   - Any reallocation or dimension-changing mutation invalidates references/views/iterators.
     */
    template <typename T>
    struct DFRowMajor final
    {
        using Memory = std::pmr::memory_resource;
        using Layout = std::layout_right;
        using MDSpan = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

        static constexpr bool IS_ROW_MAJOR = true;
        static constexpr bool IS_COL_MAJOR = false;

    private:

        /// @brief Maximum active byte size for which reorder prefers a full-buffer copy.
        static constexpr size_t REORDER_FULL_COPY_THRESHOLD_BYTES = 64UZ * 1024UZ * 1024UZ;

        // ======= HELPERS: SHAPE AND CAPACITY HELPERS =============================================================================================================================

        /**
         * @brief     Returns the current row count from mdspan extents.
         * @param[in] dataView Current data view.
         *
         * TODO Is it actually possible that the column is negative? Maybe just do a static_cast.
         * @return Non-negative row size as `size_t`.
         */
        static auto RowCount(MDSpan const& dataView) noexcept -> size_t
        {
            LUGIZMO_ASSERT_TRACE(dataView.extent(0) >= 0, "DFRowMajor should not have a negative row count.");
            return dataView.extent(0) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(0));
        }

        /**
         * @brief     Returns the current column count from mdspan extents.
         * @param[in] dataView Current data view.
         *
         * TODO Is it actually possible that the row is negative? Maybe just do a static_cast.
         * @return Non-negative columns count as `size_t`.
         */
        static auto ColCount(MDSpan const& dataView) noexcept -> size_t
        {
            LUGIZMO_ASSERT_TRACE(dataView.extent(1) >= 0, "DFRowMajor should not have a negative column count.");
            return dataView.extent(1) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(1));
        }

        /**
         * @brief Returns the number of active elements for the given shape.
         *
         * @param[in] rowCount The row count.
         * @param[in] colCount The column count.
         * @return `rowCount * colCount`.
         */
        static auto ActiveCount(size_t const rowCount, size_t const colCount) noexcept -> size_t
        {
            if(constexpr auto max = std::numeric_limits<size_t>::max(); colCount != 0 && rowCount > max / colCount)
            {
                LUGIZMO_ASSERT_TRACE(false, "DFRowMajor shape exceeds the representable element count.");
                return max;
            }

            return rowCount * colCount;
        }

        /**
         * @brief Returns the  number of active elements for the current mdspan shape.
         *
         * @param[in] dataView Current data view.
         * @return Number of active constructed elements.
         */
        static auto ActiveCount(MDSpan const& dataView) noexcept -> size_t
        {
            return ActiveCount(RowCount(dataView), ColCount(dataView));
        }

        /**
         * @brief Returns the number of bytes occupied by the active element range.
         *
         * @param[in] rowCount The row count.
         * @param[in] colCount The column count.
         * @return `rowCount * colCount * sizeof(T)`.
         */
        static auto ActiveBytes(size_t const rowCount, size_t const colCount) noexcept -> size_t
        {
            return internal::CheckedElementBytes<T>(ActiveCount(rowCount, colCount));
        }

        /**
         * @brief Computes target capacity for a required element count.
         *
         * @details
         * Reuses existing capacity if it already fits; otherwise uses the internal
         * growth policy and clamps to at least `requiredCount`.
         *
         * @param[in] currentCapacity Currently allocated element count.
         * @param[in] requiredCount   Required element count.
         * @return Next capacity in elements.
         */
        static auto NextCapacity(size_t const currentCapacity, size_t const requiredCount) noexcept -> size_t
        {
            if(requiredCount == 0) return 0;
            if(currentCapacity >= requiredCount && currentCapacity != 0) return currentCapacity;

            return std::max(requiredCount, internal::GrowthFactorDefault(requiredCount));
        }

        // ======= HELPERS: LIFETIME ===============================================================================================================================================

        /**
         * @brief Destroys `count` elements when `T` is non-trivially destructible.
         *
         * @param[in,out] data  Pointer to the first element of the range.
         * @param[in]     count Number of elements to destroy.
         */
        static void DestroyRange(T* const data, size_t const count) noexcept
        {
            if constexpr(not std::is_trivially_destructible_v<T>)
            {
                if(data == nullptr || count == 0) return;
                std::destroy_n(data, count);
            }
        }

        /**
         * @brief Destroys active elements and deallocates the backing buffer.
         *
         * @details
         * Main dataframe storage is expected to follow Lugizmo's aligned allocation
         * contract. Callers that exercise this backend directly should therefore
         * allocate storage with `internal::AllocateAligned<T>(...)`.
         *
         * @param[in,out] data        Pointer to backing storage.
         * @param[in]     activeCount Number of active constructed elements.
         * @param[in]     capacity    Allocated element count.
         * @param[in,out] res         Memory resource used for deallocation.
         */
        static void DestroyAndDeallocate(T*& data, size_t const activeCount, size_t const capacity, Memory& res) noexcept
        {
            DestroyRange(data, activeCount);
            if(data != nullptr && capacity != 0) res.deallocate(data, internal::CheckedElementBytes<T>(capacity), internal::Alignment<T>());
        }

        // ======= HELPERS: ASSIGNMENT/MOVE ON CONSTRUCTED RANGES ==================================================================================================================

        /**
         * @brief Assigns/moves `count` elements from `src` to `dst` in forward order.
         *
         * @details
         * Uses move-assignment when available, otherwise copy-assignment.
         *
         * @param[in,out] dst   Destination range start.
         * @param[in,out] src   Source range start.
         * @param[in]     count Number of elements to assign.
         */
        static void AssignForward(T* const dst, T* const src, size_t const count) noexcept
        {
            if(count == 0 || dst == src) return;

            if constexpr(std::is_move_assignable_v<T>)
            {
                for(size_t i = 0; i < count; ++i) dst[i] = std::move(src[i]);
            }
            else
            {
                static_assert(std::is_copy_assignable_v<T>, "DFRowMajor requires move-assignable or copy-assignable element types.");
                for(size_t i = 0; i < count; ++i) dst[i] = src[i];
            }
        }

        /**
         * @brief Assigns/moves `count` elements from `src` to `dst` in backward order.
         *
         * @details
         * Backward order is used for overlapping ranges when the destination starts after
         * the source. Uses move-assignment when available, otherwise copy-assignment.
         *
         * @param[in,out] dst   Destination range start.
         * @param[in,out] src   Source range start.
         * @param[in]     count Number of elements to assign.
         */
        static void AssignBackward(T* const dst, T* const src, size_t const count) noexcept
        {
            if(count == 0 || dst == src) return;

            if constexpr(std::is_move_assignable_v<T>)
            {
                for(size_t i = count; i > 0; --i) dst[i - 1] = std::move(src[i - 1]);
            }
            else
            {
                static_assert(std::is_copy_assignable_v<T>, "DFRowMajor requires move-assignable or copy-assignable element types.");
                for(size_t i = count; i > 0; --i) dst[i - 1] = src[i - 1];
            }
        }

        // ======= HELPERS: CONSTRUCTION HELPERS ON UNINITIALIZED RANGES ===========================================================================================================

        /**
         * @brief Constructs `count` destination elements from source range.
         *
         * @details
         * Uses move-construction when available, otherwise copy-construction.
         *
         * @param[in,out] src   Source range start.
         * @param[in]     count Number of elements to construct.
         * @param[in,out] dst   Destination range start (uninitialized memory).
         */
        static void UninitializedMoveOrCopy(T* const src, size_t const count, T* const dst)
        {
            if(count == 0) return;

            if constexpr(std::is_move_constructible_v<T>)
            {
                std::uninitialized_move_n(src, count, dst);
            }
            else
            {
                static_assert(std::is_copy_constructible_v<T>, "DFRowMajor requires move-constructible or copy-constructible element types.");
                std::uninitialized_copy_n(src, count, dst);
            }
        }

        /**
         * @brief Constructs rows filled with one scalar value.
         *
         * @param[in,out] data         Destination storage (uninitialized for target rows).
         * @param[in]     startRow     First destination row index.
         * @param[in]     numRows      Number of rows to construct.
         * @param[in]     colCount     Columns per row.
         * @param[in]     defaultValue Value copied into each element.
         */
        static void ConstructRowsDefault(T* const data, size_t const startRow, size_t const numRows, size_t const colCount, T const& defaultValue)
        {
            if(colCount == 0 || numRows == 0) return;

            for(size_t row = 0; row < numRows; ++row)
            {
                auto* const start = data + (startRow + row) * colCount;
                std::uninitialized_fill_n(start, colCount, defaultValue);
            }
        }

        /**
         * @brief Constructs rows by copying one source row range for each row.
         *
         * @param[in,out] data     Destination storage (uninitialized for target rows).
         * @param[in]     startRow First destination row index.
         * @param[in]     numRows  Number of rows to construct.
         * @param[in]     colCount Columns per row.
         * @param[in]     begin    Iterator to first source value.
         * @param[in]     end      Iterator one-past-last source value.
         */
        template <typename InputIterator>
        static void ConstructRowsFromValues(T* const data, size_t const startRow, size_t const numRows, size_t const colCount, InputIterator const begin, InputIterator const end)
        {
            if(colCount == 0 || numRows == 0) return;
            LUGIZMO_ASSERT_TRACE(std::distance(begin, end) == static_cast<std::ptrdiff_t>(colCount), "ConstructRowsFromValues received mismatched column count.");

            for(size_t row = 0; row < numRows; ++row)
            {
                auto* const start = data + (startRow + row) * colCount;
                std::uninitialized_copy(begin, end, start);
            }
        }

        /**
         * @brief Constructs rows from an iterator-of-rows source.
         *
         * @details
         * Consumes at most `numRows` entries from `[rowBegin, rowEnd)` and advances
         * `rowBegin` by the number of rows consumed.
         *
         * @tparam RowsIterator Iterator over row containers.
         *
         * @param[in,out] data     Destination storage (uninitialized for target rows).
         * @param[in]     startRow First destination row index.
         * @param[in]     numRows  Maximum number of rows to construct.
         * @param[in]     colCount Columns per row.
         * @param[in,out] rowBegin Current input row iterator; advanced as rows are consumed.
         * @param[in]     rowEnd   End iterator of the row input range.
         *
         * @return Number of rows actually constructed.
         */
        template <typename RowsIterator>
        static auto ConstructRowsByRow(T* const data, size_t const startRow, size_t const numRows, size_t const colCount, RowsIterator& rowBegin, RowsIterator const rowEnd) -> size_t
        {
            if(colCount == 0 || numRows == 0) return 0;

            auto constructedRows = 0UZ;
            for(size_t row = 0; row < numRows; ++row)
            {
                if(rowBegin == rowEnd) break;

                ConstructRowsFromValues(data, startRow + row, 1, colCount, rowBegin->begin(), rowBegin->end());
                ++rowBegin;
                ++constructedRows;
            }

            return constructedRows;
        }

    public:

        // ======= ADJUST COLUMNS/ROWS BY COUNT ====================================================================================================================================

        /**
         * @brief Resizes columns by applying to begin/end adjustments.
         *
         * @details
         * Positive adjustment adds columns, negative adjustment removes columns.
         * The function computes the overlap between old and new column ranges,
         * allocates destination storage, then:
         *   1. constructs prefix added columns with `defaultValue`,
         *   2. moves/copies overlapping source columns,
         *   3. constructs suffix added columns with `defaultValue`.
         *
         * Old active elements are destroyed and old storage is deallocated after
         * destination construction has completed.
         *
         * @param[in,out] data          Pointer to backing storage.
         * @param[in,out] capacity      Number of allocated elements in `data`.
         * @param[in,out] res           Memory resource used for allocation/deallocation.
         * @param[in,out] dataView      Current matrix view; updated to the new shape.
         * @param[in]     adjCountByBeg Signed adjustment at the column beginning.
         * @param[in]     adjCountByEnd Signed adjustment at the column end.
         * @param[in]     defaultValue  Value used to initialize newly added columns.
         */
        static void ResizeCols(T*& data, size_t& capacity, Memory& res, MDSpan& dataView,
                               ssize_t const adjCountByBeg, ssize_t const adjCountByEnd,
                               T const& defaultValue) noexcept
        {
            // Step 0: early-out for no-op requests
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            // Step 1: read the current shape and validate the requested new column count
            auto const rowCount    = RowCount(dataView);
            auto const colCount    = ColCount(dataView);
            auto const newColCount = static_cast<ssize_t>(colCount) + adjCountByBeg + adjCountByEnd;

            if(newColCount < 0)
            {
                LUGIZMO_ASSERT_TRACE(newColCount >= 0, "ResizeCols computed a negative column count.");
                return; // NOLINT
            }

            // Step 2: derived size values for old/new active element ranges
            auto const newColCountPos = static_cast<size_t>(newColCount);
            auto const oldActiveCount = ActiveCount(rowCount, colCount);
            auto const newActiveCount = ActiveCount(rowCount, newColCountPos);

            // Step 3a: if all columns are removed, fully free active storage
            if(newColCountPos == 0)
            {
                DestroyAndDeallocate(data, oldActiveCount, capacity, res);

                data     = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, rowCount, 0};
                return;
            }

            // Step 3b: shape-only change when there are no rows to materialize
            if(rowCount == 0)
            {
                dataView = MDSpan{data, rowCount, newColCountPos};
                return;
            }

            // Step 4: compute overlap mapping between old and new column ranges
            // Group A: source overlap [srcStartS, safeSrcEndS) in the old column space
            auto const colCountS   = static_cast<ssize_t>(colCount);
            auto const srcStartS   = std::max<ssize_t>(0, -adjCountByBeg);
            auto const srcEndS     = std::min<ssize_t>(colCountS, newColCount - adjCountByBeg);
            auto const safeSrcEndS = std::max(srcStartS, srcEndS);

            // Group B: compact derived overlap lengths/offsets
            auto const movedColCount = static_cast<size_t>(safeSrcEndS - srcStartS);
            auto const srcStart      = static_cast<size_t>(srcStartS);

            // Group C: destination prefix/suffix added regions
            auto const dstOldStartS = srcStartS + adjCountByBeg;
            auto const addBeg       = static_cast<size_t>(std::clamp<ssize_t>(dstOldStartS, 0, newColCount));
            auto const addEndS      = newColCount - static_cast<ssize_t>(addBeg) - static_cast<ssize_t>(movedColCount);
            if(addEndS < 0)
            {
                LUGIZMO_ASSERT_TRACE(addEndS >= 0, "ResizeCols produced an invalid trailing column count.");
                return; // NOLINT
            }
            auto const addEnd = static_cast<size_t>(addEndS);

            LUGIZMO_ASSERT_TRACE(addBeg + movedColCount + addEnd == newColCountPos, "ResizeCols produced inconsistent column mapping.");

            // Step 5: allocate destination buffer
            auto const newCapacity = NextCapacity(capacity, newActiveCount);
            auto* const newData    = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(newCapacity), internal::Alignment<T>()));
            LUGIZMO_ASSERT_TRACE(newData != nullptr, "ResizeCols failed to allocate destination buffer.");

            // Step 6: build each destination row (prefix defaults, overlap move/copy, suffix defaults)
            for(size_t row = 0; row < rowCount; ++row)
            {
                auto* const srcRow = data + row * colCount + srcStart;
                auto* const dstRow = newData + row * newColCountPos;

                std::uninitialized_fill_n(dstRow, addBeg, defaultValue);
                UninitializedMoveOrCopy(srcRow, movedColCount, dstRow + addBeg);
                std::uninitialized_fill_n(dstRow + addBeg + movedColCount, addEnd, defaultValue);
            }

            // Step 7: tear down the old active range and publish new storage/view
            DestroyAndDeallocate(data, oldActiveCount, capacity, res);

            data     = newData;
            capacity = newCapacity;
            dataView = MDSpan{data, rowCount, newColCountPos};
        }


        /**
         * @brief Resizes rows and initializes added rows with one default value.
         *
         * @details
         * Positive adjustments add rows, negative adjustments remove rows.
         * The function computes overlap between old and new row ranges, allocates
         * destination storage, then:
         *   1. constructs new leading rows with `defaultValue`,
         *   2. moves/copies overlapping old rows,
         *   3. constructs new trailing rows with `defaultValue`.
         *
         * Old active elements are destroyed after destination construction.
         *
         * Difference to `ResizeRows(..., Values const&)`:
         *   - this overload uses one scalar `defaultValue` for each inserted element,
         *   - no row-input shape validation is required.
         *
         * @param[in,out] data          Pointer to backing storage.
         * @param[in,out] capacity      Number of allocated elements in `data`.
         * @param[in,out] res           Memory resource used for allocation/deallocation.
         * @param[in,out] dataView      Current matrix view; updated to the new shape.
         * @param[in]     adjCountByBeg Signed adjustment at row begin.
         * @param[in]     adjCountByEnd Signed adjustment at row end.
         * @param[in]     defaultValue  Value used to initialize newly added rows.
         */
        static void ResizeRows(T*& data, size_t& capacity, Memory& res, MDSpan& dataView,
                               ssize_t const adjCountByBeg, ssize_t const adjCountByEnd, T const& defaultValue) noexcept
        {
            // Step 0: early-out for no-op requests
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            // Step 1: read the current shape and validate the requested new row count
            auto const colCount    = ColCount(dataView);
            auto const rowCount    = RowCount(dataView);
            auto const newRowCount = static_cast<ssize_t>(rowCount) + adjCountByBeg + adjCountByEnd;

            if(newRowCount == 0)
            {
                Free(data, capacity, dataView, res);
                return;
            }

            if(newRowCount < 0)
            {
                LUGIZMO_ASSERT_TRACE(newRowCount >= 0, "ResizeRows computed a negative row count.");
                return; // NOLINT
            }

            // Step 2: derived size values for old/new active element ranges
            auto const newRowCountPos = static_cast<size_t>(newRowCount);
            auto const oldActiveCount = ActiveCount(rowCount, colCount);
            auto const newActiveCount = ActiveCount(newRowCountPos, colCount);

            // Step 3: compute overlap mapping between old and new row ranges
            // Group A: source overlap [srcStartS, safeSrcEndS) in old row space
            auto const rowCountS   = static_cast<ssize_t>(rowCount);
            auto const srcStartS   = std::max<ssize_t>(0, -adjCountByBeg);
            auto const srcEndS     = std::min<ssize_t>(rowCountS, newRowCount - adjCountByBeg);
            auto const safeSrcEndS = std::max(srcStartS, srcEndS);

            // Group B: compact derived overlap lengths/offsets
            auto const keptRows   = static_cast<size_t>(safeSrcEndS - srcStartS);
            auto const srcStart   = static_cast<size_t>(srcStartS);

            // Group C: destination prefix/suffix added regions
            auto const dstOldStartS = srcStartS + adjCountByBeg;
            auto const addBeg     = static_cast<size_t>(std::clamp<ssize_t>(dstOldStartS, 0, newRowCount));
            auto const addEndS    = newRowCount - static_cast<ssize_t>(addBeg) - static_cast<ssize_t>(keptRows);
            if(addEndS < 0)
            {
                LUGIZMO_ASSERT_TRACE(addEndS >= 0, "ResizeRows produced an invalid trailing row count.");
                return; // NOLINT
            }
            auto const addEnd = static_cast<size_t>(addEndS);

            LUGIZMO_ASSERT_TRACE(addBeg + keptRows + addEnd == newRowCountPos, "ResizeRows produced inconsistent row mapping.");

            // Step 4: shape-only change for zero-column views
            if(colCount == 0)
            {
                dataView = MDSpan{data, newRowCountPos, colCount};
                return;
            }

            // Step 5: allocate destination buffer
            auto const newCapacity = NextCapacity(capacity, newActiveCount);
            auto* const newData    = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(newCapacity), internal::Alignment<T>()));
            LUGIZMO_ASSERT_TRACE(newData != nullptr, "ResizeRows failed to allocate destination buffer.");

            // Step 6a: construct inserted leading rows
            ConstructRowsDefault(newData, 0, addBeg, colCount, defaultValue);

            // Step 6b: move/copy overlapping rows
            for(size_t row = 0; row < keptRows; ++row)
            {
                auto* const srcRow = data + (srcStart + row) * colCount;
                auto* const dstRow = newData + (addBeg + row) * colCount;
                UninitializedMoveOrCopy(srcRow, colCount, dstRow);
            }

            // Step 6c: construct inserted trailing rows
            ConstructRowsDefault(newData, addBeg + keptRows, addEnd, colCount, defaultValue);

            // Step 7: tear down the old active range and publish new storage/view
            DestroyAndDeallocate(data, oldActiveCount, capacity, res);

            data     = newData;
            capacity = newCapacity;
            dataView = MDSpan{data, newRowCountPos, colCount};
        }

        /**
         * @brief Resizes rows and initializes added rows from provided value rows.
         *
         * @details
         * Supports two input forms:
         *   - one row of values reused for each inserted row,
         *   - iterable of rows consumed in order for inserted rows.
         *
         * Structural flow mirrors `ResizeRows(..., defaultValue)`: compute overlap,
         * allocate destination, construct inserted rows from `values`, move/copy overlap,
         * then destroy old active elements.
         *
         * Difference to `ResizeRows(..., T const& defaultValue)`:
         *   - this overload initializes inserted rows from caller-provided row values,
         *   - input row count/width is validated before construction.
         *
         * @tparam Values row-source container or iterable.
         *
         * @param[in,out] data          Pointer to backing storage.
         * @param[in,out] capacity      Number of allocated elements in `data`.
         * @param[in,out] res           Memory resource used for allocation/deallocation.
         * @param[in,out] dataView      Current matrix view; updated to the new shape.
         * @param[in]     adjCountByBeg Signed adjustment at the row beginning.
         * @param[in]     adjCountByEnd Signed adjustment at the row end.
         * @param[in]     values        Source row data used for newly inserted rows.
         */
        template <typename Values> requires MinimalIterable<Values>
        static void ResizeRows(T*& data, size_t& capacity, Memory& res, MDSpan& dataView,
                               ssize_t const adjCountByBeg, ssize_t const adjCountByEnd, Values const& values) noexcept
        {
            constexpr bool isItOfIt = MinimalIterableOfIterable<Values>;

            // Step 0: early-out for no-op requests
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            // Step 1: read the current shape and validate the requested new row count
            auto const colCount      = ColCount(dataView);
            auto const rowCount      = RowCount(dataView);
            auto const newRowCountS  = static_cast<ssize_t>(rowCount) + adjCountByBeg + adjCountByEnd;

            if(newRowCountS == 0)
            {
                Free(data, capacity, dataView, res);
                return;
            }

            if(newRowCountS < 0)
            {
                LUGIZMO_ASSERT_TRACE(newRowCountS >= 0, "ResizeRows(values) computed a negative row count.");
                return; // NOLINT
            }

            // Step 2: derive overlap/prefix/suffix ranges
            // Group A: source overlap [srcStartS, safeSrcEndS) in the old row space
            auto const newRowCountPos = static_cast<size_t>(newRowCountS);
            auto const rowCountS      = static_cast<ssize_t>(rowCount);
            auto const srcStartS      = std::max<ssize_t>(0, -adjCountByBeg);
            auto const srcEndS        = std::min<ssize_t>(rowCountS, newRowCountS - adjCountByBeg);
            auto const safeSrcEndS    = std::max(srcStartS, srcEndS);

            // Group B: compact derived overlap lengths/offsets
            auto const keptRows       = static_cast<size_t>(safeSrcEndS - srcStartS);
            auto const srcStart       = static_cast<size_t>(srcStartS);

            // Group C: destination prefix/suffix added regions
            auto const dstOldStartS   = srcStartS + adjCountByBeg;
            auto const addBeg         = static_cast<size_t>(std::clamp<ssize_t>(dstOldStartS, 0, newRowCountS));
            auto const addEndS        = newRowCountS - static_cast<ssize_t>(addBeg) - static_cast<ssize_t>(keptRows);
            if(addEndS < 0)
            {
                LUGIZMO_ASSERT_TRACE(addEndS >= 0, "ResizeRows(values) produced an invalid trailing row count.");
                return; // NOLINT
            }
            auto const addEnd         = static_cast<size_t>(addEndS);
            auto const rowsToAdd      = addBeg + addEnd;

            // Step 3: validate input row data only when rows are being inserted
            if(rowsToAdd > 0)
            {
                if constexpr(isItOfIt)
                {
                    auto const valuesCount = static_cast<size_t>(std::ranges::distance(values));
                    if(valuesCount < rowsToAdd) return;
                    if(valuesCount == 0) return;
                    if(std::ranges::distance(*std::begin(values)) != static_cast<std::ptrdiff_t>(colCount)) return;
                    if(not std::ranges::all_of(values, [&](auto const& row) { return row.size() == colCount; })) return;
                }
                else if(values.size() != colCount) return;
            }

            // Step 4: derived size values for old/new active element ranges
            auto const oldActiveCount = ActiveCount(rowCount, colCount);
            auto const newActiveCount = ActiveCount(newRowCountPos, colCount);

            LUGIZMO_ASSERT_TRACE(addBeg + keptRows + addEnd == newRowCountPos, "ResizeRows(values) produced inconsistent row mapping.");

            // Step 5: shape-only change for zero-column views
            if(colCount == 0)
            {
                dataView = MDSpan{data, newRowCountPos, colCount};
                return;
            }

            // Step 6: allocate destination buffer
            auto const newCapacity = NextCapacity(capacity, newActiveCount);
            auto* const newData    = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(newCapacity), internal::Alignment<T>()));
            LUGIZMO_ASSERT_TRACE(newData != nullptr, "ResizeRows(values) failed to allocate destination buffer.");

            // Step 7a: construct inserted rows from an input value source
            if(rowsToAdd > 0)
            {
                if constexpr(isItOfIt)
                {
                    auto inputRowIt = std::begin(values);
                    auto const begRows = ConstructRowsByRow(newData, 0, addBeg, colCount, inputRowIt, std::end(values));
                    auto const endRows = ConstructRowsByRow(newData, addBeg + keptRows, addEnd, colCount, inputRowIt, std::end(values));
                    if constexpr(not AssertTraceEnabled())
                    {
                        (void)begRows;
                        (void)endRows;
                    }
                    LUGIZMO_ASSERT_TRACE(begRows == addBeg && endRows == addEnd, "ResizeRows(values) did not receive enough rows to initialize inserted records.");
                }
                else
                {
                    ConstructRowsFromValues(newData, 0, addBeg, colCount, values.begin(), values.end());
                    ConstructRowsFromValues(newData, addBeg + keptRows, addEnd, colCount, values.begin(), values.end());
                }
            }

            // Step 7b: move/copy overlap from old data
            for(size_t row = 0; row < keptRows; ++row)
            {
                auto* const srcRow = data + (srcStart + row) * colCount;
                auto* const dstRow = newData + (addBeg + row) * colCount;
                UninitializedMoveOrCopy(srcRow, colCount, dstRow);
            }

            // Step 8: tear down the old active range and publish new storage/view
            DestroyAndDeallocate(data, oldActiveCount, capacity, res);

            data     = newData;
            capacity = newCapacity;
            dataView = MDSpan{data, newRowCountPos, colCount};
        }

        // ======= DROP COLUMNS/ROWS ===============================================================================================================================================

        /**
         * @brief Removes one row by index and compacts the remaining rows.
         *
         * @details
         * The trailing row range is moved one row toward the front, then the now-unused
         * last row is destroyed. Capacity is unchanged.
         *
         * @param[in,out] data        Pointer to backing storage.
         * @param[in,out] dataView    Current matrix view; row extent is decremented.
         * @param[in]     rowToRemove Row index to remove.
         */
        static void DropRow(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const rowToRemove) noexcept
        {
            // Step 1: read and validate the current shape
            auto const colCount = ColCount(dataView);
            auto const rowCount = RowCount(dataView);

            LUGIZMO_ASSERT_TRACE(rowToRemove < rowCount, "DropRow received a row index out of bounds.");
            if(rowToRemove >= rowCount) return;

            // Step 2: zero-column views only need a shape update
            if(colCount == 0)
            {
                dataView = MDSpan{data, rowCount - 1, colCount};
                return;
            }

            // Step 3: shift the trailing rows forward and destroy the last row
            auto const removeStart = rowToRemove * colCount;
            auto const tailCount   = (rowCount - rowToRemove - 1) * colCount;

            if(tailCount > 0) AssignForward(data + removeStart, data + removeStart + colCount, tailCount);
            DestroyRange(data + (rowCount - 1) * colCount, colCount);

            // Step 4: publish updated shape
            dataView = MDSpan{data, rowCount - 1, colCount};
        }

        /**
         * @brief Removes one column by index and compacts each row.
         *
         * @details
         * For each row, values after the removed column are shifted left by one slot.
         * The trailing compacted tail is destroyed. Capacity is unchanged.
         *
         * @param[in,out] data        Pointer to backing storage.
         * @param[in,out] dataView    Current matrix view; column extent is decremented.
         * @param[in]     colToRemove Column index to remove.
         */
        static void DropColumn(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const colToRemove) noexcept
        {
            // Step 1: read and validate the current shape
            auto const colCount = ColCount(dataView);
            auto const rowCount = RowCount(dataView);

            LUGIZMO_ASSERT_TRACE(colToRemove < colCount, "DropColumn received a column index out of bounds.");
            if(colToRemove >= colCount) return;

            // Step 2: derived size values for old/new active element ranges
            auto const newColCount = colCount - 1;
            auto const oldActive   = ActiveCount(rowCount, colCount);
            auto const newActive   = ActiveCount(rowCount, newColCount);

            // Step 3: compact each row in-place
            for(size_t row = 0; row < rowCount; ++row)
            {
                auto* const oldRow = data + row * colCount;
                auto* const newRow = data + row * newColCount;

                AssignForward(newRow, oldRow, colToRemove);
                AssignForward(newRow + colToRemove, oldRow + colToRemove + 1, colCount - colToRemove - 1);
            }

            // Step 4: destroy trailing now-unused elements and publish the updated shape
            DestroyRange(data + newActive, oldActive - newActive);
            dataView = MDSpan{data, rowCount, newColCount};
        }

        /**
         * @brief Reorders rows according to a new-to-old permutation.
         *
         * @param[in,out] data      Pointer to backing storage.
         * @param[in,out] capacity  Allocated element count.
         * @param[in,out] res       Memory resource used for allocation.
         * @param[in,out] dataView  Current mdspan view to rebuild after reordering.
         * @param[in]     newToOld  Permutation where each new row points to its old row index.
         */
        static void ReorderRows(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, std::span<size_t const> const newToOld) noexcept
        {
            auto const rowCount    = RowCount(dataView);
            auto const colCount    = ColCount(dataView);
            auto const activeCount = ActiveCount(rowCount, colCount);
            auto const activeBytes = ActiveBytes(rowCount, colCount);

            LUGIZMO_ASSERT_TRACE(newToOld.size() == rowCount, "ReorderRows received a permutation with mismatched row count.");
            if(newToOld.size() != rowCount || activeCount == 0) return;

            if(activeBytes <= REORDER_FULL_COPY_THRESHOLD_BYTES)
            {
                auto* const newData = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(capacity), internal::Alignment<T>()));
                LUGIZMO_ASSERT_TRACE(newData != nullptr, "ReorderRows failed to allocate destination buffer.");

                for(size_t newRow = 0; newRow < rowCount; ++newRow)
                {
                    auto const oldRow = newToOld[newRow];
                    LUGIZMO_ASSERT_TRACE(oldRow < rowCount, "ReorderRows received an out-of-bounds row permutation entry.");

                    auto* const srcRow = data + oldRow * colCount;
                    auto* const dstRow = newData + newRow * colCount;
                    UninitializedMoveOrCopy(srcRow, colCount, dstRow);
                }

                DestroyAndDeallocate(data, activeCount, capacity, res);
                data     = newData;
                dataView = MDSpan{data, rowCount, colCount};
                return;
            }

            auto* const scratch = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(colCount), internal::Alignment<T>()));
            LUGIZMO_ASSERT_TRACE(scratch != nullptr, "ReorderRows failed to allocate scratch row.");

            auto visited = std::pmr::vector<unsigned char>(&res);
            visited.resize(rowCount, 0);

            for(size_t start = 0; start < rowCount; ++start)
            {
                if(visited[start] != 0 || newToOld[start] == start)
                {
                    visited[start] = 1;
                    continue;
                }

                auto* const startRow = data + start * colCount;
                UninitializedMoveOrCopy(startRow, colCount, scratch);

                auto current = start;
                while(newToOld[current] != start)
                {
                    auto const source = newToOld[current];
                    LUGIZMO_ASSERT_TRACE(source < rowCount, "ReorderRows received an out-of-bounds row permutation entry.");

                    auto* const srcRow = data + source * colCount;
                    auto* const dstRow = data + current * colCount;
                    AssignForward(dstRow, srcRow, colCount);

                    visited[current] = 1;
                    current = source;
                }

                auto* const dstRow = data + current * colCount;
                AssignForward(dstRow, scratch, colCount);
                visited[current] = 1;
                DestroyRange(scratch, colCount);
            }

            res.deallocate(scratch, internal::CheckedElementBytes<T>(colCount), internal::Alignment<T>());
            dataView = MDSpan{data, rowCount, colCount};
        }

        /**
         * @brief Reorders columns according to a new-to-old permutation.
         *
         * @param[in,out] data      Pointer to backing storage.
         * @param[in,out] capacity  Allocated element count.
         * @param[in,out] res       Memory resource used for allocation.
         * @param[in,out] dataView  Current mdspan view to rebuild after reordering.
         * @param[in]     newToOld  Permutation where each new column points to its old column index.
         */
        static void ReorderColumns(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, std::span<size_t const> const newToOld) noexcept
        {
            auto const rowCount    = RowCount(dataView);
            auto const colCount    = ColCount(dataView);
            auto const activeCount = ActiveCount(rowCount, colCount);
            auto const activeBytes = ActiveBytes(rowCount, colCount);

            LUGIZMO_ASSERT_TRACE(newToOld.size() == colCount, "ReorderColumns received a permutation with mismatched column count.");
            if(newToOld.size() != colCount || activeCount == 0) return;

            if(activeBytes <= REORDER_FULL_COPY_THRESHOLD_BYTES)
            {
                auto* const newData = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(capacity), internal::Alignment<T>()));
                LUGIZMO_ASSERT_TRACE(newData != nullptr, "ReorderColumns failed to allocate destination buffer.");

                for(size_t row = 0; row < rowCount; ++row)
                {
                    auto* const srcRow = data + row * colCount;
                    auto* const dstRow = newData + row * colCount;

                    for(size_t newCol = 0; newCol < colCount; ++newCol)
                    {
                        auto const oldCol = newToOld[newCol];
                        LUGIZMO_ASSERT_TRACE(oldCol < colCount, "ReorderColumns received an out-of-bounds column permutation entry.");
                        UninitializedMoveOrCopy(srcRow + oldCol, 1, dstRow + newCol);
                    }
                }

                DestroyAndDeallocate(data, activeCount, capacity, res);
                data     = newData;
                dataView = MDSpan{data, rowCount, colCount};
                return;
            }

            auto* const scratch = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(colCount), internal::Alignment<T>()));
            LUGIZMO_ASSERT_TRACE(scratch != nullptr, "ReorderColumns failed to allocate scratch row.");

            for(size_t row = 0; row < rowCount; ++row)
            {
                auto* const currentRow = data + row * colCount;

                for(size_t newCol = 0; newCol < colCount; ++newCol)
                {
                    auto const oldCol = newToOld[newCol];
                    LUGIZMO_ASSERT_TRACE(oldCol < colCount, "ReorderColumns received an out-of-bounds column permutation entry.");
                    UninitializedMoveOrCopy(currentRow + oldCol, 1, scratch + newCol);
                }

                DestroyRange(currentRow, colCount);
                UninitializedMoveOrCopy(scratch, colCount, currentRow);
                DestroyRange(scratch, colCount);
            }

            res.deallocate(scratch, internal::CheckedElementBytes<T>(colCount), internal::Alignment<T>());
            dataView = MDSpan{data, rowCount, colCount};
        }

        // ======= MEMORY ==========================================================================================================================================================

        /**
         * @brief Reallocates backing storage and moves/copies current active elements.
         *
         * @details
         * Existing active elements are moved/copied into the newly allocated buffer at
         * an offset derived from `adjCountByBeg` (used for begin-side row insertions),
         * then old active elements are destroyed, and old storage is deallocated.
         *
         * Difference to `ReallocAndShift(...)`:
         *   - this function always allocates a new buffer,
         *   - no in-place shift path is attempted.
         *
         * @param[in,out] data          Pointer to backing storage.
         * @param[in,out] res           Memory resource used for allocation/deallocation.
         * @param[in,out] capacity      Number of allocated elements in `data`.
         * @param[in]     colCount      Current column count.
         * @param[in]     rowCount      Current row count.
         * @param[in]     newRowCount   Target row count.
         * @param[in]     adjCountByBeg Signed begin-side row adjustment.
         */
        static void Realloc(T*& data, Memory& res, size_t& capacity, size_t const colCount, size_t const rowCount,
                            size_t const newRowCount, ssize_t const adjCountByBeg) noexcept
        {
            // Step 1: compute old/new active sizes and target capacity
            auto const oldActiveCount = ActiveCount(rowCount, colCount);
            auto const required       = ActiveCount(newRowCount, colCount);
            auto const newCapacity    = NextCapacity(capacity, required);
            if(newCapacity == 0) return;

            // Step 2: allocate destination buffer
            auto* const newData = static_cast<T*>(res.allocate(internal::CheckedElementBytes<T>(newCapacity), internal::Alignment<T>()));

            LUGIZMO_ASSERT_TRACE(newData != nullptr, "Memory allocation returned nullptr in Realloc.");

            // Step 3: move/copy old active range into destination at begin-offset
            if(oldActiveCount > 0)
            {
                auto const destOffset = static_cast<size_t>(std::max<ssize_t>(0, adjCountByBeg));
                auto* const destBeg   = newData + destOffset * colCount;

                LUGIZMO_ASSERT_TRACE((destBeg + oldActiveCount) <= (newData + newCapacity),
                                "Realloc destination range is out of bounds.");
                UninitializedMoveOrCopy(data, oldActiveCount, destBeg);
            }

            // Step 4: tear down old storage and publish new pointer/capacity
            DestroyAndDeallocate(data, oldActiveCount, capacity, res);

            data     = newData;
            capacity = newCapacity;
        }

        /**
         * @brief Ensures capacity and applies begin-side row shifts.
         *
         * @details
         * Behavior by case:
         *   - capacity insufficient: delegates to `Realloc` (which also applies offset),
         *   - begin expansion in-place: creates space by backward shift,
         *   - begin to shrink in-place: compacts remaining rows forward.
         *
         * Only begin-side shifts are handled here; end-side adjustments are handled by
         * higher-level resize construction paths.
         *
         * Difference to `Realloc(...)`:
         *   - can avoid allocation and mutate in-place when capacity is enough,
         *   - acts as a strategy entry point (choose reallocating vs. in-place shift).
         *
         * @param[in,out] data          Pointer to backing storage.
         * @param[in,out] res           Memory resource used for allocation/deallocation.
         * @param[in,out] capacity      Number of allocated elements in `data`.
         * @param[in]     colCount      Current column count.
         * @param[in]     rowCount      Current row count.
         * @param[in]     newRowCount   Target row count.
         * @param[in]     adjCountByBeg Signed begin-side row adjustment.
         */
        static void ReallocAndShift(T*& data, Memory& res, size_t& capacity,
                                    size_t const colCount, size_t const rowCount, size_t const newRowCount,
                                    ssize_t const adjCountByBeg) noexcept
        {
            // 1: allocate-and-move the path when active data no longer fits
            if(newRowCount * colCount > capacity || capacity == 0)
            {
                Realloc(data, res, capacity, colCount, rowCount, newRowCount, adjCountByBeg);
            }
            // 2: in-place expansion at the beginning (create a gap before row 0)
            else if(adjCountByBeg > 0 && rowCount > 0 && colCount > 0)
            {
                auto const shiftElements = static_cast<size_t>(adjCountByBeg) * colCount;
                auto const totalElements = rowCount * colCount;

                // the entire active range is shifted beyond the old end
                if(shiftElements >= totalElements)
                {
                    UninitializedMoveOrCopy(data, totalElements, data + shiftElements);
                    DestroyRange(data, totalElements);
                    return;
                }

                // move trailing overlap to uninitialized tail, then backward-assign the rest
                UninitializedMoveOrCopy(data + (totalElements - shiftElements), shiftElements, data + totalElements);
                AssignBackward(data + shiftElements, data, totalElements - shiftElements);
                DestroyRange(data, shiftElements);
            }
            // 3: in-place shrink at the beginning (drop leading rows and compact)
            else if(adjCountByBeg < 0 && static_cast<ssize_t>(rowCount) + adjCountByBeg > 0)
            {
                auto const shrinkRows    = static_cast<size_t>(-adjCountByBeg);
                auto const remainRows    = rowCount - shrinkRows;
                auto const moveElemCount = remainRows * colCount;
                auto const dropElemCount = shrinkRows * colCount;

                AssignForward(data, data + dropElemCount, moveElemCount);
                DestroyRange(data + moveElemCount, dropElemCount);
            }
        }

        /**
         * @brief Destroys active elements, deallocates storage, and resets shape/capacity.
         *
         * @param[in,out] data      Pointer to backing storage.
         * @param[in,out] capacity  Number of allocated elements in `data`.
         * @param[in,out] dataView  Current matrix view; reset to an empty view.
         * @param[in,out] res       Memory resource used for deallocation.
         */
        static void Free(T*& data, size_t& capacity, MDSpan& dataView, Memory& res)
        {
            DestroyAndDeallocate(data, ActiveCount(dataView), capacity, res);

            data     = nullptr;
            capacity = 0;
            dataView = MDSpan{nullptr, 0, 0};
        }

        /**
         * @brief Constructs `numRows` rows with one default value in uninitialized storage.
         *
         * Difference to `FillRows(..., begin, end)`:
         *   - this overload repeats one scalar value for every element.
         *
         * @param[in,out] data         Destination pointer to uninitialized row-major storage.
         * @param[in]     startRow     First destination row index.
         * @param[in]     numRows      Number of rows to construct.
         * @param[in]     colCount     Columns per row.
         * @param[in]     defaultValue Value to copy into each created element.
         */
        static void FillRows(T* data, size_t const startRow, size_t const numRows, size_t const colCount, T const& defaultValue)
        {
            ConstructRowsDefault(data, startRow, numRows, colCount, defaultValue);
        }

        /**
         * @brief Constructs `numRows` rows from one source row iterator range.
         *
         * Difference to `FillRows(..., defaultValue)`:
         *   - this overload copies one provided row pattern into each created row.
         *
         * Difference to `FillRowByRow(...)`:
         *   - one row pattern is reused for all destination rows instead of consuming
         *     a different input row per destination row.
         *
         * @param[in,out] data     Destination pointer to uninitialized row-major storage.
         * @param[in]     startRow First destination row index.
         * @param[in]     numRows  Number of rows to construct.
         * @param[in]     colCount Columns per row.
         * @param[in]     begin    Iterator to first source value.
         * @param[in]     end      Iterator one-past-last source value.
         */
        template <typename InputIterator>
        static void FillRows(T* data, size_t const startRow, size_t const numRows, size_t const colCount, InputIterator const begin, InputIterator const end)
        {
            ConstructRowsFromValues(data, startRow, numRows, colCount, begin, end);
        }

        /**
         * @brief Constructs rows from an iterator-of-rows source.
         *
         * @details
         * Consumes at most `numRows` row entries from `[rowBegin, rowEnd)`.
         * Stops early if input rows are exhausted.
         *
         * Difference to `FillRows(..., begin, end)`:
         *   - consumes potentially different row content for each destination row
         *     instead of reusing one row pattern.
         *
         * @tparam RowsIterator iterator over row containers.
         *
         * @param[in,out] data     Destination pointer to uninitialized row-major storage.
         * @param[in]     startRow First destination row index.
         * @param[in]     numRows  Maximum number of rows to construct.
         * @param[in]     colCount Columns per row.
         * @param[in,out] rowBegin Current input row iterator; advanced by consumed rows.
         * @param[in]     rowEnd   Input row end iterator.
         */
        template <typename RowsIterator>
        static void FillRowByRow(T *const data, size_t const startRow, size_t const numRows, size_t const colCount,
                                 RowsIterator& rowBegin, RowsIterator const rowEnd)
        {
            (void)ConstructRowsByRow(data, startRow, numRows, colCount, rowBegin, rowEnd);
        }
    };

} // namespace lugizmo

#endif // LUGIZMO_DF_LAYOUT_ROW_MAJOR_H
