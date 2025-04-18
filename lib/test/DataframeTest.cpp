// Filename: DataFrameTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

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
#include <compare>
#include <functional>

#include "lugizmo/memory/References.h"
#include "lugizmo/dataframe/Selector.h"
#include "lugizmo/dataframe/View.h"
#include "lugizmo/dataframe/ViewIndexed.h"
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

TEST(lugizmo_dataframe_test, row_major_set_get_records)
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

TEST(lugizmo_dataframe_test, row_major_set_get_field_views)
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
        auto view = df.ViewField(col);
        ASSERT_FALSE(view.Empty());

        for(auto const& val : view)
            ASSERT_EQ(val, ARRAY[col]);

        for(auto& val : view)
            val = 42;
    }

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col, row), 42);
}

TEST(lugizmo_dataframe_test, has_field)
{
    using namespace lugizmo;

    {
        using DF = DataFrame<int, int, int>;
        auto df = DF();

        df.AddField(2);
        df.AddField(3);

        ASSERT_TRUE(df.HasField(2));
        ASSERT_TRUE(df.HasField(3));
        ASSERT_FALSE(df.HasField(4));
    }

    {
        using DF = DataFrame<int, DFRangeIndex<int>, int>;
        auto df = DF();

        df.SetFieldRange(1, 2);
        df.AddRecord(1);

        ASSERT_FALSE(df.HasField(0));
        ASSERT_TRUE(df.HasField(1));
        ASSERT_FALSE(df.HasField(2));
    }

    {
        using DF = DataFrame<int, DFRangeIndex<int>, int>;
        auto df = DF();

        df.SetFieldRange(1, 3);
        df.AddRecord(1);

        ASSERT_FALSE(df.HasField(0.f));
        ASSERT_TRUE(df.HasField(1.f));
        ASSERT_TRUE(df.HasField(static_cast<unsigned>(2)));
        ASSERT_FALSE(df.HasField(3.0));

        using DF2 = DataFrame<int, std::string, int>;
        auto df2 = DF2();

        df2.AddFields(std::array<std::string, 3>{"a", "b", "c"});
        df2.AddRecord(1);

        ASSERT_FALSE(df2.HasField(std::string("0")));
        ASSERT_TRUE(df2.HasField(std::string("a")));
        ASSERT_TRUE(df2.HasField(static_cast<const char*>("b")));
        ASSERT_TRUE(df2.HasField(std::string_view("c")));
    }
}

TEST(lugizmo_dataframe_test, has_record)
{
    using namespace lugizmo;

    {
        using DF = DataFrame<int, int, int>;
        auto df = DF();

        df.AddField(2);
        df.AddField(3);
        df.AddRecord(1);
        df.AddRecord(2);

        ASSERT_TRUE(df.HasRecord(1));
        ASSERT_TRUE(df.HasRecord(2));
        ASSERT_FALSE(df.HasRecord(3));
    }

    {
        using DF = DataFrame<int, int, DFRangeIndex<int>>;
        auto df = DF();

        df.AddField(2);
        df.AddField(3);
        df.SetRecordRange(1, 2);

        ASSERT_FALSE(df.HasRecord(0));
        ASSERT_TRUE(df.HasRecord(1));
        ASSERT_FALSE(df.HasRecord(2));
    }

    {
        using DF = DataFrame<int, int, DFRangeIndex<int>>;
        auto df = DF();

        df.AddField(1);
        df.SetRecordRange(1, 3);

        ASSERT_FALSE(df.HasRecord(0.f));
        ASSERT_TRUE(df.HasRecord(1.f));
        ASSERT_TRUE(df.HasRecord(static_cast<unsigned>(2)));
        ASSERT_FALSE(df.HasRecord(3.0));

        using DF2 = DataFrame<int, int, std::string>;
        auto df2 = DF2();

        df2.AddFields(std::array{1, 2, 3});
        df2.AddRecord("a");
        df2.AddRecord("b");
        df2.AddRecord("c");

        ASSERT_FALSE(df2.HasRecord(std::string("0")));
        ASSERT_TRUE(df2.HasRecord(std::string("a")));
        ASSERT_TRUE(df2.HasRecord(static_cast<const char*>("b")));
        ASSERT_TRUE(df2.HasRecord(std::string_view("c")));
    }
}

