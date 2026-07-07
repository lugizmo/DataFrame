// Filename: SliceTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the regular two-dimensional DFSlice DataFrame view.
//
// ✅ DefaultConstruction - empty default state
// ✅ ShapeAndStride      - extents, total size, and physical strides
// ✅ FieldConsecutive    - consecutive versus gathered field positions
// ✅ RecordConsecutive   - consecutive versus gathered record positions
// ✅ Consecutive         - combined axis property
// ✅ Contiguity          - static guarantee and runtime instance query
// ✅ PositionalAccess    - flat storage-order and checked two-axis access
// ✅ Subscript2D         - asserted two-axis C++23 subscript access
// ✅ SliceComposition    - compose gathered and regular parent mappings
// ✅ SliceFields         - compose fields while preserving parent records
// ✅ SliceRecords        - compose records while preserving parent fields
// ✅ SliceField          - preserve records while selecting one field
// ✅ SliceRecord         - preserve fields while selecting one record
// ✅ KeyAccess           - translated field/record lookup
// ✅ LayoutOrder         - row-major and column-major iteration order
// ✅ Constness           - mutable and read-only element access
// ✅ RangeAdaptor        - standard range adaptor composition
// ✅ Allocator           - owned position mappings preserve their memory resource
//

#include "gtest/gtest.h"

#include <array>
#include <mdspan>
#include <memory_resource>
#include <ranges>
#include <type_traits>

#include "lugizmo/dataframe/view/Slice.h"

namespace {

    using Index = lgz::DFUniqueIndex<int>;

    template<typename Layout, bool Contiguous = false>
    using Slice = lgz::DFSlice<int, Index, Index, Layout, Contiguous>;

    template<typename Layout, bool Contiguous = false>
    using ConstSlice = lgz::DFSlice<int const, Index, Index, Layout, Contiguous>;

    template<typename Layout>
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, Layout>;

    template<typename Layout>
    using ConstMatrix = std::mdspan<int const, std::dextents<std::ptrdiff_t, 2>, Layout>;

    class CountingResource final : public std::pmr::memory_resource
    {
    public:
        [[nodiscard]] auto Allocations() const noexcept -> std::size_t { return allocations; }

    private:
        auto do_allocate(std::size_t const bytes, std::size_t const alignment) -> void* override
        {
            ++allocations;
            return std::pmr::new_delete_resource()->allocate(bytes, alignment);
        }

        void do_deallocate(void* const pointer, std::size_t const bytes, std::size_t const alignment) override
        {
            std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
        }

        [[nodiscard]] auto do_is_equal(memory_resource const& other) const noexcept -> bool override { return this == &other; }

        std::size_t allocations = 0;
    };

    auto Fields() -> Index
    {
        auto fields = Index();
        fields.AddMultiple(std::array{10, 20, 30, 40});
        return fields;
    }

    auto Records() -> Index
    {
        auto records = Index();
        records.AddMultiple(std::array{100, 200, 300});
        return records;
    }

} // namespace

/**
 * @brief A default DFSlice is an empty random-access range.
 * @see   lgz::DFSlice::DFSlice
 */
TEST(DataframeSlice, DefaultConstruction)
{
    auto const slice = Slice<std::layout_right>();
    EXPECT_TRUE(slice.Empty());
    EXPECT_EQ(slice.Size(), 0);
    EXPECT_EQ(slice.begin(), slice.end());
}

/**
 * @brief Shape and stride queries preserve the selected rectangle and source mapping.
 * @see   lgz::DFSlice::FieldSize
 * @see   lgz::DFSlice::RecordSize
 * @see   lgz::DFSlice::FieldStride
 * @see   lgz::DFSlice::RecordStride
 */
TEST(DataframeSlice, ShapeAndStride)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto slice   = Slice<std::layout_right>::View(Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, 1, 3, 0, 3);

    EXPECT_EQ(slice.FieldSize(), 2);
    EXPECT_EQ(slice.RecordSize(), 3);
    EXPECT_EQ(slice.Size(), 6);
    EXPECT_FALSE(slice.Empty());
    EXPECT_EQ(slice.FieldStride(), 1);
    EXPECT_EQ(slice.RecordStride(), 4);
}

/**
 * @brief IsFieldConsecutive reports whether the selected field positions contain no gaps.
 * @see   lgz::DFSlice::IsFieldConsecutive
 */
