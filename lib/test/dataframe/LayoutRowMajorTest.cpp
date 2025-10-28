// Filename: IndexUniqueTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <memory>
#include <memory_resource>
#include <vector>
#include <span>
#include <initializer_list>

#include "lugizmo/dataframe/LayoutRowMajor.h"

// ====== LAYOUT ALLOCATE TEST =============================================================================================================

TEST(lugizmo_dataframe_layout_row_base_test, realloc)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t  colCount      = 3;
    constexpr size_t  rowCount      = 2;
    constexpr size_t  newRowCount   = 4;
    constexpr ssize_t adjCountByBeg = 1;

    size_t capacity = 6;

    // Allocate initial data and initialize
    auto* data = static_cast<T*>(memory->allocate(capacity * sizeof(T)));
    for (size_t i = 0; i < rowCount * colCount; ++i) data[i] = static_cast<T>(i + 1);

    // Call Realloc
    lugizmo::DFRowMajor<T>::Realloc(data, *memory, capacity, colCount, rowCount, newRowCount, adjCountByBeg);

    EXPECT_GE(capacity, newRowCount * colCount) << "Capacity should expand to fit the new rows.";
    EXPECT_NE(data, nullptr) << "Reallocated data should not be null.";

    // Check that original data is copied at the correct offset
    constexpr size_t startIndex = adjCountByBeg * colCount;
    for (size_t i = 0; i < rowCount * colCount; ++i) {
        EXPECT_EQ(data[startIndex + i], i + 1) << "Data mismatch at index " << i;
    }

    memory->deallocate(data, capacity * sizeof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, realloc_zero_capacity)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 0;
    constexpr size_t newRowCount = 4;
    constexpr ssize_t adjCountByBeg = 1;

    size_t capacity = 0;
    T* data = nullptr;

    // Call Realloc with zero capacity
    lugizmo::DFRowMajor<T>::Realloc(data, *memory, capacity, colCount, rowCount, newRowCount, adjCountByBeg);

    EXPECT_NE(data, nullptr) << "Reallocated data should not be null.";
    EXPECT_GE(capacity, newRowCount * colCount) << "Capacity should be allocated for new rows.";

    memory->deallocate(data, capacity * sizeof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, realloc_large_allocation)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 5;
    constexpr size_t rowCount = 200;
    constexpr size_t newRowCount = 1000;
    constexpr ssize_t adjCountByBeg = 10;

    size_t capacity = 1024;
    auto* data = static_cast<T*>(memory->allocate(capacity * sizeof(T)));
    for (size_t i = 0; i < rowCount * colCount; ++i) data[i] = static_cast<T>(i);

    lugizmo::DFRowMajor<T>::Realloc(data, *memory, capacity, colCount, rowCount, newRowCount, adjCountByBeg);

    EXPECT_NE(data, nullptr) << "Reallocated data should not be null.";
    EXPECT_GE(capacity, newRowCount * colCount) << "Capacity should be large enough for new rows.";

    memory->deallocate(data, capacity * sizeof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, realloc_and_shift)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t newRowCount = 4;
    constexpr ssize_t adjCountByBeg = 1;

    size_t capacity = 6;

    // Allocate initial data and initialize
    auto* data = static_cast<T*>(memory->allocate(capacity * sizeof(T)));
    for (size_t i = 0; i < rowCount * colCount; ++i) data[i] = static_cast<T>(i + 1);

    // Call ReallocAndShift
    lugizmo::DFRowMajor<T>::ReallocAndShift(data, *memory, capacity, colCount, rowCount, newRowCount, adjCountByBeg);

    EXPECT_GE(capacity, newRowCount * colCount) << "Capacity should expand if necessary.";
    EXPECT_NE(data, nullptr) << "Reallocated data should not be null.";

    // Check that original data is copied/shuffled correctly
    constexpr size_t startIndex = adjCountByBeg * colCount;
    for (size_t i = 0; i < rowCount * colCount; ++i) {
        EXPECT_EQ(data[startIndex + i], i + 1) << "Data mismatch at index " << i;
    }

    memory->deallocate(data, capacity * sizeof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, free)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;

    size_t capacity = 6;

    // Allocate memory
    auto* data = static_cast<T*>(memory->allocate(capacity * sizeof(T)));
    for (size_t i = 0; i < rowCount * colCount; ++i) data[i] = static_cast<T>(i + 1);

    // Create an mdspan to track the view
    auto dataView = lugizmo::DFRowMajor<T>::MDSpan{data, rowCount, colCount};

    // Call Free
    lugizmo::DFRowMajor<T>::Free(data, capacity, dataView, *memory);

    // Validate
    EXPECT_EQ(data, nullptr) << "Data pointer should be null after freeing.";
    EXPECT_EQ(capacity, 0) << "Capacity should be zero after freeing.";
    EXPECT_EQ(dataView.extent(0), 0) << "Row count should be zero after freeing.";
    EXPECT_EQ(dataView.extent(1), 0) << "Column count should be zero after freeing.";
}

