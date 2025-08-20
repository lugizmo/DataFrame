// Filename: RM_DataframeMutatingTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions accessing properties of the dataframe.
// The following functions are tested here (with names of tests):
//
// ✅ has
// - HasField<C>(C const& field) const -> bool
// - HasRecord<C>(C const& record) const -> bool
//
// ✅ sizes
// - Size() const -> size_t
// - FieldSize() const -> size_t
// - RecordSize() const -> size_t
// - Empty() const -> bool


#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include "RM_DataframeTestData.h"

/**
 *  @brief Check if fields or records are present
 *  @see   lugizmo::Dataframe.HasField<C>(C const& field) const -> bool
 *         lugizmo::Dataframe.HasRecord<C>(C const& record) const -> bool
 */
TEST(lugizmo_dataframe_properties_row_major, has)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;
    using namespace lugizmo::test::str;

    auto const df = DefaultDataframe();
    for(auto const& fld: DFFields)  EXPECT_TRUE(df.HasField(fld));
    for(auto const& rec: DFRecords) EXPECT_TRUE(df.HasRecord(rec));

    for(auto const& fld: DFFields)  EXPECT_TRUE(df.HasField(std::string_view(fld)));   // checking comparable concept
    for(auto const& rec: DFRecords) EXPECT_TRUE(df.HasRecord(std::string_view(rec)));  // checking comparable concept
}

/**
 *  @brief Check sizes/dimensions of the dataframe.
 *  @see   lugizmo::Dataframe.Size() const -> size_t
 *         lugizmo::Dataframe.FieldSize() const -> size_t
 *         lugizmo::Dataframe.RecordSize() const -> size_t
 *         lugizmo::Dataframe.Empty() const -> bool
 */
TEST(lugizmo_dataframe_properties_row_major, sizes)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;
    using namespace lugizmo::test::integer;

    auto const df = DefaultDataframe();
    ASSERT_EQ(df.Size(), DFRecCount * DFFldCount);
    ASSERT_EQ(df.FieldSize(), DFFldCount);
    ASSERT_EQ(df.RecordSize(), DFRecCount);
    ASSERT_FALSE(df.Empty());

    auto const empty = DataFrame<int, int, int>();
    ASSERT_EQ(empty.Size(), 0);
    ASSERT_EQ(empty.FieldSize(), 0);
    ASSERT_EQ(empty.RecordSize(), 0);
    ASSERT_TRUE(empty.Empty());
}