TEST(DataframeSlice, FieldConsecutive)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7};
    auto fields  = Fields();
    auto records = Records();
    auto matrix  = Matrix<std::layout_right>(values.data(), 2, 4);

    auto consecutive = Slice<std::layout_right>::View(matrix, &fields, &records, 1, 4, 0, 2);
    auto gathered    = Slice<std::layout_right>::SelectedFields(matrix, &fields, &records, {0, 2});
    auto strided     = Slice<std::layout_right>::Mapped(matrix, &fields, &records, 0, 2, 2, 0, 2, 1);

    EXPECT_TRUE(consecutive.IsFieldConsecutive());
    EXPECT_FALSE(gathered.IsFieldConsecutive());
    EXPECT_FALSE(strided.IsFieldConsecutive());
}

/**
 * @brief IsRecordConsecutive reports whether the selected record positions contain no gaps.
 * @see   lgz::DFSlice::IsRecordConsecutive
 */
TEST(DataframeSlice, RecordConsecutive)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto matrix  = Matrix<std::layout_right>(values.data(), 3, 4);

    auto consecutive = Slice<std::layout_right>::View(matrix, &fields, &records, 0, 4, 0, 2);
    auto gathered    = Slice<std::layout_right>::SelectedRecords(matrix, &fields, &records, {0, 2});
    auto strided     = Slice<std::layout_right>::Mapped(matrix, &fields, &records, 0, 4, 1, 0, 2, 2);

    EXPECT_TRUE(consecutive.IsRecordConsecutive());
    EXPECT_FALSE(gathered.IsRecordConsecutive());
    EXPECT_FALSE(strided.IsRecordConsecutive());
}

/**
 * @brief IsConsecutive requires both field and record selections to contain no gaps.
 * @see   lgz::DFSlice::IsConsecutive
 */
TEST(DataframeSlice, Consecutive)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto matrix  = Matrix<std::layout_right>(values.data(), 3, 4);

    auto consecutive = Slice<std::layout_right>::Selected(matrix, &fields, &records, {1, 2}, {0, 1});
    auto recordGap   = Slice<std::layout_right>::Selected(matrix, &fields, &records, {1, 2}, {0, 2});

    EXPECT_TRUE(consecutive.IsConsecutive());
    EXPECT_TRUE(recordGap.IsFieldConsecutive());
    EXPECT_FALSE(recordGap.IsRecordConsecutive());
    EXPECT_FALSE(recordGap.IsConsecutive());
}

/**
 * @brief Contiguous is a type guarantee while IsContiguous also describes a particular instance.
 * @see   lgz::DFSlice::Contiguous
 * @see   lgz::DFSlice::IsContiguous
 */
TEST(DataframeSlice, Contiguity)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7};
    auto fields  = Fields();
    auto records = Records();
    auto matrix  = Matrix<std::layout_right>(values.data(), 2, 4);

    auto full = Slice<std::layout_right, true>::View(matrix, &fields, &records, 0, 4, 0, 2);
    static_assert(decltype(full)::Contiguous);
    static_assert(std::ranges::contiguous_range<decltype(full)>);
    EXPECT_TRUE(full.IsContiguous());

    auto oneRow = Slice<std::layout_right>::View(matrix, &fields, &records, 1, 4, 1, 2);
    static_assert(!decltype(oneRow)::Contiguous);
    static_assert(!std::ranges::contiguous_range<decltype(oneRow)>);
    EXPECT_TRUE(oneRow.IsContiguous());

    auto rectangle = Slice<std::layout_right>::View(matrix, &fields, &records, 1, 3, 0, 2);
    EXPECT_FALSE(rectangle.IsContiguous());
}

/**
 * @brief Flat access follows storage order while operator() uses (record, field) positions.
 * @see   lgz::DFSlice::operator[]
 * @see   lgz::DFSlice::operator()
 */
TEST(DataframeSlice, PositionalAccess)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto slice   = Slice<std::layout_right>::View(Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, 1, 3, 0, 3);

    EXPECT_EQ(slice[0], 1);
    EXPECT_EQ(slice[2], 5);
    ASSERT_NE(slice(2, 1), nullptr);
    EXPECT_EQ(*slice(2, 1), 10);
    EXPECT_EQ(slice(3, 0), nullptr);
    EXPECT_EQ(slice(0, 2), nullptr);
}

/**
 * @brief The C++23 two-argument subscript accesses an asserted `(record, field)` position.
 * @see   lgz::DFSlice::operator[]
 */
TEST(DataframeSlice, Subscript2D)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto slice   = Slice<std::layout_right>::View(Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, 1, 3, 0, 3);

    EXPECT_EQ((slice[0, 0]), 1);
    EXPECT_EQ((slice[2, 1]), 10);
    slice[1, 0] = 42;
    EXPECT_EQ(values[5], 42);
}