TEST(lugizmo_dataframe_layout_row_base_test, free_noop)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    size_t capacity = 0;
    lugizmo::DFRowMajor<T>::MDSpan dataView{nullptr, 0, 0};
    T* data = nullptr;

    lugizmo::DFRowMajor<T>::Free(data, capacity, dataView, *memory);

    EXPECT_EQ(data, nullptr) << "Data should still be null after free.";
    EXPECT_EQ(capacity, 0) << "Capacity should remain zero.";
    EXPECT_EQ(dataView.extent(0), 0) << "Row count should remain zero.";
    EXPECT_EQ(dataView.extent(1), 0) << "Column count should remain zero.";
}

// ====== LAYOUT FILL TEST =================================================================================================================

TEST(lugizmo_dataframe_layout_row_base_test, fill_with_default_value)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data = static_cast<T*>(TestMemory.allocate(capacity * sizeof(T), alignof(T)));
    lugizmo::DFRowMajor<T>::FillRows(data, 0, rowCount, colCount, 99);

    for (size_t i = 0; i < rowCount * colCount; ++i) EXPECT_EQ(data[i], 99);
    TestMemory.deallocate(data, capacity * sizeof(T), alignof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, fill_with_span_values)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 4;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data = static_cast<T*>(TestMemory.allocate(capacity * sizeof(T), alignof(T)));

    std::vector<T> rowValues = {1, 2, 3, 4};
    std::span<T const> spanValues = rowValues;

    lugizmo::DFRowMajor<T>::FillRows(data, 0, rowCount, colCount, spanValues.begin(), spanValues.end());

    for (size_t row = 0; row < rowCount; ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], rowValues[col]);
        }
    }

    TestMemory.deallocate(data, capacity * sizeof(T), alignof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, fill_with_iterable_of_iterable)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data = static_cast<T*>(TestMemory.allocate(capacity * sizeof(T), alignof(T)));

    std::vector<std::vector<T>> matrix = {{1, 2, 3}, {4, 5, 6}};
    auto begin = matrix.begin();

    lugizmo::DFRowMajor<T>::FillRowByRow(data, 0, rowCount, colCount, begin, matrix.end());

    for (size_t row = 0; row < rowCount; ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], matrix[row][col]);
        }
    }

    TestMemory.deallocate(data, capacity * sizeof(T), alignof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, fill_with_initializer_list)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data             = static_cast<T*>(TestMemory.allocate(capacity * sizeof(T), alignof(T)));
    auto const initializer = std::initializer_list<std::initializer_list<T>>{{10, 11, 12}, {13, 14, 15}};
    auto begin             = initializer.begin();

    lugizmo::DFRowMajor<T>::FillRowByRow(data, 0, rowCount, colCount, begin, initializer.end());

    EXPECT_EQ(data[0], 10);
    EXPECT_EQ(data[1], 11);
    EXPECT_EQ(data[2], 12);
    EXPECT_EQ(data[3], 13);
    EXPECT_EQ(data[4], 14);
    EXPECT_EQ(data[5], 15);

    TestMemory.deallocate(data, capacity * sizeof(T), alignof(T));
}

