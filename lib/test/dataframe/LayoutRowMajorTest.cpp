// Filename: LayoutRowMajorTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the DFRowMajor row-major storage layout.
// The following functions are tested here (with names of tests):
//
// DataframeLayoutRowBase (static buffer helpers):
// ✅ Realloc* / ReallocAndShift  - Realloc / ReallocAndShift (grow + shift)
// ✅ Free / FreeNoop             - Free
// ✅ FillWith*                   - FillRows / FillRowByRow
//
// DataframeLayoutRow (ResizeRows over a live buffer):
// ✅ ExpandWithDefault / ShrinkingRecords                 - ResizeRows(default value)
// ✅ AdjustWith*                                          - ResizeRows(span / iterable / initializer)
// ✅ ShrinkAfterExpand / ExpandAfterExpand / ShrinkToZero - repeated ResizeRows
// ✅ ResizeColumns                                        - ResizeCols grow/shrink at both ends and shrink-to-zero
// ✅ DropRow / DropColumn                                 - direct in-place removal and compaction
// ✅ ReorderRows / ReorderColumns                         - direct permutations and large-buffer scratch paths
// ✅ NonTrivialLifetimes                                  - construction/destruction balance across layout mutations
//

#include "gtest/gtest.h"

#include <array>
#include <memory>
#include <memory_resource>
#include <numeric>
#include <vector>
#include <span>
#include <initializer_list>

#include "lugizmo/dataframe/LayoutRowMajor.h"

namespace {

    template<typename T, std::size_t N>
    void ExpectValues(T const* const data, std::array<T, N> const& expected)
    {
        for(std::size_t i = 0; i < N; ++i) EXPECT_EQ(data[i], expected[i]) << "Mismatch at position " << i;
    }

    struct LifetimeValue
    {
        inline static std::size_t alive       = 0;
        inline static std::size_t constructed = 0;
        inline static std::size_t destroyed   = 0;

        int value = 0;

        explicit LifetimeValue(int const initial = 0) noexcept : value(initial)
        {
            ++alive;
            ++constructed;
        }

        LifetimeValue(LifetimeValue const& other) noexcept : value(other.value)
        {
            ++alive;
            ++constructed;
        }

        LifetimeValue(LifetimeValue&& other) noexcept : value(other.value)
        {
            other.value = -1;
            ++alive;
            ++constructed;
        }

        auto operator=(LifetimeValue const& other) noexcept -> LifetimeValue&
        {
            value = other.value;
            return *this;
        }

        auto operator=(LifetimeValue&& other) noexcept -> LifetimeValue&
        {
            value = other.value;
            other.value = -1;
            return *this;
        }

        ~LifetimeValue()
        {
            --alive;
            ++destroyed;
        }

        static void Reset() noexcept
        {
            alive = constructed = destroyed = 0;
        }
    };

} // namespace

// ====== LAYOUT ALLOCATE TEST =============================================================================================================

TEST(DataframeLayoutRowBase, Realloc)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t  colCount      = 3;
    constexpr size_t  rowCount      = 2;
    constexpr size_t  newRowCount   = 4;
    constexpr ssize_t adjCountByBeg = 1;

    size_t capacity = 6;

    // Allocate initial data and initialize
    auto* data = lugizmo::internal::AllocateAligned<T>(*memory, capacity);
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

    lugizmo::internal::DeallocateAligned(*memory, data, capacity);
}

TEST(DataframeLayoutRowBase, ReallocZeroCapacity)
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

    lugizmo::internal::DeallocateAligned(*memory, data, capacity);
}

