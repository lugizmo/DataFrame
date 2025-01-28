// Filename: Layout.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_LAYOUT_H
#define LUGIZMO_DF_LAYOUT_H

#include <memory_resource>
#include <algorithm>
#include <cassert>
#include <span>
#include <mdspan>

namespace lugizmo {

    // TODO doc
    // data is stored row by row value
    template<typename T>
    struct DFRowMajor
    {
        using Memory = std::pmr::memory_resource;
        using Layout = std::layout_right;
        using MDSpan = std::mdspan<T, std::dextents<size_t, 2>, Layout>;

        static constexpr bool IsRowMajor = true;
        static constexpr bool IsColMajor = false;

        /**
         * @brief Expands the storage in T* by taking the current capacity and
         *        increasing that value. New memory region gets populated with
         *        old values and old region gets deallocated.
         *
         * @attention TODO check/add shrinking?
         *
         * @param data
         * @param capacity
         * @param res
         * @param colCount
         * @param rowCount
         * @return
         */
        static auto ExpandStorage(T*& data, size_t& capacity, Memory& res,
                                  size_t const colCount, size_t const rowCount) noexcept
        {
            // create new memory region
            auto const newCapacity = std::max(2 * capacity, colCount * (rowCount + 1));           // TODO better strategy
            auto*const newData     = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

            // move data to new memory region
            std::uninitialized_copy_n(data, rowCount * colCount, newData);
            res.deallocate(data, capacity * sizeof(T));

            // update data pointer variables
            data     = newData;
            capacity = newCapacity;
        }

        /**
         * @brief Expands the storage in T* by taking the current capacity and
         *        increasing that value. New memory region gets populated with
         *        old values and old region gets deallocated.
         *        Additional compared to ExpandStorage this functions expands
         *        in-place the row size. So copy data is slower because it cannot
         *        operate on a whole.
         *
         * @attention TODO check/add shrinking?
         *
         * @param data
         * @param capacity
         * @param res
         * @param colCount
         * @param rowCount
         * @param newColCount
         * @param defaultValue
         * @return
         */
        static auto ExpandStorageWithRowExpansion(T*& data, size_t& capacity, Memory& res,
                                                  size_t const colCount, size_t const rowCount,
                                                  size_t const newColCount, T const& defaultValue) noexcept
        {
            //  create new memory region
            auto const newCapacity = std::max(2 * capacity, std::max(newColCount * 10, newColCount * (rowCount + 1)));      // TODO better strategy
            auto*const newData     = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

            for (size_t row = 0; row < rowCount; ++row)
            {
                // copy existing columns in a row
                std::uninitialized_copy_n(data + row * colCount, colCount, newData + row * newColCount);

                // fill new columns with default value
                std::uninitialized_fill_n(newData + row * newColCount + colCount, newColCount - colCount, defaultValue);
            }

            // deallocate old memory region
            res.deallocate(data, capacity * sizeof(T));

            // update data pointer variables
            data     = newData;
            capacity = newCapacity;
        }

        /**
         * @brief Adds single row at the end of the current rows.
         * @param data          pointer to the data storing all data. (in/out)
         * @param capacity      the current capacity of the data memory array. (in/out)
         * @param res           the memory resources used to create/manage the data. (in/out)
         * @param dataView      the view into the data specifying the current extends. (in/out)
         * @param addRowCount   number of rows to add to the data.
         * @param defaultValue  the default value to set in the new row.
         */
        static void AddRow(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const addRowCount, T const& defaultValue) noexcept
        {
            auto const& extents = dataView.extents();
            auto const colCount = extents.extent(1);
            auto const rowCount = extents.extent(0);

            auto const newRowCount = rowCount + addRowCount;

            // expand storage if needed
            if(newRowCount * colCount > capacity /* || capacity == 0 should not be possible because that means no columns */)
            {
                ExpandStorage(data, capacity, res, colCount, rowCount);
            }

            // add new values
            auto const startIndex = rowCount * colCount;
            std::fill(data + startIndex, data + startIndex + colCount, defaultValue);

            // update dataView to reflect the expanded row count
            // -> data and capacity are updated in ExpandStorageWithRowExpansion
            dataView = MDSpan(data, newRowCount, colCount);
        }

