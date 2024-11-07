#include "gtest/gtest.h"

#include <iostream>
#include <chrono>
#include <sys/mman.h>
#include <memory_resource>
#include <string>
#include <array>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <numeric>

#include "lugizmo/DataFrame.h"

#define PRINT_ALLOCATIONS 0

template<typename Allocator>
class LoggingMemoryResource final : public std::pmr::memory_resource
{
    std::unique_ptr<Allocator> allocator = std::make_unique<Allocator>();

protected:

    void* do_allocate(std::size_t bytes, std::size_t alignment) override
    {
#if PRINT_ALLOCATIONS
        std::cout << "Allocating " << bytes << " bytes with alignment " << alignment << std::endl;
#endif
        return allocator->allocate(bytes, alignment);
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override
    {
#if PRINT_ALLOCATIONS
        std::cout << "Deallocating " << bytes << " bytes with alignment " << alignment << std::endl;
#endif
        allocator->deallocate(ptr, bytes, alignment);
    }

    [[nodiscard]]
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

auto AllocateMMAP(size_t const size) -> std::byte*
{
    void* mmapPtr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmapPtr == MAP_FAILED) return nullptr;

    return static_cast<std::byte*>(mmapPtr);
}

void DeallocateMMAP(std::byte* ptr, size_t const size)
{
    munmap(ptr, size);
}

TEST(dataframe_test, empty_initialization)
{
    using namespace lugizmo;

    DataFrame<int, int, int> const df;
    ASSERT_TRUE(df.Empty());
}

TEST(dataframe_test, row_major_set_get_records)
{
    using namespace lugizmo;
    using DF = DataFrame<int, int, int>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;
    constexpr auto ARRAY    = std::array{1, 2, 3, 4, 5};
    static_assert(ARRAY.size() == COL_COUNT);

    auto df = DF();

    for(auto col = 0; col < COL_COUNT; ++col)
        ASSERT_TRUE(df.AddField(col + 2));

    for(auto row = 0; row < ROW_COUNT; ++row)
        ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_TRUE(df.SetValue(col + 2, row, ARRAY[col] + 2));

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col + 2, row), ARRAY[col] + 2);
}

TEST(dataframe_test, row_major_set_get_field_views)
{
    using namespace lugizmo;
    using DF = DataFrame<int, int, int>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;
    constexpr auto ARRAY    = std::array{1, 2, 3, 4, 5};
    static_assert(ARRAY.size() == COL_COUNT);

    auto df = DF();

    for(auto col = 0; col < COL_COUNT; ++col)
        ASSERT_TRUE(df.AddField(col));

    for(auto row = 0; row < ROW_COUNT; ++row)
        ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));

    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto view = df.GetField(col);
        ASSERT_TRUE(view.has_value());

        for(auto& val : *view)
            ASSERT_EQ(val, ARRAY[col]);

        for(auto& val : *view)
            val = 42;
    }

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col, row), 42);
}

TEST(dataframe_test, row_major_set_get_record_views)
{
    using namespace lugizmo;
    using DF = DataFrame<int, int, int>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;
    constexpr auto ARRAY    = std::array{1, 2, 3, 4, 5};
    static_assert(ARRAY.size() == COL_COUNT);

    auto df = DF();

    for(auto col = 0; col < COL_COUNT; ++col)
        ASSERT_TRUE(df.AddField(col));

    for(auto row = 0; row < ROW_COUNT; ++row)
        ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));

    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto view = df.GetRecord(row);
        ASSERT_TRUE(view.has_value());

        for(auto col = 0; col < COL_COUNT; ++col)
            ASSERT_EQ((*view)[col], ARRAY[col]);

        for(auto& val : *view)
            val = 42;
    }

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col, row), 42);
}