TEST(DataframeLayoutRowBase, ReallocLargeAllocation)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 5;
    constexpr size_t rowCount = 200;
    constexpr size_t newRowCount = 1000;
    constexpr ssize_t adjCountByBeg = 10;

    size_t capacity = 1024;
    auto* data = lugizmo::internal::AllocateAligned<T>(*memory, capacity);
    for (size_t i = 0; i < rowCount * colCount; ++i) data[i] = static_cast<T>(i);

    lugizmo::DFRowMajor<T>::Realloc(data, *memory, capacity, colCount, rowCount, newRowCount, adjCountByBeg);

    EXPECT_NE(data, nullptr) << "Reallocated data should not be null.";
    EXPECT_GE(capacity, newRowCount * colCount) << "Capacity should be large enough for new rows.";

    lugizmo::internal::DeallocateAligned(*memory, data, capacity);
}

TEST(DataframeLayoutRowBase, ReallocAndShift)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t newRowCount = 4;
    constexpr ssize_t adjCountByBeg = 1;

    size_t capacity = 6;

    // Allocate initial data and initialize
    auto* data = lugizmo::internal::AllocateAligned<T>(*memory, capacity);
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

    lugizmo::internal::DeallocateAligned(*memory, data, capacity);
}

TEST(DataframeLayoutRowBase, Free)
{
    using T = int;
    std::pmr::memory_resource* memory = std::pmr::get_default_resource();

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;

    size_t capacity = 6;

    // Allocate memory
    auto* data = lugizmo::internal::AllocateAligned<T>(*memory, capacity);
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

TEST(DataframeLayoutRowBase, FreeNoop)
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

TEST(DataframeLayoutRowBase, FillWithDefaultValue)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data = lugizmo::internal::AllocateAligned<T>(TestMemory, capacity);
    lugizmo::DFRowMajor<T>::FillRows(data, 0, rowCount, colCount, 99);

    for (size_t i = 0; i < rowCount * colCount; ++i) EXPECT_EQ(data[i], 99);
    lugizmo::internal::DeallocateAligned(TestMemory, data, capacity);
}

TEST(DataframeLayoutRowBase, FillWithSpanValues)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 4;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data = lugizmo::internal::AllocateAligned<T>(TestMemory, capacity);

    std::vector<T> rowValues = {1, 2, 3, 4};
    std::span<T const> spanValues = rowValues;

    lugizmo::DFRowMajor<T>::FillRows(data, 0, rowCount, colCount, spanValues.begin(), spanValues.end());

    for (size_t row = 0; row < rowCount; ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], rowValues[col]);
        }
    }

    lugizmo::internal::DeallocateAligned(TestMemory, data, capacity);
}

TEST(DataframeLayoutRowBase, FillWithIterableOfIterable)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data = lugizmo::internal::AllocateAligned<T>(TestMemory, capacity);

    std::vector<std::vector<T>> matrix = {{1, 2, 3}, {4, 5, 6}};
    auto begin = matrix.begin();

    lugizmo::DFRowMajor<T>::FillRowByRow(data, 0, rowCount, colCount, begin, matrix.end());

    for (size_t row = 0; row < rowCount; ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], matrix[row][col]);
        }
    }

    lugizmo::internal::DeallocateAligned(TestMemory, data, capacity);
}

TEST(DataframeLayoutRowBase, FillWithInitializerList)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 2;
    constexpr size_t capacity = colCount * rowCount;

    auto* data             = lugizmo::internal::AllocateAligned<T>(TestMemory, capacity);
    auto const initializer = std::initializer_list<std::initializer_list<T>>{{10, 11, 12}, {13, 14, 15}};
    auto begin             = initializer.begin();

    lugizmo::DFRowMajor<T>::FillRowByRow(data, 0, rowCount, colCount, begin, initializer.end());

    EXPECT_EQ(data[0], 10);
    EXPECT_EQ(data[1], 11);
    EXPECT_EQ(data[2], 12);
    EXPECT_EQ(data[3], 13);
    EXPECT_EQ(data[4], 14);
    EXPECT_EQ(data[5], 15);

    lugizmo::internal::DeallocateAligned(TestMemory, data, capacity);
}