        /**
         * @brief Adds a single new row and populates it with the given values.
         * @param data      pointer to the data storing all data. (in/out)
         * @param capacity  the current capacity of the data memory array. (in/out)
         * @param res       the memory resources used to create/manage the data. (in/out)
         * @param dataView  the view into the data specifying the current extends. (in/out)
         * @param rowValues data to add to the new row.
         */
        static void AddRowWithValues(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, std::span<T const> rowValues) noexcept
        {
            if(rowValues.empty()) return;

            auto const& extents = dataView.extents();
            auto const colCount = extents.extent(1);
            auto const rowCount = extents.extent(0);
            assert(rowValues.size() == colCount && "Adding values in row major requires that values is the size of columns");

            auto const newRowCount = rowCount + 1;

            // expand storage if needed
            if(newRowCount * colCount > capacity /* || capacity == 0 should not be possible because that means no columns */)
            {
                ExpandStorage(data, capacity, res, colCount, rowCount);
            }

            // add new values
            auto const startIndex = rowCount * colCount;
            std::copy(rowValues.begin(), rowValues.end(), data + startIndex);

            // update dataView to reflect the expanded row count
            // -> data and capacity are updated in ExpandStorageWithRowExpansion
            dataView = MDSpan(data, newRowCount, colCount);
        }

        /**
         * @brief Adds column at the end of the current columns.
         * @param data              pointer to the data storing all data. (in/out)
         * @param capacity          the current capacity of the data memory array. (in/out)
         * @param res               the memory resources used to create/manage the data. (in/out)
         * @param dataView          the view into the data specifying the current extends. (in/out)
         * @param addColumnCount    the count of columns to add at the end.
         * @param defaultValue      the default value to set in the new columns.
         */
        static void AddColumn(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const addColumnCount, T const& defaultValue) noexcept
        {
            if(addColumnCount == 0) return;

            auto const colCount = dataView.extent(1);
            auto const rowCount = dataView.extent(0);

            auto const newColCount = colCount + addColumnCount;

            // expand storage if needed
            if(rowCount * newColCount > capacity || capacity == 0)
            {
                ExpandStorageWithRowExpansion(data, capacity, res, colCount, rowCount, newColCount, defaultValue);
            }
            else
            {
                // initialize new columns with defaultValue if no expansion is needed
                for(auto row = rowCount; row > 0; --row)
                {
                    T* oldRowStart = data + (row - 1) * colCount;          // Start of the current row in the old layout
                    T* newRowStart = data + (row - 1) * newColCount;       // Start of the row in the new layout

                    // move existing data to the new row position
                    std::move_backward(oldRowStart, oldRowStart + colCount, newRowStart + colCount);

                    // fill the new columns
                    std::fill(newRowStart + colCount, newRowStart + newColCount, defaultValue);
                }
            }

            // update dataView to reflect the expanded column count
            // -> data and capacity are updated in ExpandStorageWithRowExpansion
            dataView = MDSpan(data, rowCount, newColCount);
        }