TEST(lugizmo_dataframe_test, row_major_set_get_record_views)
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
        auto view = df.ViewRecord(row);
        ASSERT_FALSE(view.Empty());

        for(auto col = 0; col < COL_COUNT; ++col)
            ASSERT_EQ(view[col], ARRAY[col]);

        for(auto& val : view)
            val = 42;
    }

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col, row), 42);
}

TEST(lugizmo_dataframe_test, row_major_set_get_field_range_views)
{
    using namespace lugizmo;
    using DF = DataFrame<int, std::string, DFRangeIndex<int>>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;
    constexpr int ROW_BEGIN = ROW_COUNT / 2 * -1;
    constexpr int ROW_END   = ROW_COUNT / 2;
    constexpr auto ARRAY        = std::array{1, 2, 3, 4, 5};

    static_assert(ARRAY.size() == COL_COUNT);
    static_assert(ROW_COUNT % 2 == 0);

    auto df = DF();

    for(auto col = 0; col < COL_COUNT; ++col)
        ASSERT_TRUE(df.AddField(std::to_string(col)));

    ASSERT_TRUE(df.SetRecordRange(ROW_BEGIN, ROW_END, ARRAY));

    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto view = df.ViewField(std::to_string(col));
        ASSERT_FALSE(view.Empty());

        // transpose
        auto count = 0;
        for(auto& val : view) { val = count++; }

        count = 0;
        for(auto row = ROW_BEGIN; row < ROW_END; ++row)
        {
            auto const valRef = view.At(row);
            ASSERT_TRUE(valRef);
            ASSERT_EQ(*valRef, count);
            ++count;
        }
    }

    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto count = 0;
        for(auto row = ROW_BEGIN; row < ROW_END; ++row)
            ASSERT_EQ(df.GetValue(std::to_string(col), row), count++);
    }
}

TEST(lugizmo_dataframe_test, row_major_set_get_record_range_views)
{
    using namespace lugizmo;
    using DF = DataFrame<int, DFRangeIndex<int>, std::string>;

    constexpr int ROW_COUNT = 10;
    constexpr int COL_BEGIN = -2;
    constexpr int COL_END   = 3;

    DF df;

    ASSERT_TRUE(df.SetFieldRange(COL_BEGIN, COL_END, 1));
    for (int row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecord(std::to_string(row)));

    for(int row = 0; row < ROW_COUNT; ++row)
    {
        auto view = df.ViewRecord(std::to_string(row));
        ASSERT_FALSE(view.Empty());

        int count = 0;
        for (auto& val : view)
        {
            val = count++;
        }

        count = 0;
        for (int col = COL_BEGIN; col < COL_END; ++col)
        {
            auto const valOpt = view.At(col);
            ASSERT_TRUE(valOpt);
            ASSERT_EQ(*valOpt, count);
            ++count;
        }
    }

    for (int row = 0; row < ROW_COUNT; ++row)
    {
        int count = 0;
        for (int col = COL_BEGIN; col < COL_END; ++col)
        {
            ASSERT_EQ(df.GetValue(col, std::to_string(row)), count);
            ++count;
        }
    }
}