TEST(DataframeLayoutRowBase, FillWithPartialIterable)
{
    using T = int;
    auto TestBuffer = std::array<std::byte, 8192>();
    auto TestMemory = std::pmr::monotonic_buffer_resource(TestBuffer.data(), TestBuffer.size());

    constexpr size_t colCount = 3;
    constexpr size_t rowCount = 3;  // Request 3 rows, but provide only 2
    constexpr size_t capacity = colCount * rowCount;

    auto* data = lugizmo::internal::AllocateAligned<T>(TestMemory, capacity);

    std::vector<std::vector<T>> matrix = {{1, 2, 3}, {4, 5, 6}};
    auto begin = matrix.begin();

    lugizmo::DFRowMajor<T>::FillRowByRow(data, 0, rowCount, colCount, begin, matrix.end());

    for (size_t row = 0; row < 2; ++row) {
        for (size_t col = 0; col < colCount; ++col) {
            EXPECT_EQ(data[row * colCount + col], matrix[row][col]);
        }
    }

    lugizmo::internal::DeallocateAligned(TestMemory, data, capacity);
}

// ====== EXPAND BY COUNTS TEST ============================================================================================================

struct DataframeLayoutRow : testing::Test
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

        auto* newData = lugizmo::internal::AllocateAligned<T>(*memory, newCapacity);

        for(size_t row = 0; row < rowC; ++row)
        {
            std::copy(data + row * colC, data + row * colC + colC, newData + row * newColC);
            std::fill(newData + row * newColC + colC, newData + (row + 1) * newColC, defaultValue);
        }

        lugizmo::internal::DeallocateAligned(*memory, data, capacity);

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
            lugizmo::internal::DeallocateAligned(*memory, data, capacity);
            data = nullptr;
            capacity = 0;
        }
    }
};

TEST_F(DataframeLayoutRow, ExpandWithDefault)
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

TEST_F(DataframeLayoutRow, ShrinkingRecords)
{
    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    constexpr T defaultValue = 0;
    Layout::ResizeRows(data, capacity, *memory, dataView, 3, 3, defaultValue);
    ASSERT_EQ(dataView.extent(0), 6);
    ASSERT_EQ(dataView.extent(1), colCount);

    // set row-distinct values, so retaining the wrong rows is observable
    for (size_t row = 0; row < static_cast<size_t>(dataView.extent(0)); ++row)
        for (size_t col = 0; col < static_cast<size_t>(dataView.extent(1)); ++col)
            data[row * colCount + col] = static_cast<T>(row * 10 + col);

    // shrink
    Layout::ResizeRows(data, capacity, *memory, dataView, -2, -1, defaultValue);
    ASSERT_EQ(dataView.extent(0), 3);
    ASSERT_EQ(dataView.extent(1), colCount);

    // verify
    for (size_t row = 0; row < static_cast<size_t>(dataView.extent(0)); ++row)
        for (size_t col = 0; col < static_cast<size_t>(dataView.extent(1)); ++col)
            EXPECT_EQ(data[row * colCount + col], static_cast<T>((row + 2) * 10 + col));
}

TEST_F(DataframeLayoutRow, AdjustWithSpan)
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

TEST_F(DataframeLayoutRow, AdjustWithIterableOfIterable)
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

TEST_F(DataframeLayoutRow, AdjustWithInitializerOfInitializer)
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

TEST_F(DataframeLayoutRow, ShrinkAfterExpandInitializer)
{
    using InitializerList = std::initializer_list<std::initializer_list<int>>;

    ASSERT_EQ(dataView.extent(0), 0);
    ASSERT_EQ(dataView.extent(1), colCount);

    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 2, InitializerList{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}});
    Layout::ResizeRows(data, capacity, *memory, dataView, -1, -1, InitializerList{{7, 8, 9}, {7, 8, 9}});

    EXPECT_EQ(dataView.extent(0), 2);
    VerifyBuffer(data, 2, colCount, {{4, 5, 6}, {7, 8, 9}});
}