/**
 * @brief Slice composes key selections with both gathered and regular parent mappings.
 * @see   lgz::DFSlice::Slice
 */
TEST(DataframeSlice, SliceComposition)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto matrix  = Matrix<std::layout_right>(values.data(), 3, 4);

    auto gathered = Slice<std::layout_right>::Selected(matrix, &fields, &records, {0, 2, 3}, {0, 2});
    auto nestedGathered = gathered.Slice({40, 10}, {300});
    EXPECT_TRUE(std::ranges::equal(nestedGathered, std::array{8, 11}));
    EXPECT_TRUE(nestedGathered.Contains(40, 300));
    EXPECT_FALSE(nestedGathered.Contains(30, 300));

    auto regular = Slice<std::layout_right>::Mapped(matrix, &fields, &records, 0, 2, 2, 0, 3, 1);
    auto nestedRegular = regular.Slice(std::array{30}, std::array{300, 200});
    EXPECT_TRUE(std::ranges::equal(nestedRegular, std::array{6, 10}));
    EXPECT_TRUE(nestedRegular.Contains(30, 200));

    using RangeIndex = lgz::DFRangeIndex<int>;
    using RangeSlice = lgz::DFSlice<int, RangeIndex, RangeIndex, std::layout_right>;
    auto rangeFields  = RangeIndex(0, 4);
    auto rangeRecords = RangeIndex(0, 3);
    auto rangeParent  = RangeSlice::View(matrix, &rangeFields, &rangeRecords, 0, 4, 0, 3);
    auto rangeNested  = rangeParent.Slice(lgz::DFRangeIndexBounds{.lower = 1, .upper = 4, .step = 2},
                                          lgz::DFRangeIndexBounds{.lower = 0, .upper = 3, .step = 2});
    EXPECT_TRUE(std::ranges::equal(rangeNested, std::array{1, 3, 9, 11}));
}

/**
 * @brief SliceFields composes a field selection while preserving the parent records.
 * @see   lgz::DFSlice::SliceFields
 */
TEST(DataframeSlice, SliceFields)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto parent  = Slice<std::layout_right>::Selected(
            Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, {0, 2, 3}, {0, 2});

    auto selected = parent.SliceFields({40, 10});
    EXPECT_EQ(selected.FieldSize(), 2);
    EXPECT_EQ(selected.RecordSize(), 2);
    EXPECT_TRUE(std::ranges::equal(selected, std::array{0, 3, 8, 11}));
    EXPECT_TRUE(selected.Contains(40, 300));
    EXPECT_TRUE(parent.SliceFields({20}).Empty());
}

/**
 * @brief SliceRecords composes a record selection while preserving the parent fields.
 * @see   lgz::DFSlice::SliceRecords
 */
TEST(DataframeSlice, SliceRecords)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto parent  = Slice<std::layout_right>::Selected(
            Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, {0, 2, 3}, {0, 2});

    auto selected = parent.SliceRecords({300, 100});
    EXPECT_EQ(selected.FieldSize(), 3);
    EXPECT_EQ(selected.RecordSize(), 2);
    EXPECT_TRUE(std::ranges::equal(selected, std::array{0, 2, 3, 8, 10, 11}));
    EXPECT_TRUE(selected.Contains(40, 300));
    EXPECT_TRUE(parent.SliceRecords({200}).Empty());
}

/**
 * @brief SliceField preserves the parent record selection and selects one field key.
 * @see   lgz::DFSlice::SliceField
 */
TEST(DataframeSlice, SliceField)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto parent  = Slice<std::layout_right>::Selected(Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, {0, 2, 3}, {0, 2});

    auto field = parent.SliceField(30);
    EXPECT_EQ(field.FieldSize(), 1);
    EXPECT_EQ(field.RecordSize(), 2);
    EXPECT_TRUE(std::ranges::equal(field, std::array{2, 10}));
    EXPECT_TRUE(field.Contains(30, 300));
    EXPECT_TRUE(parent.SliceField(20).Empty());
}

/**
 * @brief SliceRecord preserves the parent field selection and selects one record key.
 * @see   lgz::DFSlice::SliceRecord
 */
TEST(DataframeSlice, SliceRecord)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto parent  = Slice<std::layout_right>::Selected(Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, {0, 2, 3}, {0, 2});

    auto record = parent.SliceRecord(300);
    EXPECT_EQ(record.FieldSize(), 3);
    EXPECT_EQ(record.RecordSize(), 1);
    EXPECT_TRUE(std::ranges::equal(record, std::array{8, 10, 11}));
    EXPECT_TRUE(record.Contains(40, 300));
    EXPECT_TRUE(parent.SliceRecord(200).Empty());
}

