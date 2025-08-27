// Filename: DataFrameAccessRowMajorTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions accessing individual items.
//

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

/**
 *  @brief Get value by value. "Slow Path"
 *  @see  lugizmo::Dataframe.GetValue
 */
TEST(lugizmo_dataframe_access_row_major, get_value)
{
    using namespace lugizmo;

    auto Test = []<typename T>(T& df) static
    {
        for(auto const& rec : df.Records())
        {
            auto count = 0;
            for(auto const& fld : df.Fields())
            {
                // function to test
                auto const val = df.GetValue(fld, rec);
                ASSERT_TRUE(val.has_value());
                ASSERT_EQ(*val, count);

                ++count;
            }
        }
    };

    // test for the const and non const version of GetValue
    auto const constDF = DataFrame<int, int, int>::FromFieldsAndRecords({0, 1, 2}, {0, 1, 2}, {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}});
    auto mutDF         = DataFrame<int, int, int>::FromFieldsAndRecords({0, 1, 2}, {0, 1, 2}, {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}});

    ASSERT_EQ(constDF.Records().size(), 3);
    ASSERT_EQ(constDF.Fields().size(), 3);
    ASSERT_EQ(mutDF.Records().size(), 3);
    ASSERT_EQ(mutDF.Fields().size(), 3);

    Test(constDF);
    Test(mutDF);
}

/**
 *  @brief Get value by value. "Slow Path"
 *  @see  lugizmo::Dataframe.GetValue
 */
TEST(lugizmo_dataframe_access_row_major, get_value_by_operator)
{
    using namespace lugizmo;

    auto Test = []<typename T>(T& df) static
    {
        for(auto const& rec : df.Records())
        {
            auto count = 0;
            for(auto const& fld : df.Fields())
            {
                // should not be needed as records and
                // fields taken from the dataframe.
                ASSERT_TRUE(df.HasRecord(rec));
                ASSERT_TRUE(df.HasField(fld));

                // function to test
                auto const& val = df[fld, rec];
                ASSERT_EQ(val, count);

                ++count;
            }
        }
    };

    // test for the const and non const version of GetValue
    auto const constDF = DataFrame<int, int, int>::FromFieldsAndRecords({0, 1, 2}, {0, 1, 2}, {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}});
    auto mutDF         = DataFrame<int, int, int>::FromFieldsAndRecords({0, 1, 2}, {0, 1, 2}, {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}});

    ASSERT_EQ(constDF.Records().size(), 3);
    ASSERT_EQ(constDF.Fields().size(), 3);
    ASSERT_EQ(mutDF.Records().size(), 3);
    ASSERT_EQ(mutDF.Fields().size(), 3);

    Test(constDF);
    Test(mutDF);
}

/**
 *  @brief Set value by value. "Slow Path"
 *  @see  lugizmo::Dataframe.GetValue
 */
TEST(lugizmo_dataframe_access_row_major, set_value)
{
    using namespace lugizmo;

    auto df = DataFrame<int, int, int>::FromFieldsAndRecords({0, 1, 2}, {0, 1, 2}, {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}});
    ASSERT_EQ(df.Records().size(), 3);
    ASSERT_EQ(df.Fields().size(), 3);

    for(auto const& rec : df.Records())
        for(auto const& fld : df.Fields())
            ASSERT_TRUE(df.Replace(fld, rec, 42));

    for(auto const val : df.Values())
        ASSERT_EQ(val, 42);
}