TEST(lugizmo_dataframe_test, row_major_indexed_views)
{
    using namespace lugizmo;
    using DF         = DataFrame<int, int, int>;
    using DFOptional = DataFrame<std::optional<int>, int, int>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;
    constexpr auto ARRAY    = std::array{1, 2, 3, 4, 5};
    constexpr auto ARRAY_OP = std::array<std::optional<int>, COL_COUNT>{1, 2, std::optional<int>(), 4, 5};
    static_assert(ARRAY.size() == COL_COUNT);

    auto df   = DF();
    auto dfOp = DFOptional();

    for(auto col = 0; col < COL_COUNT; ++col)
    {
        ASSERT_TRUE(df.AddField(col));
        ASSERT_TRUE(dfOp.AddField(col));
    }

    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));
        ASSERT_TRUE(dfOp.AddRecordPopulated(row, ARRAY_OP));
    }

    // check with view.Get() field values
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto view = df.ViewFieldIndexed(col);
        ASSERT_FALSE(view.Empty());
        ASSERT_TRUE(view.Size() == ROW_COUNT);

        static_assert(std::size(ARRAY) == COL_COUNT);
        auto columnValue = ARRAY[col];

        for(auto row = 0; row < ROW_COUNT; ++row)
        {
            ASSERT_TRUE(view.TryAt(row));
            ASSERT_EQ(view.TryAt(row), columnValue);
        }
    }

    // check with view.GetUnwrappedOptional() field values
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto view = dfOp.ViewFieldIndexed(col);
        ASSERT_FALSE(view.Empty());
        ASSERT_TRUE(view.Size() == ROW_COUNT);

        static_assert(std::size(ARRAY_OP) == COL_COUNT);
        auto columnValue = ARRAY_OP[col];

        for(auto row = 0; row < ROW_COUNT; ++row)
        {
            if(col != 2)
            {
                ASSERT_TRUE(view.Unwrap(row));
                ASSERT_EQ(*view.Unwrap(row), columnValue);
            }
            else
            {
                ASSERT_FALSE(view.Unwrap(row));
            }
        }
    }

    // check with view.Get() record values
    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto view = df.ViewRecordIndexed(row);
        ASSERT_FALSE(view.Empty());
        ASSERT_TRUE(view.Size() == COL_COUNT);

        for(auto col = 0; col < COL_COUNT; ++col)
        {
            static_assert(std::size(ARRAY) == COL_COUNT);
            auto columnValue = ARRAY[col];

            ASSERT_TRUE(view.At(col));
            ASSERT_EQ(*view.At(col), columnValue);
        }
    }

    // check with view.GetUnwrappedOptional() record values
    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto view = dfOp.ViewRecordIndexed(row);
        ASSERT_FALSE(view.Empty());
        ASSERT_TRUE(view.Size() == COL_COUNT);

        for(auto col = 0; col < COL_COUNT; ++col)
        {
            static_assert(std::size(ARRAY_OP) == COL_COUNT);
            auto columnValue = ARRAY_OP[col];

            if(col != 2)
            {
                ASSERT_TRUE(view.Unwrap(col));
                ASSERT_EQ(*view.Unwrap(col), columnValue);
            }
            else
            {
                ASSERT_FALSE(view.Unwrap(col));
            }
        }
    }

    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto view = df.ViewFieldIndexed(col);
        ASSERT_FALSE(view.Empty());

        auto currentRow = 0;
        for(auto& [val, idx] : view)
        {
            ASSERT_EQ(idx, currentRow);
            ASSERT_EQ(val, ARRAY[col]);

            val = 42;
            currentRow++;
        }
    }

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col, row), 42);

    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto view = df.ViewRecordIndexed(row);
        ASSERT_FALSE(view.Empty());

        auto currentRow = 0;
        for(auto& [val, idx] : view)
        {
            ASSERT_EQ(idx, currentRow);
            ASSERT_EQ(val, 42);

            val = 43;
            currentRow++;
        }
    }

    for(auto col = 0; col < COL_COUNT; ++col)
        for(auto row = 0; row < ROW_COUNT; ++row)
            ASSERT_EQ(df.GetValue(col, row), 43);
}

