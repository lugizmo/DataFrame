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
                                  size_t const colCount, size_t const rowCount)
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
                                                  size_t const newColCount, T const& defaultValue)
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

//        template<typename... Ts>
//        static void AddRow(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, Ts&&... rows)
//        {
//            static_assert(sizeof...(rows) > 0, "Values pack cannot be empty"); // TODO is this possible and if so why not just do nothing?
//
//            auto const& extents = dataView.extents();
//            auto const colCount = extents.extent(1);
//            auto const rowCount = extents.extent(0);
//            assert(sizeof...(rows) == colCount && "Adding values in row major requires that values is the size of columns");
//
//            auto const newRowCount = rowCount + 1;
//
//            // expand storage if needed
//            if(newRowCount * colCount > capacity /* || capacity == 0 should not be possible because that means no columns */)
//            {
//                ExpandStorage(data, capacity, res, colCount, rowCount);
//            }
//
//            // add new values
//            auto const startIndex = rowCount * colCount;
//            size_t colIndex = 0;
//            ((data[startIndex + colIndex++] = std::forward<T>(rows)), ...);
//
//            // update dataView to reflect the expanded row count
//            // -> data and capacity are updated in ExpandStorageWithRowExpansion
//            dataView = MDSpan(data, newRowCount, colCount);
//        }

        static void AddRowWithValues(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, std::span<T const> rows)
        {
            if(rows.empty()) return;

            auto const& extents = dataView.extents();
            auto const colCount = extents.extent(1);
            auto const rowCount = extents.extent(0);
            assert(rows.size() == colCount && "Adding values in row major requires that values is the size of columns");

            auto const newRowCount = rowCount + 1;

            // expand storage if needed
            if(newRowCount * colCount > capacity /* || capacity == 0 should not be possible because that means no columns */)
            {
                ExpandStorage(data, capacity, res, colCount, rowCount);
            }

            // add new values
            auto const startIndex = rowCount * colCount;
            std::copy(rows.begin(), rows.end(), data + startIndex);

            // update dataView to reflect the expanded row count
            // -> data and capacity are updated in ExpandStorageWithRowExpansion
            dataView = MDSpan(data, newRowCount, colCount);
        }

        static void AddRowWithDefault(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, T const& defaultValue)
        {
            auto const& extents = dataView.extents();
            auto const colCount = extents.extent(1);
            auto const rowCount = extents.extent(0);

            auto const newRowCount = rowCount + 1;

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

        static void AddColumn(T*& data, size_t& capacity, Memory& res, MDSpan& dataView, size_t const addColumnCount, T const& defaultValue)
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

        static void DropRow(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const rowToRemove)
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

        static void DropColumn(T*& data, size_t& /*capacity*/, Memory& /*res*/, MDSpan& dataView, size_t const colToRemove)
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
