// Filename: LayoutRowMajor.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_LAYOUT_ROW_MAJOR_H
#define LUGIZMO_DF_LAYOUT_ROW_MAJOR_H

#include <memory>
#include <memory_resource>
#include <algorithm>
#include <cassert>
#include <span>
#include <tuple>
#include <mdspan>
#include <cmath>

#include "lugizmo/memory/Memory.h"
#include "lugizmo/container/Concepts.h"

namespace lugizmo {

    /**
     * @brief   Manages Data stored as T* that consists of fields and records.
     * @details User passes the pointer to the underlying data (T*) and can then:
     *          - ResizeCols
     *          - ResizeRows
     *          TODO add shrink function (maybe as replacement for drop?)
     *
     * @tparam T
     */
    template <typename T>
    struct DFRowMajor final
    {
        using Memory = std::pmr::memory_resource;
        using Layout = std::layout_right;
        using MDSpan = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

        static constexpr bool IsRowMajor = true;
        static constexpr bool IsColMajor = false;

        // ======= ADJUST COLUMNS/ROWS BY COUNT ============================================================================================

        /**
         * TODO test
         * @brief Adds column at the beginning and end of the current columns.
         *
         * @param data          pointer to the data storing all data. (in/out)
         * @param capacity      the current capacity of data. (in/out)
         * @param res           the memory resources used to create/manage the data. (in/out)
         * @param dataView      the view into the data specifying the current extends. (in/out)
         * @param adjCountByBeg the count of columns to add/remove from the beginning.
         * @param adjCountByEnd the count of columns to add/remove from the end.
         * @param defaultValue  the default value to set in the new columns.
         */ // TODO this function should also take ssize_t to remove columns. It might already work.
        static void ResizeCols(T*& data, size_t& capacity, Memory& res, MDSpan& dataView,
                               ssize_t const adjCountByBeg, ssize_t const adjCountByEnd,
                               T const& defaultValue) noexcept
        {
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            auto const colCount = dataView.extent(1) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(1));
            auto const rowCount = dataView.extent(0) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(0));

            auto const newColCount = static_cast<ssize_t>(colCount) + adjCountByBeg + adjCountByEnd;

            // This is considered an error.
            if(newColCount < 0) return;

            // clear data if zero count
            if(newColCount == 0)
            {
                // TODO check if this is like expected
                if(data != nullptr) res.deallocate(data, capacity * sizeof(T), internal::Alignment<T>());

                data     = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, 0, 0};

