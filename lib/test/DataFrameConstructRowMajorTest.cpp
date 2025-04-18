// Filename: DataFrameConstructionTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions constructing a dataframe either
// from constructors or helper functions.
//

#include "gtest/gtest.h"

#include <array>
#include <vector>
#include <string>

#include "lugizmo/DataFrame.h"

/**
 *  Test if the default layout is row_major/std::layout_right.
 */
TEST(lugizmo_dataframe_construct_row_major, default_layout)
{
    using namespace lugizmo;
    using DF = DataFrame<int, int, int>;
    static_assert(std::is_same_v<DF::Layout, DFRowMajor<int>>);
}

/**
 *  @brief Test the default constructor that should create a completely
 *         empty dataframe with no allocation happening.
 *
 *  @see lugizmo::DataFrame::DataFrame(size_t, std::shared_ptr<std::pmr::memory_resource>)
 */
TEST(lugizmo_dataframe_construct_row_major, default_constructor)
{
    using DFInt   = lugizmo::DataFrame<int, int, int>;
    auto const df = DFInt{};

    ASSERT_EQ(df.Size(), 0);
    // TODO add ASSERT_EQ(df.Capacity(), 0) << "Default Capacity should be 0";
    ASSERT_TRUE(df.Empty());

    ASSERT_EQ(df.FieldSize(), 0);
    ASSERT_EQ(df.RecordSize(), 0);
    ASSERT_EQ(df.Data(), nullptr) << "Empty dataframe with no reservation does not need valid data.";
}

/**
 *  @brief Test the default constructor that reserved memory upfront
 *         so memory got allocated but no content got populated yet.
 *
 *  @see lugizmo::DataFrame::DataFrame(size_t, std::shared_ptr<std::pmr::memory_resource>)
 */
TEST(lugizmo_dataframe_construct_row_major, reserved_constructor)
{
    using DFInt                    = lugizmo::DataFrame<int, int, int>;
    constexpr size_t reserveValues = 50;

    auto const df = DFInt{reserveValues};
    ASSERT_EQ(df.Size(), 0);
    // TODO add ASSERT_EQ(df.Capacity(), reserveValues) << "Capacity should be of reserveValues!"
    ASSERT_TRUE(df.Empty());

    ASSERT_EQ(df.FieldSize(), 0);
    ASSERT_EQ(df.RecordSize(), 0);
    ASSERT_NE(df.Data(), nullptr);
}

/**
 *  @brief Test creating the dataframe from field names.
 *         This can either create memory when using reserved values.
 *  @see FromFields("span", size_t, std::shared_ptr<std::pmr::memory_resource> res)
 */
TEST(lugizmo_dataframe_construct_row_major, from_fields)
{
    using DFInt           = lugizmo::DataFrame<int, int, int>;
    constexpr auto fields = std::array{1, 2, 3, 4};

    // no reservation
    {
        auto const df = DFInt::FromFields(fields);

        ASSERT_EQ(df.Size(), 0) << "Reserved constructor should initialize with size 0.";
        // TODO add ASSERT_TRUE(df.Capacity() > df.Size()) << "Capacity should be 0!"
        ASSERT_TRUE(df.Empty()) << "DataFrame should be empty.";

        ASSERT_EQ(df.FieldSize(), fields.size());
        ASSERT_EQ(df.RecordSize(), 0);
        // TODO why not nullptr? ASSERT_EQ(df.Data(), nullptr);

        auto countFld = 0;
        for(auto fld : df.Fields())
        {
            ASSERT_TRUE(countFld < df.FieldSize());
            ASSERT_EQ(fld, fields[countFld]);
            ++countFld;
        }
    }

    // with reservation
    {
        auto const df = DFInt::FromFields(fields, 5);

        ASSERT_EQ(df.Size(), 0) << "Reserved constructor should initialize with size 0.";
        // TODO add ASSERT_TRUE(df.Capacity() > df.Size()) << "Capacity should be set!"
        ASSERT_TRUE(df.Empty()) << "DataFrame should be empty.";

        ASSERT_EQ(df.FieldSize(), fields.size());
        ASSERT_EQ(df.RecordSize(), 0);
        ASSERT_NE(df.Data(), nullptr);

        auto countFld = 0;
        for(auto fld : df.Fields())
        {
            ASSERT_TRUE(countFld < df.FieldSize());
            ASSERT_EQ(fld, fields[countFld]);
            ++countFld;
        }
    }
}