        /**
         * @brief Adds column at the beginning and end of the current columns.
         * @param data          pointer to the data storing all data. (in/out)
         * @param capacity      the current capacity of data. (in/out)
         * @param res           the memory resources used to create/manage the data. (in/out)
         * @param dataView      the view into the data specifying the current extends. (in/out)
         * @param adjCountByBeg the count of columns to add/remove from the beginning.
         * @param adjCountByEnd the count of columns to add/remove from the end.
         * @param defaultValue  the default value to set in the new columns.
         */
        static void AdjustColumnCount(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const adjCountByBeg, size_t const adjCountByEnd, T const& defaultValue) noexcept
        {
            if(adjCountByBeg == 0 && adjCountByEnd == 0) return;

            auto const colCount = dataView.extent(1);
            auto const rowCount = dataView.extent(0);

            auto const newColCount = colCount + adjCountByBeg + adjCountByEnd;

            // This is considered an error.
            if(newColCount < 0) return;

            // clear data if zero count
            if(newColCount == 0)
            {
                // TODO check if this is like expected
                res.deallocate(data, capacity * sizeof(T));

                data     = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, 0, 0};

                return;
            }

            if(rowCount * newColCount > capacity || capacity == 0)
            {
                // Expand storage and shift data
                auto const newCapacity = std::max<size_t>(2 * capacity, newColCount * (rowCount + 1));  // TODO better strategy
                auto* const newData    = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

                for(size_t row = 0; row < rowCount; ++row)
                {
                    // New row starting position
                    T* newRowStart = newData + row * newColCount;

                    // Fill new columns at the beginning
                    std::uninitialized_fill_n(newRowStart, adjCountByBeg, defaultValue);

                    // Copy existing columns
                    std::uninitialized_copy_n(data + row * colCount, colCount, newRowStart + adjCountByBeg);

                    // Fill new columns at the end
                    std::uninitialized_fill_n(newRowStart + adjCountByBeg + colCount, adjCountByEnd, defaultValue);
                }

                // Deallocate old memory
                res.deallocate(data, capacity * sizeof(T));

                // Update pointer and capacity
                data = newData;
                capacity = newCapacity;
            }
            else
            {
                // If no expansion is needed
                for (size_t row = rowCount; row > 0; --row)
                {
                    // Old row start and new row start
                    T* oldRowStart = data + (row - 1) * colCount;
                    T* newRowStart = data + (row - 1) * newColCount;

                    // Move existing data to the new position
                    std::move_backward(oldRowStart, oldRowStart + colCount, newRowStart + adjCountByBeg + colCount);

                    // Fill new columns at the beginning
                    std::fill(newRowStart, newRowStart + adjCountByBeg, defaultValue);

                    // Fill new columns at the end
                    std::fill(newRowStart + adjCountByBeg + colCount, newRowStart + newColCount, defaultValue);
                }
            }

            // Update the view to reflect the new column count
            dataView = std::mdspan<T, std::dextents<size_t, 2>, Layout>(data, rowCount, newColCount);
        }

        static void AdjustRecordCount(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const adjCountByBeg, size_t const adjCountByEnd, T const& defaultValue) noexcept
        {
            if (adjCountByBeg == 0 && adjCountByEnd == 0) return;

            auto const colCount = dataView.extent(1);
            auto const rowCount = dataView.extent(0);

            auto const newRowCount = rowCount + adjCountByBeg + adjCountByEnd;

            // This is considered an error.
            if (newRowCount < 0) return;

            // Clear data if zero count
            if (newRowCount == 0)
            {
                res.deallocate(data, capacity * sizeof(T));

                data     = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, 0, 0};

                return;
            }

