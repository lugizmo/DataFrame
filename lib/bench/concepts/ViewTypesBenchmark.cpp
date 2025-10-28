// Filename: DataFrame.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "benchmark/benchmark.h"

#include <array>
#include <cassert>
#include <mdspan>
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>
#include <ranges>

template <typename T, typename Layout>
struct MatrixView
{
    static_assert(std::is_same_v<Layout, std::layout_right> || std::is_same_v<Layout, std::layout_left>);
    using MDSpanT = std::mdspan<T, std::dextents<std::ptrdiff_t, 2>, Layout>;

    MDSpanT matrix;

    constexpr MatrixView(T* data, size_t const rows, size_t const cols) noexcept : matrix(data, rows, cols) {}

    [[nodiscard]]
    auto ColumnViewMDSpan(size_t targetColumn) noexcept -> std::mdspan<T, std::dextents<std::ptrdiff_t, 1>, std::layout_stride>
    {
        assert(targetColumn < matrix.extent(1) && "Column index out of bounds");

        if constexpr (std::is_same_v<Layout, std::layout_left>)
        {
            // layout is column-major, we can access the column directly
            return std::mdspan<T, std::dextents<std::ptrdiff_t, 1>>(matrix.data_handle() + targetColumn * matrix.stride(1), matrix.extent(0));
        }
        else
        {
            //  layout is row-major, we need to use layout_stride for non-contiguous access
            auto const extents = std::dextents<std::ptrdiff_t, 1>(matrix.extent(0));
            auto const strides = std::array<size_t, 1>{matrix.stride(1)};
            auto mapping = std::layout_stride::mapping(extents, strides);

            auto* columnStart = matrix.data_handle() + targetColumn * matrix.stride(1);
            return std::mdspan<T, std::dextents<std::ptrdiff_t, 1>, std::layout_stride>(columnStart, mapping);
        }
    }

    [[nodiscard]]
    auto RowViewMDSpan(size_t const targetRow) noexcept -> std::mdspan<T, std::dextents<std::ptrdiff_t, 1>, std::layout_stride>
    {
        assert(targetRow < matrix.extent(0) && "Row index out of bounds");

        if constexpr (std::is_same_v<Layout, std::layout_right>)
        {
            // layout is row-major, we can access the row directly
            return std::mdspan<T, std::dextents<std::ptrdiff_t, 1>>(matrix.data_handle() + targetRow * matrix.stride(0), matrix.extent(1));
        }
        else
        {
            // layout is column-major, we need to use layout_stride for non-contiguous access
            auto const extents = std::dextents<std::ptrdiff_t, 1>(matrix.extent(1));
            auto const strides = std::array<size_t, 1>{matrix.stride(0)};
            auto mapping       = std::layout_stride::mapping(extents, strides);

            auto* rowStart = matrix.data_handle() + targetRow * matrix.stride(0);
            return std::mdspan<T, std::dextents<std::ptrdiff_t, 1>, std::layout_stride>(rowStart, mapping);
        }
    }

    [[nodiscard]]
    auto ColumnViewRange(size_t const targetColumn) noexcept
    {
        assert(targetColumn < matrix.extent(1) && "Column index out of bounds");

        if constexpr (std::is_same_v<Layout, std::layout_left>)
        {
            auto* columnStart = matrix.data_handle() + targetColumn * matrix.stride(1);

            return std::ranges::views::iota(size_t{0}, matrix.extent(0)) |
                   std::ranges::views::transform([=, this](size_t i) -> T& { return columnStart[i]; });
        }
        else
        {
            auto const stride  = matrix.stride(1);
            auto* columnStart = matrix.data_handle() + targetColumn * matrix.stride(1);

            return std::ranges::views::iota(size_t{0}, matrix.extent(0)) |
                   std::ranges::views::transform([=, this](size_t i) -> T& { return columnStart[i * stride]; });
        }
    }

    [[nodiscard]]
    auto RowViewRange(size_t const targetRow) noexcept
    {
        assert(targetRow < matrix.extent(0) && "Row index out of bounds");

        if constexpr (std::is_same_v<Layout, std::layout_right>)
        {
            auto* rowStart = matrix.data_handle() + targetRow * matrix.stride(0);

            return std::ranges::views::iota(size_t{0}, matrix.extent(1)) |
                   std::ranges::views::transform([=, this](size_t j) -> T& { return rowStart[j]; });
        }
        else
        {
            auto* rowStart    = matrix.data_handle() + targetRow * matrix.stride(0);
            auto const stride = matrix.stride(0);

            return std::ranges::views::iota(size_t{0}, matrix.extent(1)) |
                   std::ranges::views::transform([=, this](size_t j) -> T& { return rowStart[j * stride]; });
        }
    }
};