/**
 *  TODO doc see from_fields test
 *  TODO Currently fields must be present before adding records.
 *       This should change and the index should be updated as for the fields.
 */
TEST(lugizmo_dataframe_construct_row_major, from_records)
{
}

/**
 *   @brief Test creating the dataframe by adding field after field
 *          and then record after record.
 *   @note  This is not how you should create a dataframe better to
 *          iterate over full records or fields.
 *          TODO add reference to doc for fast initialization
 *
 *   @see lugizmo::DataFrame.AddField(value)
 *   @see lugizmo::DataFrame.AddRecord(value)
 */
TEST(lugizmo_dataframe_construct_row_major, one_by_on_fields)
{
    using namespace lugizmo;
    using TestDF = DataFrame<float, std::string, int>;

    constexpr std::size_t FLD_COUNT = 10;
    constexpr std::size_t REC_COUNT = 10;

    auto df = TestDF(FLD_COUNT * REC_COUNT);
    for(auto fld = 0; fld < FLD_COUNT; ++fld) df.AddField(std::to_string(fld));
    for(auto rec = 0; rec < REC_COUNT; ++rec) df.AddRecord(rec, 42.f);

    ASSERT_TRUE(df.Size() == FLD_COUNT * REC_COUNT);
    ASSERT_TRUE(df.FieldSize() == FLD_COUNT);
    ASSERT_TRUE(df.RecordSize() == REC_COUNT);
    ASSERT_TRUE(std::ranges::all_of(df.ValuesSpan(), [](auto const val) { return val == 42.f; }));

    for(auto const& val : df.ValuesSpan()) ASSERT_FLOAT_EQ(42.f, val);

    auto count = 0;
    for(auto const& fld : df.Fields()) ASSERT_EQ(fld, std::to_string(count++));

    count = 0;
    for(auto const& rec : df.Records()) ASSERT_EQ(rec, count++);
}

/**
 *   @brief Test creating the dataframe by adding record after record
 *          and then field after field.
 *   @note  This is not how you should create a dataframe better to
 *          iterate over full records or fields.
 *          TODO add reference to doc for fast initialization
 *
 *   @see lugizmo::DataFrame.AddRecord(value)
 *   @see lugizmo::DataFrame.AddField(value)
 */
TEST(lugizmo_dataframe_construct_row_major, one_by_on_records)
{
    // using namespace lugizmo;
    // using TestDF = DataFrame<float, std::string, int>;

    // TODO add when adding first records is allowed
    //
    // constexpr std::size_t FLD_COUNT = 10;
    // constexpr std::size_t REC_COUNT = 10;
    //
    // auto df = TestDF(FLD_COUNT * REC_COUNT);
    // for(auto rec = 0; rec < REC_COUNT; ++rec) df.AddRecord(rec, 42.f);
    // for(auto fld = 0; fld < FLD_COUNT; ++fld) df.AddField(std::to_string(fld));
    //
    // ASSERT_TRUE(!df.Empty());
    // ASSERT_TRUE(!df.Fields().empty());
    // ASSERT_TRUE(!df.Records().empty());
    // ASSERT_TRUE(df.Fields().size() == FLD_COUNT);
    // ASSERT_TRUE(df.Records().size() == REC_COUNT);
    // ASSERT_TRUE(df.FieldSize() == FLD_COUNT);
    // ASSERT_TRUE(df.RecordSize() == REC_COUNT);
    // ASSERT_TRUE(std::ranges::all_of(df.ValuesSpan(), [](auto const val) { return val == 42.f; }));
}

