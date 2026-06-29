// Filename: DFViewTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the DFView lightweight view over a single field / record.
// The following functions are tested here (with names of tests):
//
// ✅ Construct                 - DFView() / Empty
// ✅ IndexOperatorOptionalRef  - operator()(key) -> OptionalRef / At
// ✅ FieldViewSubrange         - FieldView(..., begin, end) key translation
// ✅ RecordViewSubrange        - RecordView(..., begin, end) key translation
//

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <mdspan>
#include <string>

#include "lugizmo/DataFrame.h"
#include "lugizmo/dataframe/IndexRange.h"
#include "lugizmo/dataframe/IndexUnique.h"
#include "lugizmo/dataframe/View.h"

TEST(DataframeView, Construct)
{
    using namespace lugizmo;

    auto const empty = DFView<int, DFUniqueIndex<int>>();
    ASSERT_TRUE(empty.Empty());
}

TEST(DataframeView, IndexOperatorOptionalRef)
{
    using namespace lugizmo;

    auto empty = DFView<int, DFUniqueIndex<int>>();
    auto miss  = empty(0);
    ASSERT_FALSE(miss.HasValue());

    auto df   = DataFrame<int, int, int>::FromFieldsAndRecords({0}, {0}, {{7}});
    auto view = df.ViewField(0);
    auto hit  = view(0);

    ASSERT_TRUE(hit.HasValue());
    ASSERT_EQ(hit.Value(), 7);

    hit.Value() = 9;
    ASSERT_EQ(*view.At(0), 9);
}

TEST(DataframeView, FieldViewSubrange)
{
    using namespace lugizmo;

    auto values = std::array{0, 1,
                             2, 3,
                             4, 5,
                             6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 4, 2);

    auto const records = DFRangeIndex(10, 18, 2); // 10, 12, 14, 16
    auto view = DFView<int, DFRangeIndex<int>>::FieldView(matrix, &records, 1, 1, 3);

    ASSERT_EQ(view.Size(), 2);
    EXPECT_EQ(view[0], 3);
    EXPECT_EQ(view[1], 5);

    EXPECT_FALSE(view.Contains(10));
    EXPECT_TRUE(view.Contains(12));
    EXPECT_TRUE(view.Contains(14));
    EXPECT_FALSE(view.Contains(16));

    ASSERT_NE(view.At(12), nullptr);
    ASSERT_NE(view.At(14), nullptr);
    EXPECT_EQ(*view.At(12), 3);
    EXPECT_EQ(*view.At(14), 5);
    EXPECT_EQ(view.At(10), nullptr);
    EXPECT_EQ(view.At(16), nullptr);
}

TEST(DataframeView, RecordViewSubrange)
{
    using namespace lugizmo;

    auto values = std::array{0, 1, 2, 3,
                             4, 5, 6, 7};
    using Matrix = std::mdspan<int, std::dextents<std::ptrdiff_t, 2>, std::layout_right>;
    auto matrix = Matrix(values.data(), 2, 4);

    auto fields = DFUniqueIndex<int>();
    fields.AddMultiple(std::array{10, 20, 30, 40});
    auto view = DFView<int, DFUniqueIndex<int>>::RecordView(matrix, &fields, 1, 1, 3);

    ASSERT_EQ(view.Size(), 2);
    EXPECT_EQ(view[0], 5);
    EXPECT_EQ(view[1], 6);

    EXPECT_FALSE(view.Contains(10));
    EXPECT_TRUE(view.Contains(20));
    EXPECT_TRUE(view.Contains(30));
    EXPECT_FALSE(view.Contains(40));

    ASSERT_NE(view.At(20), nullptr);
    ASSERT_NE(view.At(30), nullptr);
    EXPECT_EQ(*view.At(20), 5);
    EXPECT_EQ(*view.At(30), 6);
    EXPECT_EQ(view.At(10), nullptr);
    EXPECT_EQ(view.At(40), nullptr);
}