static void ColumnViewMDSpanBenchmark_Right(benchmark::State& state)
{
    auto const     rows = state.range(0);
    constexpr auto cols = 1000;

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_right>(data.data(), rows, cols);
    auto colView = matrix.ColumnViewMDSpan(cols/2);

    for (auto _ : state)
    {
        for (size_t i = 0; i < colView.extent(0); ++i)
        {
            benchmark::DoNotOptimize(colView[i] *= 2);
        }
    }

    state.SetComplexityN(rows);
}

static void ColumnViewMDSpanBenchmark_Left(benchmark::State& state)
{
    auto const     rows = state.range(0);
    constexpr auto cols = 1000;

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_left>(data.data(), rows, cols);
    auto colView = matrix.ColumnViewMDSpan(cols/2);

    for (auto _ : state)
    {
        for (size_t i = 0; i < colView.extent(0); ++i)
        {
            benchmark::DoNotOptimize(colView[i] *= 2);
        }
    }

    state.SetComplexityN(rows);
}

static void ColumnViewMDSpanRandomAccessBenchmark_Right(benchmark::State& state)
{
    auto const     rows = state.range(0);
    constexpr auto cols = 1000;

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_right>(data.data(), rows, cols);
    auto colView = matrix.ColumnViewMDSpan(cols/2);

    std::vector<size_t> randomIndices(rows);
    std::iota(randomIndices.begin(), randomIndices.end(), 0);
    std::random_device   rd;
    std::mt19937         generator(rd());
    std::ranges::shuffle(randomIndices, generator);

    for (auto _ : state)
    {
        for (auto const randomIndex : randomIndices)
        {
            benchmark::DoNotOptimize(colView[randomIndex] *= 2);
        }
    }

    state.SetComplexityN(rows);
}

static void ColumnViewMDSpanRandomAccessBenchmark_Left(benchmark::State& state)
{
    auto const     rows = state.range(0);
    constexpr auto cols = 1000;

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_left>(data.data(), rows, cols);
    auto colView = matrix.ColumnViewMDSpan(cols/2);

    std::vector<size_t> randomIndices(rows);
    std::iota(randomIndices.begin(), randomIndices.end(), 0);
    std::random_device   rd;
    std::mt19937         generator(rd());
    std::ranges::shuffle(randomIndices, generator);

    for (auto _ : state)
    {
        for (auto const randomIndex : randomIndices)
        {
            benchmark::DoNotOptimize(colView[randomIndex] *= 2);
        }
    }

    state.SetComplexityN(rows);
}

static void RowViewMDSpanBenchmark_Right(benchmark::State& state)
{
    constexpr auto rows = 1000;
    auto const     cols = state.range(0);

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_right>(data.data(), rows, cols);
    auto rowView = matrix.RowViewMDSpan(rows / 2);

    for (auto _ : state)
    {
        for (size_t i = 0; i < rowView.extent(0); ++i)
        {
            benchmark::DoNotOptimize(rowView[i] *= 2);
        }
    }

    state.SetComplexityN(cols);
}

static void RowViewMDSpanBenchmark_Left(benchmark::State& state)
{
    constexpr auto rows = 1000;
    auto const     cols = state.range(0);

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_left>(data.data(), rows, cols);
    auto rowView = matrix.RowViewMDSpan(rows / 2);

    for (auto _ : state)
    {
        for (size_t i = 0; i < rowView.extent(0); ++i)
        {
            benchmark::DoNotOptimize(rowView[i] *= 2);
        }
    }

    state.SetComplexityN(cols);
}

static void RowViewMDSpanRandomAccessBenchmark_Right(benchmark::State& state)
{
    constexpr auto rows = 1000;
    auto const     cols = state.range(0);

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_right>(data.data(), rows, cols);
    auto rowView = matrix.RowViewMDSpan(rows / 2);

    std::vector<size_t> randomIndices(cols);
    std::iota(randomIndices.begin(), randomIndices.end(), 0);
    std::random_device rd;
    std::mt19937       generator(rd());
    std::ranges::shuffle(randomIndices, generator);

    for (auto _ : state)
    {
        for (auto const randomIndex : randomIndices)
        {
            benchmark::DoNotOptimize(rowView[randomIndex] *= 2);
        }
    }

    state.SetComplexityN(cols);
}