/**
 *  @brief Create a dataframe from know fields and records and
 *         using the default value to initialize values in the table.
 *
 *  @see lugizmo::DataFrame::FromFieldsAndRecord(fields, records)
 */
TEST(lugizmo_dataframe_construct_row_major, from_fields_and_record_defaulted)
{
    using namespace lugizmo;
    using TestDF = DataFrame<float, std::string, int>;

    constexpr auto Flds = std::array{std::string("0"), std::string("1"), std::string("2")};
    constexpr auto Recs = std::array{0, 1, 2};

    auto const dfFromFldAndRecDef = TestDF::FromFieldsAndRecord(Flds, Recs);
    ASSERT_TRUE(dfFromFldAndRecDef.Size() == 3 * 3);
    ASSERT_TRUE(dfFromFldAndRecDef.FieldSize() == 3);
    ASSERT_TRUE(dfFromFldAndRecDef.RecordSize() == 3);

    auto countFld = 0;
    for(auto const& field : dfFromFldAndRecDef.Fields())
    {
        ASSERT_TRUE(countFld < dfFromFldAndRecDef.FieldSize());
        ASSERT_EQ(field, Flds[countFld]);
        ++countFld;
    }

    auto countRec = 0;
    for(auto const& record : dfFromFldAndRecDef.Records())
    {
        ASSERT_TRUE(countRec < dfFromFldAndRecDef.RecordSize());
        ASSERT_EQ(record, Recs[countRec]);
        ++countRec;
    }

    for(auto const val : dfFromFldAndRecDef.ValuesSpan()) ASSERT_EQ(float(), val);
}

/**
 *  @brief Create a dataframe from know fields and records and
 *         using the same defined values for the records.
 *         Therefore, the records must have the size of the fields.
 *
 *  @see lugizmo::DataFrame::FromFieldsAndRecord(fields, records, data)
 */
TEST(lugizmo_dataframe_construct_row_major, from_fields_and_record_set)
{
    using namespace lugizmo;
    using TestDF = DataFrame<float, std::string, int>;

    constexpr auto Flds     = std::array{std::string("0"), std::string("1"), std::string("2")};
    constexpr auto Recs     = std::array{0, 1, 2};
    constexpr auto RecsData = std::array{1.f, 2.f, 3.f};

    auto const dfFromFldAndRec = TestDF::FromFieldsAndRecord(Flds, Recs, RecsData);
    ASSERT_TRUE(dfFromFldAndRec.Size() == 3 * 3);
    ASSERT_TRUE(dfFromFldAndRec.Fields().size() == 3);
    ASSERT_TRUE(dfFromFldAndRec.Records().size() == 3);

    auto countFld = 0;
    for(auto const& field : dfFromFldAndRec.Fields())
    {
        ASSERT_TRUE(countFld < dfFromFldAndRec.FieldSize());
        ASSERT_EQ(field, Flds[countFld]);
        ++countFld;
    }

    auto countRec = 0;
    for(auto const& record : dfFromFldAndRec.Records())
    {
        ASSERT_TRUE(countRec < dfFromFldAndRec.RecordSize());
        ASSERT_EQ(record, Recs[countRec]);
        ++countRec;
    }

    for(auto const record : dfFromFldAndRec.Records())
    {
        countFld = 0;
        for(auto const& val : dfFromFldAndRec.ViewRecord(record))
        {
            ASSERT_TRUE(countFld < dfFromFldAndRec.FieldSize());
            ASSERT_EQ(RecsData[countFld], val);
            ++countFld;
        }
    }
}

/**
 *  @brief Create a dataframe from know fields and records and
 *         initializing all values explicitly.
 *         Therefore, the data must have the shape of the dataframe.
 *
 *  @see lugizmo::DataFrame::FromFieldsAndRecords(fields, records, iterable)
 */