                return;
            }

            auto uNewColCount = static_cast<size_t>(newColCount);
            if(rowCount * uNewColCount> capacity || capacity == 0)
            {
                // Expand storage and shift data
                auto const newCapacity = std::max<size_t>(2 * capacity, uNewColCount * (rowCount + 1));  // TODO better strategy
                auto* const newData    = static_cast<T*>(res.allocate(newCapacity * sizeof(T), internal::Alignment<T>()));

                for(size_t row = 0; row < rowCount; ++row)
                {
                    // New row starting position
                    T* newRowStart = newData + row * uNewColCount;

                    // Fill new columns at the beginning
                    std::uninitialized_fill_n(newRowStart, adjCountByBeg, defaultValue);

                    // Copy existing columns
                    std::uninitialized_copy_n(data + row * colCount, colCount, newRowStart + adjCountByBeg);

                    // Fill new columns at the end
                    std::uninitialized_fill_n(newRowStart + adjCountByBeg + colCount, adjCountByEnd, defaultValue);
                }

                // Deallocate old memory
                if(data != nullptr) res.deallocate(data, capacity * sizeof(T), internal::Alignment<T>());

                // Update pointer and capacity
                data = newData;
                capacity = newCapacity;
            }
            else
            {
                // If no expansion is needed
                for(size_t row = rowCount; row > 0; --row)
                {
                    // Old row start and new row start
                    T* oldRowStart = data + (row - 1) * colCount;
                    T* newRowStart = data + (row - 1) * uNewColCount;

                    // Move existing data to the new position
                    std::move_backward(oldRowStart, oldRowStart + colCount, newRowStart + adjCountByBeg + colCount);

                    // Fill new columns at the beginning
                    std::fill(newRowStart, newRowStart + adjCountByBeg, defaultValue);

                    // Fill new columns at the end
                    std::fill(newRowStart + adjCountByBeg + colCount, newRowStart + uNewColCount, defaultValue);
                }
            }

            // Update the view to reflect the new column count
            dataView = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>(data, rowCount, uNewColCount);
        }


        /**
         * @brief Adjusts the number of rows in the dataset, filling new rows with a default value.
         *
         * @details This function resizes the row count by adding or removing rows from the beginning
         *          and/or end. If rows are added, they are initialized with the provided 'defaultValue'.
         *          If the new row count exceeds the allocated capacity, memory is reallocated.
         *          If rows are removed, no additional cleanup is performed.
         *
         * @param[in,out] data           Pointer to the allocated memory, updated if reallocation occurs.
         * @param[in,out] capacity       The total memory capacity, updated if reallocation occurs.
         * @param[in]     res            The memory resource used for allocation and de-allocation.
         * @param[in,out] dataView       The mdspan view representing the data, updated to reflect changes.
         * @param[in]     adjCountByBeg  Number of rows to add/remove at the beginning.
         * @param[in]     adjCountByEnd  Number of rows to add/remove at the end.
         * @param[in]     defaultValue   The default value used to initialize new rows.
         *
         * @note
         * - If 'adjCountByBeg' or 'adjCountByEnd' is negative, rows are removed instead of added.
         * - If 'newRowCount == 0', the function deallocates all memory.
         * - If expansion is required, the function reallocates memory with a growth strategy.
         */
        static void ResizeRows(T*& data, size_t& capacity, Memory& res, MDSpan& dataView,
                               ssize_t const adjCountByBeg, ssize_t const adjCountByEnd, T const& defaultValue) noexcept
        {
            // nothing to change
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            // store current and compute change size
            auto const colCount    = dataView.extent(1) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(1));
            auto const rowCount    = dataView.extent(0) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(0));
            auto const newRowCount = static_cast<ssize_t>(rowCount) + adjCountByBeg + adjCountByEnd;

            // free memory when empty data
            if(newRowCount == 0)
            {
                Free(data, capacity, dataView, res);
                return;
            }

            // non-empty size
            // check inputs first
            if(newRowCount < 0)
            {
                assert(newRowCount >= 0 && "Record count is negative!");
                return;
            }

            // row count cannot be negative here anymore
            auto const newRowCountPos = static_cast<size_t>(newRowCount);

            // 1. allocate & expand memory if needed
            // 2. shift data as needed to accommodate new data
            ReallocAndShift(data, res, capacity, colCount, rowCount, newRowCountPos, adjCountByBeg);

            // fill data at beginning and/or end
            if(rowCount == 0)
            {
                // fill all with default value as nothing was in previously
                FillRows(data, 0, newRowCountPos, colCount, defaultValue);
            }
            else
            {
                if(adjCountByBeg > 0) FillRows(data, 0, static_cast<size_t>(adjCountByBeg), colCount, defaultValue);
                if(adjCountByEnd > 0) FillRows(data, newRowCountPos - static_cast<size_t>(adjCountByEnd), static_cast<size_t>(adjCountByEnd), colCount, defaultValue);
            }

            // update the view
            dataView = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>(data, newRowCountPos, colCount);
        }

        /**
         * @brief Adjusts the number of rows in the dataset using provided row values.
         *
         * @details This function resizes the row count by adding or removing rows at the beginning
         *          and/or end. If rows are added, they are initialized using values from `values`.
         *          If the new row count exceeds the allocated capacity, memory is reallocated.
         *          If rows are removed, no additional cleanup is performed.
         *
         * @tparam Values                A container type representing one or more rows of data.
         *
         * @param[in,out] data           Pointer to the allocated memory, updated if reallocation occurs.
         * @param[in,out] capacity       The total memory capacity, updated if reallocation occurs.
         * @param[in]     res            The memory resource used for allocation and de-allocation.
         * @param[in,out] dataView       The mdspan view representing the data, updated to reflect changes.
         * @param[in]     adjCountByBeg  Number of rows to add/remove at the beginning.
         * @param[in]     adjCountByEnd  Number of rows to add/remove at the end.
         * @param[in]     values         Iterable containing row values (either a single row repeated or multiple rows for direct init.).
         *
         * @note
         * - If `adjCountByBeg` or `adjCountByEnd` is negative, rows are removed instead of added.
         * - If `values` is a single iterable, all added rows are initialized using it.
         * - If `values` is an iterable of iterables, each new row is initialized with corresponding values.
         * - If `newRowCount == 0`, the function deallocates all memory.
         * - If expansion is required, the function reallocates memory with a growth strategy.
         */
        template <typename Values> requires MinimalIterable<Values>
        static void ResizeRows(T*& data, size_t& capacity, Memory& res, MDSpan& dataView,
                               ssize_t const adjCountByBeg, ssize_t const adjCountByEnd, Values const& values) noexcept
        {
            constexpr bool isItOfIt = MinimalIterableOfIterable<Values>;

            // nothing to change
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            // store current extends
            auto const colCount = dataView.extent(1) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(1));
            auto const rowCount = dataView.extent(0) < 0 ? 0UZ : static_cast<size_t>(dataView.extent(0));

            // row count (signed)
            auto const rowCountS    = static_cast<ssize_t>(rowCount);
            auto const newRowCountS = rowCountS + adjCountByBeg + adjCountByEnd;

            // free memory when empty data
            if(newRowCountS == 0)
            {
                Free(data, capacity, dataView, res);
                return;
            }

            // non-empty size
            // check inputs first
            if(newRowCountS < 0)
            {
                return;
            }

            if constexpr(isItOfIt)
            {
                if(std::ranges::distance(values) == 0) return;
                if(std::ranges::distance(*std::begin(values)) != static_cast<std::ptrdiff_t>(colCount)) return;
                if(!std::ranges::all_of(values, [&](auto const& row) { return row.size() == colCount; })) return;
            }
            else if(values.size() != colCount) return;

            // 1. allocate and expand memory if needed
            // 2. shift data as needed to accommodate new data
            ReallocAndShift(data, res, capacity, colCount, rowCount, static_cast<size_t>(newRowCountS), adjCountByBeg);

            if constexpr(isItOfIt)
            {
                // fill each row individually with a different row of values .
                auto inputRowIt = std::begin(values);
                if(adjCountByBeg > 0) FillRowByRow(data, 0, static_cast<size_t>(adjCountByBeg), colCount, inputRowIt, std::end(values));
                if(adjCountByEnd > 0) FillRowByRow(data, static_cast<size_t>(newRowCountS - adjCountByEnd), static_cast<size_t>(adjCountByEnd), colCount, inputRowIt, std::end(values));
            }
            else
            {
                // fill the entire row with a single set of values
                if(adjCountByBeg > 0) FillRows(data, 0, static_cast<size_t>(adjCountByBeg), colCount, values.begin(), values.end());
                if(adjCountByEnd > 0) FillRows(data, static_cast<size_t>(newRowCountS - adjCountByEnd), static_cast<size_t>(adjCountByEnd), colCount, values.begin(), values.end());
            }

            // update the view
            dataView = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>(data, static_cast<size_t>(newRowCountS), colCount);
        }

        // ======= DROP COLUMNS/ROWS =======================================================================================================

        static void DropRow(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const rowToRemove) noexcept
        {
            auto const& extents = dataView.extents();
            auto const colCount = static_cast<size_t>(extents.extent(1));
            auto const rowCount = static_cast<size_t>(extents.extent(0));

            assert(rowToRemove < rowCount && "Row index out of bounds");

            if(rowToRemove < rowCount - 1)
            {
                // calculate the starting index of the row to remove &
                // shift subsequent rows up by one row to fill the gap
                auto const removeStartIndex = rowToRemove * colCount;
                std::move(data + removeStartIndex + colCount, data + rowCount * colCount, data + removeStartIndex);
            }

            // update dataView to reflect the reduced row count
            dataView = MDSpan(data, rowCount - 1, colCount);
            // TODO think about reducing capacity
        }

        static void DropColumn(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const colToRemove) noexcept
        {
            auto const& extents = dataView.extents();
            auto const colCount = static_cast<size_t>(extents.extent(1));
            auto const rowCount = static_cast<size_t>(extents.extent(0));

            assert(colToRemove < colCount && "Column index out of bounds");

            auto const newColCount = colCount - 1;
            for (size_t row = 0; row < rowCount; ++row)
            {
                // calculate the starting index of the old row
                auto const oldRowStart = row * colCount;

                // move data from columns before colToRemove
                std::move(data + oldRowStart, data + oldRowStart + colToRemove, data + row * newColCount);

                // move data from columns after colToRemove
                std::move(data + oldRowStart + colToRemove + 1, data + oldRowStart + colCount, data + row * newColCount + colToRemove);
            }

            dataView = MDSpan(data, rowCount, newColCount);
            // TODO think about reducing capacity
        }

    public: // TODO make private + test dependency

        // ======= MEMORY ==================================================================================================================

        /**
         *  @brief   Allocates a new buffer with increased capacity and moves existing rows to their new positions.
         *
         *  @details This function reallocates memory when the new row count exceeds the current capacity.
         *           It calculates an optimal growth factor for memory expansion and allocates a new buffer.
         *           If `rowCount > 0`, it moves existing data to the appropriate offset in the new buffer
         *           (taking `adjCountByBeg` into account). The old buffer is then deallocated.
         *
         *  @param[in,out]  data          Reference to the pointer holding the current allocated buffer.
         *                                This will be updated to point to the newly allocated buffer.
         *  @param[in,out]  res           Memory resource used for allocation and de-allocation.
         *  @param[in,out]  capacity      Reference to the current capacity of the buffer in elements.
         *                                This will be updated to the new capacity after reallocation.
         *  @param[in]      colCount      Number of columns in the data structure.
         *  @param[in]      rowCount      Current number of rows in the data structure.
         *  @param[in]      newRowCount   New number of rows after resizing.
         *  @param[in]      adjCountByBeg Number of rows added or removed at the beginning.
         *
         *  @note This function does **not** handle filling newly allocated rows with default values.
         *        It only ensures correct memory layout and preserves existing data.
         *
         *  @warning The caller must ensure that any newly allocated rows are properly initialized.
         */
        static void Realloc(T*& data, Memory& res, size_t& capacity, size_t const colCount, size_t const rowCount,
                            size_t const newRowCount, ssize_t const adjCountByBeg) noexcept
        {
            auto GrowthFactor = [](size_t const current)
            {
                // TODO better strategy?
                if(current < 1 * 1024 * 1024) return current * 2;                                                   // < 1MB
                if(current < 256 * 1024 * 1024) return static_cast<size_t>(static_cast<float>(current) * 1.25f);    // < 256MB
                return current + 32 * 1024 * 1024;
            };

            auto const newCapacity = GrowthFactor(capacity + colCount * (newRowCount + 1));
            if(newCapacity == 0) return;

            auto* const newData = static_cast<T*>(res.allocate(newCapacity * sizeof(T), internal::Alignment<T>()));

            assert(newCapacity > capacity || colCount == 0);
            assert(newData != nullptr && "Memory allocation failed");

            // move existing data to the new buffer at the correct offset
            if(rowCount > 0)
            {
                auto* const destBeg = newData + std::max<ssize_t>(0, adjCountByBeg) * static_cast<ssize_t>(colCount);
                assert((destBeg + rowCount * colCount /* destEnd */) <= newData + newCapacity && "Destination out of bounds for copying existing rows");
                std::uninitialized_copy_n(data, rowCount * colCount, destBeg);
            }

            // deallocate old memory
            if(data != nullptr) res.deallocate(data, capacity * sizeof(T), internal::Alignment<T>());

            // update the pointer and capacity
            data     = newData;
            capacity = newCapacity;
        }

        /**
         *  @brief   Ensures that the data buffer has enough capacity and adjusts the row positions.
         *
         *  @details This function handles memory expansion and row shifting:
         *           - If the new row count exceeds capacity, it reallocates memory and shifts data within `Realloc()`.
         *           - If no reallocation is needed, it shifts rows forward or backward in-place as required.
         *
         *  @param[in,out] data          Pointer to the allocated buffer holding the row-major data.
         *  @param[in,out] res           Memory resource used for allocation and de-allocation.
         *  @param[in,out] capacity      Current capacity of the buffer in elements, updated if reallocated.
         *  @param[in]     colCount      Number of columns in the data structure.
         *  @param[in]     rowCount      Current number of rows in the data structure.
         *  @param[in]     newRowCount   New number of rows after resizing.
         *  @param[in]     adjCountByBeg Number of rows added or removed at the beginning.
         *
         *  @note  If reallocation is performed, `Realloc()` also handles shifting data accordingly.
         *         If no reallocation is required, the function shifts existing rows forward or backward
         *         to accommodate changes at the beginning.
         */
        static void ReallocAndShift(T*& data, Memory& res, size_t& capacity,
                                    size_t const colCount, size_t const rowCount, size_t const newRowCount,
                                    ssize_t const adjCountByBeg) noexcept
        {
            if(newRowCount * colCount > capacity || capacity == 0)
            {
                // if new data size exceeds capacity or there's no allocated memory, perform reallocation
                // this also handles shifting as part of the reallocation process
                Realloc(data, res, capacity, colCount, rowCount, newRowCount, adjCountByBeg);
            }
            else if(adjCountByBeg > 0 && rowCount > 0)
            {
                // expanding at the beginning: Shift existing rows backward to make space for new rows
                auto const begin  = data;
                auto const end    = data + rowCount * colCount;
                auto const newEnd = data + (rowCount + static_cast<size_t>(adjCountByBeg)) * colCount;

                assert(newEnd > end && begin <= end && "Move backward memory in undefined range");
                std::move_backward(begin, end, newEnd);
            }
            else if(adjCountByBeg < 0 && static_cast<ssize_t>(rowCount) + adjCountByBeg > 0)
            {
                // shrinking at the beginning: compute remaining rows in signed domain
                const ssize_t remSigned = static_cast<ssize_t>(rowCount) + adjCountByBeg;
                if (remSigned <= 0) {
                    // nothing to move (all rows removed or invalid); caller handles zeroing if needed
                    return;
                }

                auto const remainRows    = static_cast<size_t>(remSigned);           // rows to keep
                auto const shrinkRows    = static_cast<size_t>(-adjCountByBeg);      // rows removed at begin
                auto const elemsPerRow   = colCount;
                auto const moveElemCount = remainRows * elemsPerRow;

                T* const src = data + shrinkRows * elemsPerRow;
                T* const dst = data;

                assert((dst + moveElemCount /* dstEnd */) >= dst && "Shrinking rows caused invalid range");
                std::move(src, src + moveElemCount, dst);
            }
        }

        /**
         * @brief Frees allocated memory and resets associated data structures.
         *
         * @param[in,out] data       Pointer to the allocated memory, set to 'nullptr' after de-allocation.
         * @param[in,out] capacity   The capacity of the allocated memory, reset to zero.
         * @param[in,out] dataView   The mdspan view representing the data, reset to an empty state.
         * @param[in]     res        The memory resource used for de-allocation.
         */
        static void Free(T*& data, size_t& capacity, MDSpan& dataView, Memory& res)
        {
            if(data != nullptr) res.deallocate(data, capacity * sizeof(T), internal::Alignment<T>());

            data     = nullptr;
            capacity = 0;
            dataView = MDSpan{nullptr, 0, 0};
        }

        // TODO doc
        // Utility function: Copy default values into newly allocated rows
        // TODO make private add compiler flags for non nullable pointers then
        // TODO move to other context this is probably also useful in non count adj.
        // TODO add test
        static void FillRows(T* data, size_t const startRow, size_t const numRows, size_t const colCount, T const& defaultValue)
        {
            if(colCount == 0) return;

            for(size_t row = 0; row < numRows; ++row)
            {
                auto* start = data + (startRow + row) * colCount;
                auto* end   = start + colCount;

                assert(start < end && "Invalid memory range for filling default values");
                std::fill(start, end, defaultValue);
            }
        }

        // TODO doc
        // Utility function: Copy rows from an input iterable
        // TODO make private add compiler flags for non nullable pointers then
        // TODO move to other context this is probably also useful in non count adj.
        // TODO add test
        template <typename InputIterator>
        static void FillRows(T* data, size_t const startRow, size_t const numRows, size_t const colCount, InputIterator const begin, InputIterator const end)
        {
            if(colCount == 0) return;

            for(size_t row = 0; row < numRows; ++row)
            {
                auto* start  = data + (startRow + row) * colCount;

                assert(start < (start + colCount /* finish */) && "Invalid memory range in FillRows");
                assert(std::distance(begin, end) == static_cast<std::ptrdiff_t>(colCount) && "Column count mismatch");
                std::copy(begin, end, start);
            }
        }

        /**
         * TODO doc
         * TODO test
         * @tparam RowsIterator
         * @param data
         * @param startRow
         * @param numRows
         * @param colCount
         * @param rowBegin
         * @param rowEnd
         */
        template <typename RowsIterator>
        static void FillRowByRow(T *const data, size_t const startRow, size_t const numRows, size_t const colCount,
                                 RowsIterator& rowBegin, RowsIterator const rowEnd)
        {
            if(colCount == 0) return;

            for(size_t row = 0; row < numRows && rowBegin != rowEnd; ++row, ++rowBegin)
            {
                FillRows(data, startRow + row, 1, colCount, rowBegin->begin(), rowBegin->end());
            }
        }
    };
}

#endif // LUGIZMO_DF_LAYOUT_ROW_MAJOR_H
