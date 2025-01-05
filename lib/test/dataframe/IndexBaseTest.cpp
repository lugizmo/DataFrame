// Filename: IndexUniqueTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <string>

#include "lugizmo/dataframe/IndexBase.h"
#include "lugizmo/dataframe/IndexRange.h"
#include "lugizmo/dataframe/IndexUnique.h"

TEST(lugizmo_dataframe_index_base_test, concepts)
{
    using namespace lugizmo;

    // unique index
    static_assert(DFValueIndex<DFUniqueIndex<int>>);
    static_assert(DFValueIndex<DFUniqueIndex<std::string>>);
    static_assert(DFIndexType<DFUniqueIndex<std::string>>);
    static_assert(DFIndexType<DFUniqueIndex<int>>);

    // range index
    static_assert(DFSequenceIndex<DFRangeIndex<int>>);
    static_assert(DFIndexType<DFRangeIndex<int>>);

    // not index
    static_assert(not DFValueIndex<int>);
    static_assert(not DFSequenceIndex<int>);
    static_assert(not DFIndexType<int>);
}