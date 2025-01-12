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
    constexpr auto valInt    = DFValIndex<DFUniqueIndex<int>>;
    constexpr auto valStr    = DFValIndex<DFUniqueIndex<std::string>>;
    constexpr auto idxValInt = DFIdxType<DFUniqueIndex<std::string>>;
    constexpr auto idxValStr = DFIdxType<DFUniqueIndex<int>>;
    ASSERT_TRUE(valInt);
    ASSERT_TRUE(valStr);
    ASSERT_TRUE(idxValInt);
    ASSERT_TRUE(idxValStr);

    // multiple unique indices
    // TODO clang does show annoying errors in the moment but compiles
    //constexpr auto valMulInt    = DFValIndex<DFUniqueIndex<int>, DFUniqueIndex<long>>;
    //constexpr auto valMulStr    = DFValIndex<DFUniqueIndex<std::string>, DFUniqueIndex<int>>;
    //constexpr auto idxValMulInt = DFIdxType<DFUniqueIndex<int>, DFUniqueIndex<long>>;
    //constexpr auto idxValMulStr = DFIdxType<DFUniqueIndex<std::string>, DFUniqueIndex<int>>;
    //ASSERT_TRUE(valMulInt);
    //ASSERT_TRUE(valMulStr);
    //ASSERT_TRUE(idxValMulInt);
    //ASSERT_TRUE(idxValMulStr);

    // range index
    constexpr auto seqInt    = DFSeqIndex<DFRangeIndex<int>>;
    constexpr auto idxSeqInt = DFIdxType<DFRangeIndex<int>>;
    ASSERT_TRUE(seqInt);
    ASSERT_TRUE(idxSeqInt);

    // multiple range indices
    // TODO clang does show annoying errors in the moment but compiles
    //constexpr auto seqMultInt   = DFSeqIndex<DFRangeIndex<int>, DFRangeIndex<long>>;
    //constexpr auto idxSeqMulInt = DFIdxType<DFRangeIndex<int>, DFRangeIndex<long>>;
    //ASSERT_TRUE(seqMultInt);
    //ASSERT_TRUE(idxSeqMulInt);

    // not index
    constexpr auto notValInt = DFValIndex<int>;
    constexpr auto notSeqInt = DFSeqIndex<int>;
    constexpr auto notIdxInt = DFIdxType<int>;
    ASSERT_FALSE(notValInt);
    ASSERT_FALSE(notSeqInt);
    ASSERT_FALSE(notIdxInt);

    // multiple not index
    // TODO clang does show annoying errors in the moment but compiles
    //constexpr auto notValMulInt = DFValIndex<int, long>;
    //constexpr auto notSeqMulInt = DFSeqIndex<int, long>;
    //constexpr auto notIdxMulInt = DFIdxType<int, long>;
    //ASSERT_FALSE(notValMulInt);
    //ASSERT_FALSE(notSeqMulInt);
    //ASSERT_FALSE(notIdxMulInt);
}