            if (newRowCount * colCount > capacity || capacity == 0)
            {
                // Expand storage
                auto const newCapacity = std::max<size_t>(2 * capacity, colCount * (newRowCount + 1));  // TODO better strategy
                auto* const newData    = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

                // Fill new rows at the beginning
                for(size_t row = 0; row < adjCountByBeg; ++row)
                {
                    #ifndef NDEBUG
                    auto* const start = newData + row * colCount;
                    auto* const end   = newData + (row + 1) * colCount;
                    assert(start < newData + (row + 1) * colCount && end <= newData + newCapacity && "Invalid memory range for std::uninitialized_fill_n");
                    #endif // NDEBUG

                    std::uninitialized_fill_n(newData + row * colCount, colCount, defaultValue);
                }

                // Copy existing rows
                if(rowCount > 0)
                {
                    #ifndef NDEBUG
                    auto* const destBeg = newData + adjCountByBeg * colCount;
                    auto* const destEnd = destBeg + rowCount * colCount;
                    assert(destEnd <= newData + newCapacity && "Destination out of bounds for std::uninitialized_copy_n");
                    #endif // NDEBUG

                    std::uninitialized_copy_n(data, rowCount * colCount, newData + adjCountByBeg * colCount);
                }

                // Fill new rows at the end
                for(size_t row = 0; row < adjCountByEnd; ++row)
                {
                    #ifndef NDEBUG
                    auto* const start = newData + (adjCountByBeg + rowCount + row) * colCount;
                    auto* const end   = newData + (adjCountByBeg + rowCount + row + 1) * colCount;
                    assert(start < end && end <= newData + newCapacity && "Invalid memory range for std::uninitialized_fill_n");
                    #endif // NDEBUG

                    std::uninitialized_fill_n(newData + (adjCountByBeg + rowCount + row) * colCount, colCount, defaultValue);
                }

                // Deallocate old memory
                res.deallocate(data, capacity * sizeof(T));

                // Update pointer and capacity
                data = newData;
                capacity = newCapacity;
            }
            else
            {
                // If no expansion is needed, shift rows in place
                if(adjCountByBeg > 0)
                {
                    // Move rows up to make room at the beginning
                    auto const begin  = data;
                    auto const end    = data + rowCount * colCount;
                    auto const newEnd = data + (rowCount + adjCountByBeg) * colCount;

                    assert(newEnd > end && begin <= end && "Move backward memory in undefined range");
                    if(rowCount != 0) std::move_backward(begin, end, newEnd);

                    // Initialize newly exposed rows at the beginning
                    for(size_t row = 0; row < adjCountByBeg; ++row)
                    {
                        auto* const fillStart = data + row * colCount;
                        auto* const fillEnd   = data + (row + 1) * colCount;
                        assert(fillStart < fillEnd && fillEnd <= data + capacity * colCount && "Invalid memory range for std::fill");

                        std::fill(fillStart, fillEnd, defaultValue);
                    }
                }

                // Initialize newly exposed rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    auto* const start = data + (adjCountByBeg + rowCount + row) * colCount;
                    auto* const end   = data + (adjCountByBeg + rowCount + row + 1) * colCount;
                    assert(start < end && end <= data + capacity * colCount && "Invalid memory range for std::fill");

                    std::fill(start, end, defaultValue);
                }
            }

            // Update the view to reflect the new row count
            dataView = std::mdspan<T, std::dextents<size_t, 2>, Layout>(data, newRowCount, colCount);
        }

        static void AdjustRecordCount(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const adjCountByBeg, size_t const adjCountByEnd, std::span<T const> const values) noexcept
        {
            if (adjCountByBeg == 0 && adjCountByEnd == 0) return;

            auto const colCount = dataView.extent(1);
            auto const rowCount = dataView.extent(0);

            // Ensure the size of the span matches the column count
            if (values.size() != colCount) return;

            auto const newRowCount = rowCount + adjCountByBeg + adjCountByEnd;

            // This is considered an error.
            if (newRowCount < 0) return;

            // Clear data if zero count
            if (newRowCount == 0)
            {
                res.deallocate(data, capacity * sizeof(T));

                data = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, 0, 0};

                return;
            }

            if (newRowCount * colCount > capacity || capacity == 0)
            {
                // Expand storage
                auto const newCapacity = std::max<size_t>(2 * capacity, colCount * (newRowCount + 1));
                auto* const newData = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

                // Fill new rows at the beginning
                for (size_t row = 0; row < adjCountByBeg; ++row)
                {
                    std::uninitialized_copy(values.begin(), values.end(), newData + row * colCount);
                }

                // Copy existing rows
                std::uninitialized_copy_n(data, rowCount * colCount, newData + adjCountByBeg * colCount);

                // Fill new rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    std::uninitialized_copy(values.begin(), values.end(), newData + (adjCountByBeg + rowCount + row) * colCount);
                }

                // Deallocate old memory
                res.deallocate(data, capacity * sizeof(T));

                // Update pointer and capacity
                data = newData;
                capacity = newCapacity;
            }
            else
            {
                // If no expansion is needed, shift rows in place
                std::move_backward(data, data + rowCount * colCount, data + (rowCount + adjCountByEnd) * colCount);

                // Fill new rows at the beginning
                for (size_t row = 0; row < adjCountByBeg; ++row)
                {
                    std::copy(values.begin(), values.end(), data + row * colCount);
                }

                // Fill new rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    std::copy(values.begin(), values.end(), data + (adjCountByBeg + rowCount + row) * colCount);
                }
            }

            // Update the view to reflect the new row count
            dataView = std::mdspan<T, std::dextents<size_t, 2>, Layout>(data, newRowCount, colCount);
        }

        static void AdjustRecordCount(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const adjCountByBeg, size_t const adjCountByEnd, IterableOfIterable auto const& values) noexcept
        {
            if (adjCountByBeg == 0 && adjCountByEnd == 0) return;

            auto const colCount = dataView.extent(1);
            auto const rowCount = dataView.extent(0);

            // Check if the input matches the required dimensions
            auto const inputRowCount = std::ranges::distance(values);
            if (inputRowCount == 0) return;

            auto const inputColCount = std::ranges::distance(*std::begin(values));
            if (inputColCount != colCount) return;

            auto const newRowCount = rowCount + adjCountByBeg + adjCountByEnd;

            // This is considered an error
            if (newRowCount < 0) return;

            // Clear data if zero count
            if (newRowCount == 0)
            {
                res.deallocate(data, capacity * sizeof(T));

                data = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, 0, 0};

                return;
            }

            if (newRowCount * colCount > capacity || capacity == 0)
            {
                // Expand storage
                auto const newCapacity = std::max<size_t>(2 * capacity, colCount * (newRowCount + 1));
                auto* const newData = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

                // Fill new rows at the beginning
                auto inputRowIt = std::begin(values);
                for (size_t row = 0; row < adjCountByBeg; ++row)
                {
                    if (inputRowIt != std::end(values)) {
                        std::ranges::copy(*inputRowIt, newData + row * colCount);
                        ++inputRowIt;
                    }
                }

                // Copy existing rows
                std::uninitialized_copy_n(data, rowCount * colCount, newData + adjCountByBeg * colCount);

                // Fill new rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    if (inputRowIt != std::end(values)) {
                        std::ranges::copy(*inputRowIt, newData + (adjCountByBeg + rowCount + row) * colCount);
                        ++inputRowIt;
                    }
                }

                // Deallocate old memory
                res.deallocate(data, capacity * sizeof(T));

                // Update pointer and capacity
                data = newData;
                capacity = newCapacity;
            }
            else
            {
                // If no expansion is needed, shift rows in place
                std::move_backward(data, data + rowCount * colCount, data + (rowCount + adjCountByEnd) * colCount);

                // Fill new rows at the beginning
                auto inputRowIt = std::begin(values);
                for (size_t row = 0; row < adjCountByBeg; ++row)
                {
                    if (inputRowIt != std::end(values)) {
                        std::copy(std::begin(*inputRowIt), std::end(*inputRowIt), data + row * colCount);
                        ++inputRowIt;
                    }
                }

                // Fill new rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    if (inputRowIt != std::end(values)) {
                        std::copy(std::begin(*inputRowIt), std::end(*inputRowIt),
                                  data + (adjCountByBeg + rowCount + row) * colCount);
                        ++inputRowIt;
                    }
                }
            }

            // Update the view to reflect the new row count
            dataView = std::mdspan<T, std::dextents<size_t, 2>, Layout>(data, newRowCount, colCount);
        }

        static void AdjustRecordCount(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const adjCountByBeg, size_t const adjCountByEnd,
                                      std::initializer_list<std::initializer_list<T>> values) noexcept
        {
            if (adjCountByBeg == 0 && adjCountByEnd == 0) return;

            auto const colCount = dataView.extent(1);
            auto const rowCount = dataView.extent(0);

            // Validate dimensions of the input
            auto const inputRowCount = values.size();
            if (inputRowCount == 0) return;

            auto const inputColCount = values.begin()->size();
            for (auto const& row : values) {
                if (row.size() != inputColCount) return; // Ensure uniform column size in input
            }
            if (inputColCount != colCount) return;

            auto const newRowCount = rowCount + adjCountByBeg + adjCountByEnd;

            // This is considered an error
            if (newRowCount < 0) return;

            // Clear data if zero count
            if (newRowCount == 0)
            {
                res.deallocate(data, capacity * sizeof(T));
                data = nullptr;
                capacity = 0;
                dataView = MDSpan{nullptr, 0, 0};
                return;
            }

            if (newRowCount * colCount > capacity || capacity == 0)
            {
                // Expand storage
                auto const newCapacity = std::max<size_t>(2 * capacity, colCount * (newRowCount + 1));
                auto* const newData = static_cast<T*>(res.allocate(newCapacity * sizeof(T)));

                // Fill new rows at the beginning
                auto inputRowIt = values.begin();
                for (size_t row = 0; row < adjCountByBeg; ++row)
                {
                    if (inputRowIt != values.end()) {
                        std::uninitialized_copy(inputRowIt->begin(), inputRowIt->end(), newData + row * colCount);
                        ++inputRowIt;
                    }
                }

                // Copy existing rows
                std::uninitialized_copy_n(data, rowCount * colCount, newData + adjCountByBeg * colCount);

                // Fill new rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    if (inputRowIt != values.end()) {
                        std::uninitialized_copy(inputRowIt->begin(), inputRowIt->end(),
                                                newData + (adjCountByBeg + rowCount + row) * colCount);
                        ++inputRowIt;
                    }
                }

                // Deallocate old memory
                res.deallocate(data, capacity * sizeof(T));

                // Update pointer and capacity
                data = newData;
                capacity = newCapacity;
            }
            else
            {
                // If no expansion is needed, shift rows in place
                std::move_backward(data, data + rowCount * colCount, data + (rowCount + adjCountByEnd) * colCount);

                // Fill new rows at the beginning
                auto inputRowIt = values.begin();
                for (size_t row = 0; row < adjCountByBeg; ++row)
                {
                    if (inputRowIt != values.end()) {
                        std::copy(inputRowIt->begin(), inputRowIt->end(), data + row * colCount);
                        ++inputRowIt;
                    }
                }

                // Fill new rows at the end
                for (size_t row = 0; row < adjCountByEnd; ++row)
                {
                    if (inputRowIt != values.end()) {
                        std::copy(inputRowIt->begin(), inputRowIt->end(),
                                  data + (adjCountByBeg + rowCount + row) * colCount);
                        ++inputRowIt;
                    }
                }
            }

            // Update the view to reflect the new row count
            dataView = std::mdspan<T, std::dextents<size_t, 2>, Layout>(data, newRowCount, colCount);
        }

        static void DropRow(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const rowToRemove) noexcept
        {
            auto const& extents = dataView.extents();
            auto const colCount = extents.extent(1);
            auto const rowCount = extents.extent(0);

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
            auto const colCount = extents.extent(1);
            auto const rowCount = extents.extent(0);

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
    };

    struct DFColMajor
    {
        static constexpr bool IsRowMajor = false;
        static constexpr bool IsColMajor = true;

    };
}

#endif // LUGIZMO_DF_LAYOUT_H