TEST_F(DataframeLayoutRow, ExpandAfterExpandInitializer)
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

TEST_F(DataframeLayoutRow, ShrinkToZero)
{
    using InitializerList = std::initializer_list<std::initializer_list<int>>;

    Layout::ResizeRows(data, capacity, *memory, dataView, 1, 1, InitializerList{{7, 8, 9}, {7, 8, 9}});
    Layout::ResizeRows(data, capacity, *memory, dataView, -1, -1, InitializerList{{7, 8, 9}, {7, 8, 9}}); // TODO you should not have to give values when shrinking (in both directions)

    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(capacity, 0);
    EXPECT_EQ(dataView.extent(0), 0);
    EXPECT_EQ(dataView.extent(1), 0);
}

/**
 * @brief ResizeCols preserves overlap, fills both added sides, and releases storage when all columns are removed.
 * @see   lugizmo::DFRowMajor::ResizeCols
 */
TEST_F(DataframeLayoutRow, ResizeColumns)
{
    auto const rows = std::array{std::array{1, 2, 3}, std::array{4, 5, 6}};
    Layout::ResizeRows(data, capacity, *memory, dataView, 2, 0, rows);

    Layout::ResizeCols(data, capacity, *memory, dataView, 1, 1, 9);
    VerifyBuffer(data, 2, 5, {{9, 1, 2, 3, 9}, {9, 4, 5, 6, 9}});

    Layout::ResizeCols(data, capacity, *memory, dataView, -1, -1, 0);
    VerifyBuffer(data, 2, 3, {{1, 2, 3}, {4, 5, 6}});

    Layout::ResizeCols(data, capacity, *memory, dataView, -1, -2, 0);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(capacity, 0);
    EXPECT_EQ(dataView.extent(0), 2);
    EXPECT_EQ(dataView.extent(1), 0);
}

/**
 * @brief DropRow compacts first, middle, last, and only-row removals without changing capacity.
 * @see   lugizmo::DFRowMajor::DropRow
 */
TEST(DataframeLayoutRowMutation, DropRow)
{
    using Layout = lugizmo::DFRowMajor<int>;
    auto* const memory = std::pmr::get_default_resource();
    size_t capacity = 12;
    auto* data = lugizmo::internal::AllocateAligned<int>(*memory, capacity);
    auto const initial = std::array{0, 1, 2, 10, 11, 12, 20, 21, 22, 30, 31, 32};
    std::uninitialized_copy(initial.begin(), initial.end(), data);
    auto view = Layout::MDSpan{data, 4, 3};

    Layout::DropRow(data, capacity, *memory, view, 1);
    ExpectValues(data, std::array{0, 1, 2, 20, 21, 22, 30, 31, 32});

    Layout::DropRow(data, capacity, *memory, view, 0);
    ExpectValues(data, std::array{20, 21, 22, 30, 31, 32});

    Layout::DropRow(data, capacity, *memory, view, 1);
    ExpectValues(data, std::array{20, 21, 22});

    Layout::DropRow(data, capacity, *memory, view, 0);
    EXPECT_EQ(view.extent(0), 0);
    EXPECT_EQ(capacity, 12);
    Layout::Free(data, capacity, view, *memory);
}

/**
 * @brief DropColumn compacts first, middle, last, and only-column removals across every row.
 * @see   lugizmo::DFRowMajor::DropColumn
 */