/**
 * @brief Contains and At translate dataframe keys through both slice offsets.
 * @see   lgz::DFSlice::Contains
 * @see   lgz::DFSlice::At
 */
TEST(DataframeSlice, KeyAccess)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto fields  = Fields();
    auto records = Records();
    auto slice   = Slice<std::layout_right>::View(Matrix<std::layout_right>(values.data(), 3, 4), &fields, &records, 1, 3, 1, 3);

    EXPECT_TRUE(slice.Contains(20, 200));
    EXPECT_FALSE(slice.Contains(10, 200));
    EXPECT_FALSE(slice.Contains(20, 100));
    ASSERT_NE(slice.At(30, 300), nullptr);
    EXPECT_EQ(*slice.At(30, 300), 10);
    EXPECT_EQ(slice.At(40, 300), nullptr);
}

/**
 * @brief Flattened iteration follows the source layout's contiguous axis.
 * @see   lgz::DFSlice::begin
 * @see   lgz::DFSlice::end
 */
TEST(DataframeSlice, LayoutOrder)
{
    auto fields  = Fields();
    auto records = Records();

    auto rowValues = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto rowSlice  = Slice<std::layout_right>::View(Matrix<std::layout_right>(rowValues.data(), 3, 4), &fields, &records, 1, 3, 0, 3);
    EXPECT_TRUE(std::ranges::equal(rowSlice, std::array{1, 2, 5, 6, 9, 10}));

    auto colValues = std::array{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto colSlice  = Slice<std::layout_left>::View(Matrix<std::layout_left>(colValues.data(), 3, 4), &fields, &records, 1, 3, 1, 3);
    EXPECT_TRUE(std::ranges::equal(colSlice, std::array{4, 5, 7, 8}));
}

/**
 * @brief Element mutability comes from T rather than wrapper constness.
 * @see   lgz::DFSlice
 */
TEST(DataframeSlice, Constness)
{
    auto       values       = std::array{0, 1, 2, 3};
    auto       fields       = Fields();
    auto       records      = Records();
    auto const mutableSlice = Slice<std::layout_right, true>::View(Matrix<std::layout_right>(values.data(), 1, 4), &fields, &records, 0, 4, 0, 1);
    mutableSlice[1]         = 9;
    EXPECT_EQ(values[1], 9);

    auto const readOnlySlice = ConstSlice<std::layout_right, true>::View(ConstMatrix<std::layout_right>(values.data(), 1, 4), &fields, &records, 0, 4, 0, 1);
    static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(readOnlySlice)>, int const&>);
    EXPECT_EQ(readOnlySlice[1], 9);
}

/**
 * @brief DFSlice composes directly with standard range adaptor closures.
 * @see   lgz::DFSlice
 */
TEST(DataframeSlice, RangeAdaptor)
{
    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7};
    auto fields  = Fields();
    auto records = Records();
    auto slice   = Slice<std::layout_right>::View(Matrix<std::layout_right>(values.data(), 2, 4), &fields, &records, 1, 3, 0, 2);
    auto doubled = slice | std::views::transform([](int const value) { return value * 2; });

    EXPECT_TRUE(std::ranges::equal(doubled, std::array{2, 4, 10, 12}));
}

/**
 * @brief Gathered mappings and their copies retain the caller-provided memory resource.
 * @see   lgz::DFSlice::SelectedFields
 */
TEST(DataframeSlice, Allocator)
{
    auto resource = CountingResource();
    auto positions = std::pmr::vector<std::size_t>(&resource);
    positions.push_back(0);
    positions.push_back(2);

    auto values  = std::array{0, 1, 2, 3, 4, 5, 6, 7};
    auto fields  = Fields();
    auto records = Records();
    auto slice   = Slice<std::layout_right>::SelectedFields(
            Matrix<std::layout_right>(values.data(), 2, 4), &fields, &records, std::move(positions));

    auto allocations = resource.Allocations();
    auto copy         = slice;
    EXPECT_GT(resource.Allocations(), allocations);
    EXPECT_TRUE(std::ranges::equal(copy, std::array{0, 2, 4, 6}));

    allocations = resource.Allocations();
    auto assigned = Slice<std::layout_right>();
    assigned      = slice;
    EXPECT_GT(resource.Allocations(), allocations);
    EXPECT_TRUE(std::ranges::equal(assigned, std::array{0, 2, 4, 6}));
}