static void RowViewMDSpanRandomAccessBenchmark_Left(benchmark::State& state)
{
    constexpr auto rows = 1000;
    auto const     cols = state.range(0);

    auto data    = std::vector<int>(rows * cols, 1);
    auto matrix  = MatrixView<int, std::layout_left>(data.data(), rows, cols);
    auto rowView = matrix.RowViewMDSpan(rows / 2);

    std::vector<size_t> randomIndices(cols);
    std::iota(randomIndices.begin(), randomIndices.end(), 0);
    std::random_device rd;
    std::mt19937       generator(rd());
    std::ranges::shuffle(randomIndices, generator);

    for (auto _ : state)
    {
        for (auto const randomIndex : randomIndices)
        {
            benchmark::DoNotOptimize(rowView[randomIndex] *= 2);
        }
    }

    state.SetComplexityN(cols);
}

static void ColumnViewRangeBenchmark_Right(benchmark::State& state)
{
    auto const rows = state.range(0);
    constexpr auto cols = 1000;

    auto data         = std::vector<int>(rows * cols, 1);
    auto matrix       = MatrixView<int, std::layout_right>(data.data(), rows, cols);
    auto colViewRange = matrix.ColumnViewRange(cols / 2);

    for (auto _ : state)
    {
        for (auto& elem : colViewRange)
        {
            benchmark::DoNotOptimize(elem *= 2);
        }
    }

    state.SetComplexityN(rows);
}

static void ColumnViewRangeBenchmark_Left(benchmark::State& state)
{
    auto const rows = state.range(0);
    constexpr auto cols = 1000;

    auto data         = std::vector<int>(rows * cols, 1);
    auto matrix       = MatrixView<int, std::layout_left>(data.data(), rows, cols);
    auto colViewRange = matrix.ColumnViewRange(cols / 2);

    for (auto _ : state)
    {
        for (auto& elem : colViewRange)
        {
            benchmark::DoNotOptimize(elem *= 2);
        }
    }

    state.SetComplexityN(rows);
}

static void RowViewRangeBenchmark_Right(benchmark::State& state)
{
    constexpr auto rows = 1000;
    auto const cols = state.range(0);

    auto data         = std::vector<int>(rows * cols, 1);
    auto matrix       = MatrixView<int, std::layout_right>(data.data(), rows, cols);
    auto rowViewRange = matrix.RowViewRange(rows / 2);

    for (auto _ : state)
    {
        for (auto& elem : rowViewRange)
        {
            benchmark::DoNotOptimize(elem *= 2);
        }
    }

    state.SetComplexityN(cols);
}

static void RowViewRangeBenchmark_Left(benchmark::State& state)
{
    constexpr auto rows = 1000;
    auto const cols = state.range(0);

    auto data         = std::vector<int>(rows * cols, 1);
    auto matrix       = MatrixView<int, std::layout_left>(data.data(), rows, cols);
    auto rowViewRange = matrix.RowViewRange(rows / 2);

    for (auto _ : state)
    {
        for (auto& elem : rowViewRange)
        {
            benchmark::DoNotOptimize(elem *= 2);
        }
    }

    state.SetComplexityN(cols);
}

//BENCHMARK(ColumnViewMDSpanBenchmark_Right)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(ColumnViewMDSpanBenchmark_Left)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(ColumnViewMDSpanRandomAccessBenchmark_Right)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(ColumnViewMDSpanRandomAccessBenchmark_Left)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();

//BENCHMARK(RowViewMDSpanBenchmark_Right)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(RowViewMDSpanBenchmark_Left)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(RowViewMDSpanRandomAccessBenchmark_Right)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(RowViewMDSpanRandomAccessBenchmark_Left)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();

//BENCHMARK(ColumnViewRangeBenchmark_Right)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(ColumnViewRangeBenchmark_Left)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();

//BENCHMARK(RowViewRangeBenchmark_Right)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();
//BENCHMARK(RowViewRangeBenchmark_Left)->RangeMultiplier(2)->Range(512, 512 * 1024)->Complexity();