TEST(DataframeLayoutRowMutation, DropColumn)
{
    using Layout = lugizmo::DFRowMajor<int>;
    auto* const memory = std::pmr::get_default_resource();
    size_t capacity = 8;
    auto* data = lugizmo::internal::AllocateAligned<int>(*memory, capacity);
    auto const initial = std::array{0, 1, 2, 3, 10, 11, 12, 13};
    std::uninitialized_copy(initial.begin(), initial.end(), data);
    auto view = Layout::MDSpan{data, 2, 4};

    Layout::DropColumn(data, capacity, *memory, view, 1);
    ExpectValues(data, std::array{0, 2, 3, 10, 12, 13});

    Layout::DropColumn(data, capacity, *memory, view, 0);
    ExpectValues(data, std::array{2, 3, 12, 13});

    Layout::DropColumn(data, capacity, *memory, view, 1);
    ExpectValues(data, std::array{2, 12});

    Layout::DropColumn(data, capacity, *memory, view, 0);
    EXPECT_EQ(view.extent(1), 0);
    EXPECT_EQ(capacity, 8);
    Layout::Free(data, capacity, view, *memory);
}

/**
 * @brief ReorderRows applies identity and multi-cycle new-to-old permutations.
 * @see   lugizmo::DFRowMajor::ReorderRows
 */
TEST(DataframeLayoutRowMutation, ReorderRows)
{
    using Layout = lugizmo::DFRowMajor<int>;
    auto* const memory = std::pmr::get_default_resource();
    size_t capacity = 6;
    auto* data = lugizmo::internal::AllocateAligned<int>(*memory, capacity);
    std::uninitialized_copy_n(std::array{0, 1, 10, 11, 20, 21}.begin(), capacity, data);
    auto view = Layout::MDSpan{data, 3, 2};

    Layout::ReorderRows(data, capacity, *memory, view, std::array<size_t, 3>{2, 0, 1});
    ExpectValues(data, std::array{20, 21, 0, 1, 10, 11});

    Layout::ReorderRows(data, capacity, *memory, view, std::array<size_t, 3>{0, 1, 2});
    ExpectValues(data, std::array{20, 21, 0, 1, 10, 11});
    Layout::Free(data, capacity, view, *memory);
}

/**
 * @brief ReorderColumns applies identity and multi-cycle new-to-old permutations to every row.
 * @see   lugizmo::DFRowMajor::ReorderColumns
 */
TEST(DataframeLayoutRowMutation, ReorderColumns)
{
    using Layout = lugizmo::DFRowMajor<int>;
    auto* const memory = std::pmr::get_default_resource();
    size_t capacity = 6;
    auto* data = lugizmo::internal::AllocateAligned<int>(*memory, capacity);
    std::uninitialized_copy_n(std::array{0, 1, 2, 10, 11, 12}.begin(), capacity, data);
    auto view = Layout::MDSpan{data, 2, 3};

    Layout::ReorderColumns(data, capacity, *memory, view, std::array<size_t, 3>{2, 0, 1});
    ExpectValues(data, std::array{2, 0, 1, 12, 10, 11});

    Layout::ReorderColumns(data, capacity, *memory, view, std::array<size_t, 3>{0, 1, 2});
    ExpectValues(data, std::array{2, 0, 1, 12, 10, 11});
    Layout::Free(data, capacity, view, *memory);
}

/**
 * @brief Large row reordering uses the in-place scratch path and preserves the backing allocation.
 * @see   lugizmo::DFRowMajor::ReorderRows
 */
TEST(DataframeLayoutRowMutation, ReorderRowsLargeBuffer)
{
    using T = std::uint64_t;
    using Layout = lugizmo::DFRowMajor<T>;
    auto* const memory = std::pmr::get_default_resource();
    constexpr size_t rows = 2;
    constexpr size_t cols = (64UZ * 1024UZ * 1024UZ) / (rows * sizeof(T)) + 1;
    size_t capacity = rows * cols;
    auto* data = lugizmo::internal::AllocateAligned<T>(*memory, capacity);
    std::uninitialized_fill_n(data, cols, 1);
    std::uninitialized_fill_n(data + cols, cols, 2);
    auto view = Layout::MDSpan{data, rows, cols};
    auto* const original = data;

    Layout::ReorderRows(data, capacity, *memory, view, std::array<size_t, 2>{1, 0});

    EXPECT_EQ(data, original);
    EXPECT_EQ(data[0], 2);
    EXPECT_EQ(data[cols - 1], 2);
    EXPECT_EQ(data[cols], 1);
    EXPECT_EQ(data[capacity - 1], 1);
    Layout::Free(data, capacity, view, *memory);
}