TEST(lugizmo_dataframe_test, row_major_for_each)
{
    using namespace lugizmo;
    using DF = DataFrame<int, int, int>;

    constexpr int COL_COUNT = 5;
    constexpr int ROW_COUNT = 10;

    constexpr auto ARRAY = std::array{1, 2, 3, 4, 5};
    constexpr auto ONES  = std::array{1, 1, 1, 1, 1};
    static_assert(ARRAY.size() == COL_COUNT);
    static_assert(ONES.size() == ARRAY.size());

    // std::algorithm
    {
        auto df = DF();
        for(auto col = 0; col < COL_COUNT; ++col) ASSERT_TRUE(df.AddField(col));
        for(auto row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));

        auto field = df.ViewField(0);
        ASSERT_FALSE(field.Empty());

        std::ranges::for_each(field, [](auto& val) { val += 1; });
        ASSERT_TRUE(std::ranges::all_of(field, [](auto& val) { return val == 2; }));

        auto record = df.ViewRecord(0);
        ASSERT_FALSE(record.Empty());

        std::ranges::for_each(record, [](auto& val) { val += 1; });

        auto count = 0;
        ASSERT_TRUE(std::ranges::all_of(record, [&](auto& val) {
            if(count == 0) {
                count++;
                return val == 3;
            }

            auto const arr = ARRAY[count];
            count++;
            return val == (arr + 1);
        }));
    }

    // DataFrame::ForEach*
    {
        auto df = DF();
        for(auto col = 0; col < COL_COUNT; ++col) ASSERT_TRUE(df.AddField(col));
        for(auto row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecordPopulated(row, ARRAY));

        // field non-const
        auto view = df.ForEachOnField(1, [](auto& val) { val += 2; });
        ASSERT_TRUE(view.Size() != 0);

        std::ranges::for_each(view, [](int& val) { val += 2; });
        ASSERT_TRUE(std::ranges::all_of(view, [](auto& val) { return val == 6; }));

        // TODO record non-const
    }

    {
        auto df = DF();
        for(auto col = 0; col < COL_COUNT; ++col) ASSERT_TRUE(df.AddField(col));
        for(auto row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecordPopulated(row, ONES));

        auto const& cdf = df;

        // field const
        auto view1 = cdf.ForEachOnField(2, [](auto const& val) { ASSERT_EQ(val, 1); });
        ASSERT_TRUE(view1.Size() != 0);

        // record const
        auto view2 = cdf.ForEachOnRecord(2, [](auto const& val) { ASSERT_EQ(val, 1); });
        ASSERT_TRUE(view2.Size() != 0);
    }

    {
        auto df = DF();
        for(auto col = 0; col < COL_COUNT; ++col) ASSERT_TRUE(df.AddField(col));
        for(auto row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecordPopulated(row, ONES));

        auto const& cdf = df;

        // field const
        auto view1 = cdf | SelectField(2) | std::views::filter([](auto r){ return r % 2 == 0; })
                                          | std::views::transform([](auto r) { return r; });

        ASSERT_TRUE(std::ranges::all_of(view1, [](auto const r) { return r % 2 == 0; }));

        // record const
        auto view2 = cdf | SelectRecord(2) | std::views::filter([](auto r){ return r % 2 == 0; })
                                           | std::views::transform([](auto r) { return r; });

        ASSERT_TRUE(std::ranges::all_of(view2, [](auto const r) { return r % 2 == 0; }));

        // field const
        auto view3 = cdf.ViewField(2) | std::views::filter([](auto r){ return r % 2 == 0; })
                                      | std::views::transform([](auto r) { return r; });

        ASSERT_TRUE(std::ranges::all_of(view3, [](auto const r) { return r % 2 == 0; }));

        // record const
        auto view4 = cdf.ViewRecord(2) | std::views::filter([](auto r){ return r % 2 == 0; })
                                       | std::views::transform([](auto r) { return r; });

        ASSERT_TRUE(std::ranges::all_of(view4, [](auto const r) { return r % 2 == 0; }));
    }

    {
        auto df = DF();
        for(auto col = 0; col < COL_COUNT; ++col) ASSERT_TRUE(df.AddField(col));
        for(auto row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecordPopulated(row, ONES));

        // field const
        auto view1 = df | SelectFieldIndexed(2) | std::views::filter([](auto rec) { return rec.val % 2 == 0; })
                                                | std::views::transform([](auto rec) { return rec.val; });

        ASSERT_TRUE(std::ranges::all_of(view1, [](auto const r) { return r % 2 == 0; }));

        // record const
        auto view2 = df | SelectRecordIndexed(2) | std::views::filter([](auto field){ return field.val % 2 == 0; })
                                                 | std::views::transform([](auto field) { return field.val; });

        ASSERT_TRUE(std::ranges::all_of(view2, [](auto const r) { return r % 2 == 0; }));

        // field const
        auto view3 = df.ViewFieldIndexed(2) | std::views::filter([](auto rec){ return rec.val % 2 == 0; })
                                            | std::views::transform([](auto rec) { return rec.val; });

        ASSERT_TRUE(std::ranges::all_of(view3, [](auto const r) { return r % 2 == 0; }));

        // record const
        auto view4 = df.ViewRecordIndexed(2) | std::views::filter([](auto field){ return field.val % 2 == 0; })
                                             | std::views::transform([](auto field) { return field.val; });

        ASSERT_TRUE(std::ranges::all_of(view4, [](auto const r) { return r % 2 == 0; }));
    }

    {
        auto df = DF();
        for(auto col = 0; col < COL_COUNT; ++col) ASSERT_TRUE(df.AddField(col));
        for(auto row = 0; row < ROW_COUNT; ++row) ASSERT_TRUE(df.AddRecordPopulated(row, ONES));

        auto const& cdf = df;

        // field const
        auto view1 = cdf | SelectFieldIndexed(2) | std::views::filter([](auto const rec) { return rec.val % 2 == 0; })
                                                 | std::views::transform([](auto const rec) { return rec.val; });

        ASSERT_TRUE(std::ranges::all_of(view1, [](auto const r) { return r % 2 == 0; }));

        // record const
        auto view2 = cdf | SelectRecordIndexed(2) | std::views::filter([](auto const field){ return field.val % 2 == 0; })
                                                  | std::views::transform([](auto const field) { return field.val; });

        ASSERT_TRUE(std::ranges::all_of(view2, [](auto const r) { return r % 2 == 0; }));

        // field const
        auto view3 = cdf.ViewFieldIndexed(2) | std::views::filter([](auto const rec){ return rec.val % 2 == 0; })
                                             | std::views::transform([](auto const rec) { return rec.val; });

        ASSERT_TRUE(std::ranges::all_of(view3, [](auto const r) { return r % 2 == 0; }));

        // record const
        auto view4 = cdf.ViewRecordIndexed(2) | std::views::filter([](auto const field){ return field.val % 2 == 0; })
                                              | std::views::transform([](auto const field) { return field.val; });

        ASSERT_TRUE(std::ranges::all_of(view4, [](auto const r) { return r % 2 == 0; }));
    }
}

