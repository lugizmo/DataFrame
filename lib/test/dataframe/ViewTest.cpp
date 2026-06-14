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
//

#include "gtest/gtest.h"

#include <string>

#include "lugizmo/DataFrame.h"
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