TEST(lugizmo_dataframe_layout_row_base_test, fill_with_partial_iterable)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 3;  // Request 3 rows, but provide only 2
    constexpr size_t capacity = colCount * rowCount;

    auto* data = static_cast<T*>(TestMemory.allocate(capacity * sizeof(T), alignof(T)));

    std::vector<std::vector<T>> matrix = {{1, 2, 3}, {4, 5, 6}};
    auto begin = matrix.begin();

    lugizmo::DFRowMajor<T>::FillRowByRow(data, 0, rowCount, colCount, begin, matrix.end());

    for (size_t row = 0; row < 2; ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], matrix[row][col]);
        }
    }

    TestMemory.deallocate(data, capacity * sizeof(T), alignof(T));
}

// ====== EXPAND BY COUNTS TEST ============================================================================================================

struct lugizmo_dataframe_layout_row_test : testing::Test
{
protected:

    using T = int;
    using Layout = lugizmo::DFRowMajor<T>;
    using MDSpan = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>>;

    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    T* data         = nullptr;
    size_t capacity = 0;
    MDSpan dataView = MDSpan{nullptr, 0, 0};
    size_t colCount = 3;

    static void VerifyBuffer(T const* data, size_t const rowCount, size_t const colCount, const std::vector<std::vector<T>>& expected)
    {
        ASSERT_EQ(rowCount, expected.size()) << "Row count mismatch!";
        for (size_t row = 0; row < rowCount; ++row) {
            ASSERT_EQ(colCount, expected[row].size()) << "Column count mismatch at row " << row;
            for (size_t col = 0; col < colCount; ++col) {
                EXPECT_EQ(data[row * colCount + col], expected[row][col])
                    << "Mismatch at (" << row << ", " << col << ")";
            }
        }
    }

    void ExpandColumns(size_t const additionalCols, T const defaultValue = 0)
    {
        auto const rowC    = static_cast<size_t>(dataView.extent(0));
        auto const colC    = static_cast<size_t>(dataView.extent(1));
        auto const newColC = colC + additionalCols;

        if (newColC == colC) return;
        auto const newCapacity = rowC * newColC;

        auto* newData = static_cast<T*>(memory->allocate(newCapacity * sizeof(T), alignof(T)));

        for(size_t row = 0; row < rowC; ++row)
        {
            std::copy(data + row * colC, data + row * colC + colC, newData + row * newColC);
            std::fill(newData + row * newColC + colC, newData + (row + 1) * newColC, defaultValue);
        }

        if(data) memory->deallocate(data, capacity * sizeof(T));

        data     = newData;
        capacity = newCapacity;
        dataView = MDSpan{data, rowC, newColC};
    }

    void SetUp() override
    {
        ExpandColumns(colCount, 99);
    }

    void TearDown() override
    {
        if (data) {
            memory->deallocate(data, capacity * sizeof(T));
            data = nullptr;
            capacity = 0;
        }
    }
};

TEST_F(lugizmo_dataframe_layout_row_test, expand_with_default)
{
    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    constexpr T defaultValue = 42;
    Layout::ResizeRows(data, capacity, *memory, dataView, 3, 2, defaultValue);

    EXPECT_EQ(dataView.extent(0), 5);
    EXPECT_EQ(dataView.extent(1), colCount);

    for (size_t i = 0; i < 5 * static_cast<size_t>(dataView.extent(1)); ++i) {
        EXPECT_EQ(data[i], defaultValue);
    }
}

TEST_F(lugizmo_dataframe_layout_row_test, shrinking_records)
{
    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    constexpr T defaultValue = 0;
    Layout::ResizeRows(data, capacity, *memory, dataView, 3, 3, defaultValue);
    ASSERT_EQ(dataView.extent(0), 6);
    ASSERT_EQ(dataView.extent(1), colCount);

    // set unique values
    for (size_t row = 0; row < static_cast<size_t>(dataView.extent(0)); ++row)
        for (size_t col = 0; col < static_cast<size_t>(dataView.extent(1)); ++col)
            data[row * colCount + col] = static_cast<T>(col);

    // shrink
    Layout::ResizeRows(data, capacity, *memory, dataView, -2, -1, defaultValue);
    ASSERT_EQ(dataView.extent(0), 3);
    ASSERT_EQ(dataView.extent(1), colCount);

    // verify
    for (size_t row = 0; row < static_cast<size_t>(dataView.extent(0)); ++row)
        for (size_t col = 0; col < static_cast<size_t>(dataView.extent(1)); ++col)
            EXPECT_EQ(data[row * colCount + col], static_cast<T>(col));
}