TEST(lugizmo_dataframe_construct_row_major, from_fields_and_records_iterable)
{
    using namespace lugizmo;
    using TestDF = DataFrame<float, std::string, int>;

    constexpr auto Flds = std::array{std::string("0"), std::string("1"), std::string("2")};
    constexpr auto Recs = std::array{0, 1, 2};

    constexpr auto RecsAllData = std::array{
            std::array{1.f, 2.f, 3.f},
            std::array{4.f, 5.f, 6.f},
            std::array{7.f, 8.f, 9.f}
    };

    auto const dfFromFldAndRecs = TestDF::FromFieldsAndRecords(Flds, Recs, RecsAllData);
    ASSERT_TRUE(!dfFromFldAndRecs.Empty());
    ASSERT_TRUE(dfFromFldAndRecs.Fields().size() == 3);
    ASSERT_TRUE(dfFromFldAndRecs.Records().size() == 3);

    auto countRec = 0;
    auto countFld = 0;
    for(auto const record : dfFromFldAndRecs.Records())
    {
        ASSERT_EQ(Recs[countRec], record);
        for(auto const& field : dfFromFldAndRecs.Fields())
        {
            ASSERT_EQ(Flds[countFld], field);
            ASSERT_TRUE(countRec < dfFromFldAndRecs.RecordSize());
            ASSERT_TRUE(countFld < dfFromFldAndRecs.FieldSize());

            auto const dfVal = dfFromFldAndRecs.GetValue(field, record);
            ASSERT_TRUE(dfVal.has_value());
            ASSERT_EQ(RecsAllData[countRec][countFld], *dfVal);

            ++countFld;
        }

        countFld = 0;
        ++countRec;
    }
}

/**
 *  @brief Create a dataframe from know fields and records and
 *         initializing all values explicitly.
 *         Therefore, the data must have the shape of the dataframe.
 *
 *  @see lugizmo::DataFrame::FromFieldsAndRecords(fields, records, initializer_list)
 */
TEST(lugizmo_dataframe_construct_row_major, from_fields_and_records_initializer_list)
{
    using namespace lugizmo;
    using TestDF = DataFrame<float, std::string, int>;

    constexpr auto Flds = std::array{std::string("0"), std::string("1"), std::string("2")};
    constexpr auto Recs = std::array{0, 1, 2};

    constexpr auto RecsAllData = std::array{
            std::array{1.f, 2.f, 3.f},
            std::array{4.f, 5.f, 6.f},
            std::array{7.f, 8.f, 9.f}
    };

    // clang-format off
    auto const dfFromInitListDef = TestDF::FromFieldsAndRecords({"0", "1", "2"}, {0, 1, 2});
    auto const dfFromInitList    = TestDF::FromFieldsAndRecords({"0", "1", "2"}, {0, 1, 2},
                                                                {{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}, {7.f, 8.f, 9.f}});
    // clang-format on
    ASSERT_TRUE(!dfFromInitListDef.Empty());
    ASSERT_TRUE(dfFromInitListDef.Fields().size() == 3);
    ASSERT_TRUE(dfFromInitListDef.Records().size() == 3);

    ASSERT_TRUE(!dfFromInitList.Empty());
    ASSERT_TRUE(dfFromInitList.Fields().size() == 3);
    ASSERT_TRUE(dfFromInitList.Records().size() == 3);

    auto countRec = 0;
    auto countFld = 0;
    for(auto const record : dfFromInitList.Records())
    {
        ASSERT_EQ(Recs[countRec], record);
        for(auto const& field : dfFromInitList.Fields())
        {
            ASSERT_EQ(Flds[countFld], field);
            ASSERT_TRUE(countRec < dfFromInitList.RecordSize());
            ASSERT_TRUE(countFld < dfFromInitList.FieldSize());

            auto const dfVal = dfFromInitList.GetValue(field, record);
            ASSERT_TRUE(dfVal.has_value());
            ASSERT_EQ(RecsAllData[countRec][countFld], *dfVal);

            ++countFld;
        }

        countFld = 0;
        ++countRec;
    }
}