/**
 * @brief Large column reordering uses the per-row scratch path and preserves the backing allocation.
 * @see   lugizmo::DFRowMajor::ReorderColumns
 */
TEST(DataframeLayoutRowMutation, ReorderColumnsLargeBuffer)
{
    using T = std::uint64_t;
    using Layout = lugizmo::DFRowMajor<T>;
    auto* const memory = std::pmr::get_default_resource();
    constexpr size_t cols = 2;
    constexpr size_t rows = (64UZ * 1024UZ * 1024UZ) / (cols * sizeof(T)) + 1;
    size_t capacity = rows * cols;
    auto* data = lugizmo::internal::AllocateAligned<T>(*memory, capacity);
    for(size_t row = 0; row < rows; ++row)
    {
        std::construct_at(data + row * cols, 1);
        std::construct_at(data + row * cols + 1, 2);
    }
    auto view = Layout::MDSpan{data, rows, cols};
    auto* const original = data;

    Layout::ReorderColumns(data, capacity, *memory, view, std::array<size_t, 2>{1, 0});

    EXPECT_EQ(data, original);
    EXPECT_EQ(data[0], 2);
    EXPECT_EQ(data[1], 1);
    EXPECT_EQ(data[capacity - 2], 2);
    EXPECT_EQ(data[capacity - 1], 1);
    Layout::Free(data, capacity, view, *memory);
}

/**
 * @brief Non-trivial values remain balanced across resize, drop, reorder, and free operations.
 * @see   lugizmo::DFRowMajor
 */
TEST(DataframeLayoutRowMutation, NonTrivialLifetimes)
{
    using Layout = lugizmo::DFRowMajor<LifetimeValue>;
    auto* const memory = std::pmr::get_default_resource();
    LifetimeValue::Reset();

    {
        auto defaultValue = LifetimeValue(9);
        size_t capacity = 0;
        LifetimeValue* data = nullptr;
        auto view = Layout::MDSpan{data, 0, 3};

        Layout::ResizeRows(data, capacity, *memory, view, 2, 0, defaultValue);
        ASSERT_EQ(LifetimeValue::alive, 7);
        for(size_t i = 0; i < 6; ++i) data[i].value = static_cast<int>(i + 1);

        Layout::ResizeCols(data, capacity, *memory, view, 1, 0, defaultValue);
        EXPECT_EQ(LifetimeValue::alive, 9);
        EXPECT_EQ(data[0].value, 9);
        EXPECT_EQ(data[1].value, 1);

        Layout::DropColumn(data, capacity, *memory, view, 1);
        EXPECT_EQ(LifetimeValue::alive, 7);
        EXPECT_EQ(data[0].value, 9);
        EXPECT_EQ(data[1].value, 2);

        Layout::ReorderRows(data, capacity, *memory, view, std::array<size_t, 2>{1, 0});
        EXPECT_EQ(LifetimeValue::alive, 7);
        EXPECT_EQ(data[0].value, 9);
        EXPECT_EQ(data[1].value, 5);

        Layout::DropRow(data, capacity, *memory, view, 0);
        EXPECT_EQ(LifetimeValue::alive, 4);
        EXPECT_EQ(data[0].value, 9);
        EXPECT_EQ(data[1].value, 2);

        Layout::Free(data, capacity, view, *memory);
        EXPECT_EQ(LifetimeValue::alive, 1);
    }

    EXPECT_EQ(LifetimeValue::alive, 0);
    EXPECT_EQ(LifetimeValue::constructed, LifetimeValue::destroyed);
}