TEST_F(lugizmo_dataframe_layout_row_test, adjust_with_span)
{
    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    std::vector<T> rowValues = {1, 2, 3};
    std::span<T const> const spanValues = rowValues;

    ASSERT_EQ(dataView.extent(1), spanValues.size());
    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 1, spanValues);

    ASSERT_EQ(dataView.extent(0), 3);
    ASSERT_EQ(dataView.extent(1), rowValues.size());

    VerifyBuffer(data, static_cast<size_t>(dataView.extent(0)), static_cast<size_t>(dataView.extent(1)), {rowValues, rowValues, rowValues});
}

TEST_F(lugizmo_dataframe_layout_row_test, adjust_with_iterable_of_iterable)
{
    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    std::vector<std::vector<T>> const matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    ASSERT_EQ(matrix.size(), 3);
    ASSERT_EQ(matrix[0].size(), 3);
    ASSERT_EQ(matrix[1].size(), 3);
    ASSERT_EQ(matrix[2].size(), 3);

    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 1, matrix);

    EXPECT_EQ(dataView.extent(0), 3);
    EXPECT_EQ(dataView.extent(1), 3);

    for (size_t row = 0; row < matrix.size(); ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], matrix[row][col]);
        }
    }
}

TEST_F(lugizmo_dataframe_layout_row_test, adjust_with_initializer_of_initializer)
{
    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 2, std::initializer_list<std::initializer_list<int>>{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}});

    EXPECT_EQ(dataView.extent(0), 4);
    EXPECT_EQ(dataView.extent(1), 3);

    EXPECT_EQ(data[0], 1);
    EXPECT_EQ(data[1], 2);
    EXPECT_EQ(data[2], 3);
    EXPECT_EQ(data[3], 4);
    EXPECT_EQ(data[4], 5);
    EXPECT_EQ(data[5], 6);
    EXPECT_EQ(data[6], 7);
    EXPECT_EQ(data[7], 8);
    EXPECT_EQ(data[8], 9);
    EXPECT_EQ(data[9], 10);
    EXPECT_EQ(data[10], 11);
    EXPECT_EQ(data[11], 12);
}

TEST_F(lugizmo_dataframe_layout_row_test, shrink_after_expand_initializer_of_initializer)
{
    using InitializerList = std::initializer_list<std::initializer_list<int>>;

    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 2, InitializerList{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}});
    Layout::ResizeRows(data, capacity, *memory, dataView, -1, -1, InitializerList{{7, 8, 9}, {7, 8, 9}});

    EXPECT_EQ(dataView.extent(0), 2);
}

TEST_F(lugizmo_dataframe_layout_row_test, expand_after_expand_initializer_of_initializer)
{
    using InitializerList = std::initializer_list<std::initializer_list<int>>;

    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 2, InitializerList{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}});

    EXPECT_EQ(dataView.extent(0), 4);
    EXPECT_EQ(dataView.extent(1), colCount);

    Layout::ResizeRows(data, capacity, *memory, dataView, 1, 1, InitializerList{{-1, -2, -3}, {13, 14, 15}});

    EXPECT_EQ(dataView.extent(0), 6);
    EXPECT_EQ(dataView.extent(1), colCount);

    EXPECT_EQ(data[0], -1);
    EXPECT_EQ(data[1], -2);
    EXPECT_EQ(data[2], -3);
    EXPECT_EQ(data[15], 13);
    EXPECT_EQ(data[16], 14);
    EXPECT_EQ(data[17], 15);
}

TEST_F(lugizmo_dataframe_layout_row_test, shrink_to_zero)
{
    using InitializerList = std::initializer_list<std::initializer_list<int>>;

    Layout::ResizeRows(data, capacity, *memory, dataView, 1, 1, InitializerList{{7, 8, 9}, {7, 8, 9}});
    Layout::ResizeRows(data, capacity, *memory, dataView, -1, -1, InitializerList{{7, 8, 9}, {7, 8, 9}}); // TODO you should not have to give values when shrinking (in both directions)

    // TODO data nullptr?
    // TODO capacity 0?
    EXPECT_EQ(dataView.extent(0), 0);
    EXPECT_EQ(dataView.extent(1), 0);
}