TEST(dataframe_test, drop)
{
    using namespace lugizmo;
    using DF = DataFrame<int, int, int>;

    constexpr int COL_COUNT = 10;
    constexpr int ROW_COUNT = 10;
    constexpr auto ARRAY    = std::array{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    static_assert(ARRAY.size() == COL_COUNT);

    auto df = DF();

    for(auto col = 0; col < COL_COUNT; ++col)
        ASSERT_TRUE(df.AddField(col));

    for(auto row = 0; row < ROW_COUNT; ++row)
        ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));

    // drop columns/fields
    for(auto col = 0; col < COL_COUNT; ++col)
        if(col % 2 == 0) ASSERT_TRUE(df.DropField(col));

    ASSERT_EQ(df.Fields().size(), COL_COUNT/2);

    int value = 2;
    for(auto const fld: df.Fields())
    {
        ASSERT_TRUE(fld % 2 != 0);

        auto const get = df.GetField(fld);
        ASSERT_TRUE(get.has_value());
        ASSERT_TRUE(std::ranges::all_of(*get, [value](auto const v) { return v == value; }));

        value += 2;
    }

    // drop rows/records
    for(auto row = 0; row < ROW_COUNT; ++row)
        if(row % 2 == 0) ASSERT_TRUE(df.DropRecord(row));

    ASSERT_EQ(df.Records().size(), ROW_COUNT/2);

    for(auto const rec: df.Records())
    {
        ASSERT_TRUE(rec % 2 != 0);

        auto const get = df.GetRecord(rec);
        ASSERT_TRUE(get.has_value());

        auto recValue = 0;
        ASSERT_TRUE(std::ranges::all_of(*get, [&recValue](auto const v) { recValue += 2; return v == recValue; }));
    }

    // After row/col drop the uneven are first:  1, 3, 5, 7, 9,  0, 2, 4, 6, 8
    constexpr auto ARRAY_AFTER_DROP = std::array{2, 4, 6, 8, 10, 1, 3, 5, 7, 9};
    static_assert(ARRAY_AFTER_DROP.size() == COL_COUNT);

    // add values to check if drop is resetting indices in memory
    for(auto col = 0; col < COL_COUNT; ++col)
        if(col % 2 == 0) ASSERT_TRUE(df.AddField(col, col + 1));

    for(auto row = 0; row < ROW_COUNT; ++row)
        if(row % 2 == 0) ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY_AFTER_DROP));
}

TEST(dataframe_test, dev)
{
    using namespace lugizmo;
    using namespace std::chrono;
    using ms = milliseconds;

    constexpr int COL_COUNT = 10;
    constexpr int ROW_COUNT = 3'000'000;

    auto const loggingRes =
#if PRINT_ALLOCATIONS
        std::make_shared<LoggingMemoryResource<std::pmr::monotonic_buffer_resource>>();
#else
        std::shared_ptr<std::pmr::memory_resource>(std::pmr::get_default_resource(), [](std::pmr::memory_resource*) {});
        //std::make_shared<std::pmr::unsynchronized_pool_resource>();
#endif

    auto start = high_resolution_clock::now();
    DataFrame<int, int, int> df{0, loggingRes};
    auto end = high_resolution_clock::now();
    std::cout << "Creating empty (with cap.) dataframe took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(int i = 0; i < COL_COUNT; ++i)
    {
        //ASSERT_TRUE(df.AddField(std::format("col{}", i)));
        ASSERT_TRUE(df.AddField(i));
    }
    ASSERT_TRUE(df.Empty());
    end = high_resolution_clock::now();
    std::cout << "Adding " << COL_COUNT << " fields took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    auto CreateArray = [](int i) -> std::array<int, COL_COUNT>{
        std::array<int, COL_COUNT> arr{};
        std::iota(arr.begin(), arr.end(), i);
        return arr;
    };

    start = high_resolution_clock::now();
    for(int i = 0; i < ROW_COUNT; ++i)
    {
        ASSERT_TRUE(df.AddRecordPopulated(i, CreateArray(0)));
    }
    end = high_resolution_clock::now();
    std::cout << "Adding " << ROW_COUNT << " records took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        for(auto row = 0; row < ROW_COUNT; ++row)
        {
            //auto const get = df.GetValue(std::format("col{}", col), row);
            auto const get = df.GetValue(col, row);
            ASSERT_TRUE(get.has_value());
        }
    }
    end = high_resolution_clock::now();
    std::cout << "Getting() " << ROW_COUNT * COL_COUNT << " values took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        for(auto row = 0; row < ROW_COUNT; ++row)
        {
            //auto const get = df[std::format("col{}", col), row];
            auto const get = df[col, row];
            ASSERT_TRUE(get >= 0);
        }
    }
    end = high_resolution_clock::now();
    std::cout << "Getting[] " << ROW_COUNT * COL_COUNT << " values took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        for(auto row = 0; row < ROW_COUNT; ++row)
        {
            //auto const set = df.SetValue(std::format("col{}", col), row, COL_COUNT - col);
            auto const set = df.SetValue(col, row, 42);
            ASSERT_TRUE(set);
        }
    }
    end = high_resolution_clock::now();
    std::cout << "Setting " << ROW_COUNT * COL_COUNT << " values took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto const rec = df.GetRecord(row);
        ASSERT_TRUE(rec);
        ASSERT_TRUE(rec->size() == COL_COUNT);
    }
    end = high_resolution_clock::now();
    std::cout << "Getting " << ROW_COUNT << " records took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        //auto const fld = df.GetField(std::format("col{}", col));
        auto const fld = df.GetField(col);
        ASSERT_TRUE(fld);

        auto count = 0;
        for(auto const& fldValue: *fld)
        {
            //std::cout << fldValue << ' ';
            count++;
        }
        ASSERT_TRUE(count == ROW_COUNT);
        //std::cout << std::endl;
    }
    end = high_resolution_clock::now();
    std::cout << "Getting " << COL_COUNT << " fields took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

   //std::ofstream ofs("/tmp/df_print.csv");
   //df.PrintCSV(ofs);
}
