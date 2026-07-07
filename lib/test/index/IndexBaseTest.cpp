// Filename: IndexBaseTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the index concepts shared by all index types.
// The following concepts are tested here (with names of tests):
//
// ✅ Concepts - DFUnqIndex / DFRngIndex / DFIdxType
//

#include "gtest/gtest.h"

#include <string>

#include "lugizmo/dataframe/index/IndexBase.h"
#include "lugizmo/dataframe/index/IndexRange.h"
#include "lugizmo/dataframe/index/IndexUnique.h"

TEST(DataframeIndexBase, Concepts)
{
    using namespace lgz;

    // unique index
    constexpr auto valInt    = DFUnqIndex<DFUniqueIndex<int>>;
    constexpr auto valStr    = DFUnqIndex<DFUniqueIndex<std::string>>;
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
    constexpr auto seqInt    = DFRngIndex<DFRangeIndex<int>>;
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
    constexpr auto notValInt = DFUnqIndex<int>;
    constexpr auto notSeqInt = DFRngIndex<int>;
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