TEST(lugizmo_dataframe_test, row_major_drop)
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

        auto const get = df.ViewField(fld);
        ASSERT_FALSE(get.Empty());
        ASSERT_TRUE(std::ranges::all_of(get, [value](auto const v) { return v == value; }));

        value += 2;
    }

    // drop rows/records
    for(auto row = 0; row < ROW_COUNT; ++row)
        if(row % 2 == 0) ASSERT_TRUE(df.DropRecord(row));

    ASSERT_EQ(df.Records().size(), ROW_COUNT/2);

    for(auto const rec: df.Records())
    {
        ASSERT_TRUE(rec % 2 != 0);

        auto const get = df.ViewRecord(rec);
        ASSERT_FALSE(get.Empty());

        auto recValue = 0;
        ASSERT_TRUE(std::ranges::all_of(get, [&recValue](auto const v) { recValue += 2; return v == recValue; }));
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

TEST(lugizmo_dataframe_test, dev)
{
    using namespace lugizmo;
    using namespace std::chrono;
    using ms = milliseconds;

    constexpr int COL_COUNT = 100;
    constexpr int ROW_COUNT = 100;

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

            auto const con = get >= 0;
            ASSERT_TRUE(con);
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
            auto const con = get >= 0;
            ASSERT_TRUE(con);
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
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        auto colView = df.ViewField(col);
        ASSERT_FALSE(colView.Empty());
        std::ranges::for_each(colView, [col](auto& v){ v = col;});
    }
    end = high_resolution_clock::now();
    std::cout << "Setting columns with view of: " << COL_COUNT << " columns with " << ROW_COUNT << " rows took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto rowView = df.ViewRecord(row);
        ASSERT_FALSE(rowView.Empty());
        std::ranges::for_each(rowView, [row](auto& v){ v = row;});
    }
    end = high_resolution_clock::now();
    std::cout << "Setting rows with view of: " << ROW_COUNT << " rows with " << COL_COUNT << " cols took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto row = 0; row < ROW_COUNT; ++row)
    {
        auto const rec = df.ViewRecord(row);
        ASSERT_TRUE(rec.Size() == COL_COUNT);
    }
    end = high_resolution_clock::now();
    std::cout << "Getting " << ROW_COUNT << " records took: " << duration_cast<ms>(end - start).count() << "ms" << std::endl;

    start = high_resolution_clock::now();
    for(auto col = 0; col < COL_COUNT; ++col)
    {
        //auto const fld = df.GetField(std::format("col{}", col));
        auto const fld = df.ViewField(col);
        ASSERT_FALSE(fld.Empty());

        auto count = 0;
        for(auto const& fldValue: fld)
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
