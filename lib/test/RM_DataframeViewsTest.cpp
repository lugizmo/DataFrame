// Filename: RM_DataframeViewsTest.cpp
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test functions viewing dataframe data. (one could change values though it though)
// The following functions are tested here (with names of tests):
//
//   ✅ view_all
// - Values() const -> std::span<T>
//
//   ✅ view_all_val
// - Fields() const -> Flds
// - Records() const -> Recs
//
//   ✅ view_all_seq
// - Fields() const -> Flds
// - Records() const -> Recs
//
//   ✅ view_field_val
// - ViewField(F const& index)               -> DFView<T, RecI>       requires DFValIndex<RecI>;
// - ViewField(F const& index) const         -> DFView<T const, RecI> requires DFValIndex<RecI>;
//
//   ✅ view_field_seq
// - ViewField(FldT const& index)            -> DFView<T, RecI>       requires DFSeqIndex<RecI>;
// - ViewField(FldT const& index) const      -> DFView<T const, RecI> requires DFSeqIndex<RecI>;
//
//   ✅ view_field_idx_val
// - ViewFieldIndexed(F const& index)        -> DFViewIndexed<T, RecI const>        requires DFValIndex<RecI>;
// - ViewFieldIndexed(F const& index) const  -> DFViewIndexed<T const, RecI const>  requires DFValIndex<RecI>;
//
//  TODO view_field_idx_seq
//
//   ✅ view_record_val
// - ViewRecord(R const& index)       -> DFView<T, FldI>       requires DFValIndex<RecI>;
// - ViewRecord(R const& index) const -> DFView<T const, FldI> requires DFValIndex<RecI>;
//
//  TODO view_record_seq
//
//   ✅ view_record_idx_val
// - ViewRecordIndexed(R const& index)       -> DFViewIndexed<T, FldI const>        requires DFValIndex<RecI>;
// - ViewRecordIndexed(R const& index) const -> DFViewIndexed<T const, FldI const>  requires DFValIndex<RecI>;
//
//  TODO view_record_idx_seq
//
//   ✅ select_field_val
// - operator|(SelectField<F> const& index)       -> DFView<T, RecI>       requires DFValIndex<FldI>
// - operator|(SelectField<F> const& index) const -> DFView<T const, RecI> requires DFValIndex<FldI>
//
//  TODO select_field_seq
//
//   ✅ select_record_val
// - operator|(SelectRecord<R> const& index)       -> DFView<T, FldI>       requires DFValIndex<RecI>
// - operator|(SelectRecord<R> const& index) const -> DFView<T const, FldI> requires DFValIndex<RecI>
//
//  TODO select_record_seq
//
//   ✅ select_field_idx_val
// - operator|(SelectFieldIndexed<F> const& index)        -> DFViewIndexed<T, RecI const>       requires DFValIndex<FldI>
// - operator|(SelectFieldIndexed<F> const& index) const  -> DFViewIndexed<T const, RecI const> requires DFValIndex<FldI>
//
//  TODO select_field_idx_seq
//
//   ✅ select_record_idx_val
// - operator|(SelectRecordIndexed<R> const& index)       -> DFViewIndexed<T, RecI const>       requires DFValIndex<RecI>
// - operator|(SelectRecordIndexed<R> const& index) const -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI>
//
//  TODO select_record_idx_seq
//

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

#include "RM_DataframeTestData.h"

/**
 *  @brief Viewing all values as a span over the raw memory.
 *  @see   lugizmo::Dataframe.Values() const -> std::span<T>
 */
TEST(lugizmo_dataframe_views_row_major, view_all)
{
    // no difference between the value index and sequence index
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        auto const df   = DefaultDataframe();
        auto const vals = df.Values();

        EXPECT_EQ(vals.size(), DFRecCount * DFFldCount);
        EXPECT_EQ(vals.size(), df.Size());
    }

    {
        using namespace lugizmo::test::str;

        auto const df   = DefaultDataframe();
        auto const vals = df.Values();

        EXPECT_EQ(vals.size(), DFRecCount * DFFldCount);
        EXPECT_EQ(vals.size(), df.Size());
    }
}

/**
 *  @brief Viewing all fields and records as spans.
 *  @see   lugizmo::Dataframe.Fields() const -> Flds
 *         lugizmo::Dataframe.Records() const -> Recs
 */
TEST(lugizmo_dataframe_views_row_major, view_all_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df   = DefaultDataframe();
        auto const flds = df.Fields();
        auto const recs = df.Records();

        static_assert(std::is_const_v<std::remove_reference_t<decltype(flds.front())>>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(recs.front())>>);

        EXPECT_EQ(flds.size(), DFFldCount);
        EXPECT_EQ(recs.size(), DFRecCount);

        size_t fldCount = 0;
        size_t recCount = 0;
        for(auto const fld : flds) EXPECT_EQ(fld, DFFields[fldCount++]);
        for(auto const rec : recs) EXPECT_EQ(rec, DFRecords[recCount++]);

        // mutable version
        [[maybe_unused]] auto dfM   = DefaultDataframe();
        [[maybe_unused]] auto fldsM = df.Fields();
        [[maybe_unused]] auto recsM = df.Records();

        static_assert(std::is_const_v<std::remove_reference_t<decltype(fldsM.front())>>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(recsM.front())>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df   = DefaultDataframe();
        auto const flds = df.Fields();
        auto const recs = df.Records();

        static_assert(std::is_const_v<std::remove_reference_t<decltype(flds.front())>>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(recs.front())>>);

        EXPECT_EQ(flds.size(), DFFldCount);
        EXPECT_EQ(recs.size(), DFRecCount);

        size_t fldCount = 0;
        size_t recCount = 0;
        for(auto const& fld : flds) EXPECT_EQ(fld, DFFields[fldCount++]);
        for(auto const& rec : recs) EXPECT_EQ(rec, DFRecords[recCount++]);

        // mutable version
        [[maybe_unused]] auto dfM   = DefaultDataframe();
        [[maybe_unused]] auto fldsM = df.Fields();
        [[maybe_unused]] auto recsM = df.Records();

        static_assert(std::is_const_v<std::remove_reference_t<decltype(fldsM.front())>>);
        static_assert(std::is_const_v<std::remove_reference_t<decltype(recsM.front())>>);
    }
}

/**
 *  @brief Viewing all fields and records as bounding.
 *  @see   lugizmo::Dataframe.Fields() const -> Flds
 *         lugizmo::Dataframe.Records() const -> Recs
 */
TEST(lugizmo_dataframe_views_row_major, view_all_seq)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    using namespace lugizmo::test::sequence;

    // const versions
    auto const df   = DefaultDataframe();
    auto const flds = df.Fields();
    auto const recs = df.Records();

    static_assert(std::is_const_v<decltype(flds)>);
    static_assert(std::is_const_v<decltype(recs)>);

    EXPECT_EQ(flds.size(), DFFldCount);
    EXPECT_EQ(recs.size(), DFRecCount);

    EXPECT_EQ(flds.lower, DFFields.lower);
    EXPECT_EQ(flds.upper, DFFields.upper);

    EXPECT_EQ(recs.lower, DFRecords.lower);
    EXPECT_EQ(recs.upper, DFRecords.upper);

    // mutable version
    [[maybe_unused]] auto dfM   = DefaultDataframe();
    [[maybe_unused]] auto fldsM = df.Fields();
    [[maybe_unused]] auto recsM = df.Records();

    // TODO make const static_assert(std::is_const_v<decltype(fldsM)>);
    // TODO make const static_assert(std::is_const_v<decltype(recsM)>);
}

/**
 *  @brief Iterating over fields and their values.
 *  @see   lugizmo::Dataframe.ViewField(F const& index)       -> DFView<T, RecI>       requires DFValIndex<RecI>;
 *         lugizmo::Dataframe.ViewField(F const& index) const -> DFView<T const, RecI> requires DFValIndex<RecI>;
 */
TEST(lugizmo_dataframe_views_row_major, view_field_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            auto const fldView = df.ViewField(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView) EXPECT_EQ(rec, DFData[recCount++][fldCount]);

            ++fldCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM.ViewField(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            auto const fldView = df.ViewField(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView) EXPECT_EQ(rec, DFData[recCount++][fldCount]);

            ++fldCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM.ViewField(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);
    }
}

/**
 *  @brief Iterating over fields and their values.
 *  @see   lugizmo::Dataframe.ViewField(FldT const& index)       -> DFView<T, RecI>       requires DFSeqIndex<RecI>;
 *         lugizmo::Dataframe.ViewField(FldT const& index) const -> DFView<T const, RecI> requires DFSeqIndex<RecI>;
 */
TEST(lugizmo_dataframe_views_row_major, view_field_seq)
{
    using namespace lugizmo;
    using namespace lugizmo::test;
    using namespace lugizmo::test::sequence;

    // const versions
    auto const df   = DefaultDataframe();
    auto const flds = df.Fields();
    auto const recs = df.Records();
    ASSERT_EQ(DFFields.lower, flds.lower);
    ASSERT_EQ(DFFields.upper, flds.upper);
    ASSERT_EQ(DFRecords.lower, recs.lower);
    ASSERT_EQ(DFRecords.upper, recs.upper);

    size_t fldCount = 0;
    for(auto fld = DFFields.lower; fld < DFFields.upper; ++fld)
    {
        auto const view = df.ViewField(fld);
        EXPECT_EQ(view.Size(), DFRecCount);
        EXPECT_FALSE(view.Empty());

        size_t recCount = 0;
        for(int rec = DFRecords.lower; rec < DFRecords.upper; ++rec)
        {
            auto const val = view.TryAt(rec);
            ASSERT_TRUE(val.has_value());
            EXPECT_EQ(*val, DFData[recCount++][fldCount]);
        }

        ++fldCount;
    }

    auto dfM   = DefaultDataframe();
    auto fldsM = df.Fields();
    auto recsM = df.Records();
    ASSERT_EQ(DFFields.lower, fldsM.lower);
    ASSERT_EQ(DFFields.upper, fldsM.upper);
    ASSERT_EQ(DFRecords.lower, recsM.lower);
    ASSERT_EQ(DFRecords.upper, recsM.upper);

    fldCount = 0;
    for(auto fld = DFFields.lower; fld < DFFields.upper; ++fld)
    {
        auto const view = dfM.ViewField(fld);
        EXPECT_EQ(view.Size(), DFRecCount);
        EXPECT_FALSE(view.Empty());

        size_t recCount = 0;
        for(int rec = DFRecords.lower; rec < DFRecords.upper; ++rec)
        {
            auto const val = view.TryAt(rec);
            ASSERT_TRUE(val.has_value());
            EXPECT_EQ(*val, DFData[recCount++][fldCount]);
        }

        ++fldCount;
    }
}

/**
 *  @brief View fields and their values. Also getting the index of the record.
 *  @see   lugizmo::Dataframe.ViewFieldIndexed(F const& index)       -> DFViewIndexed<T, RecI const>       requires DFValIndex<RecI>;
 *         lugizmo::Dataframe.ViewFieldIndexed(F const& index) const -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI>;
 */
TEST(lugizmo_dataframe_views_row_major, view_field_idx_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            auto const fldView = df.ViewFieldIndexed(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView)
            {
                EXPECT_EQ(rec.val, DFData[recCount][fldCount]);
                EXPECT_EQ(rec.idx, DFRecords[recCount++]);
            }

            ++fldCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM.ViewFieldIndexed(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            auto const fldView = df.ViewFieldIndexed(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView)
            {
                EXPECT_EQ(rec.val, DFData[recCount][fldCount]);
                EXPECT_EQ(rec.idx.Get(), DFRecords[recCount++]); // TODO get rid of the Get()
            }

            ++fldCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM.ViewFieldIndexed(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);
    }
}

/**
 *  @brief View records and their values.
 *  @see   lugizmo::Dataframe.ViewRecord(R const& index)       -> DFView<T, FldI>       requires DFValIndex<RecI>;
 *         lugizmo::Dataframe.ViewRecord(R const& index) const -> DFView<T const, FldI> requires DFValIndex<RecI>;
 */
TEST(lugizmo_dataframe_views_row_major, view_record_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const recName : DFRecords)
        {
            auto const recView = df.ViewRecord(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView) EXPECT_EQ(fld, DFData[recCount][fldCount++]);

            ++recCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM.ViewRecord(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const& recName : DFRecords)
        {
            auto const recView = df.ViewRecord(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView) EXPECT_EQ(fld, DFData[recCount][fldCount++]);

            ++recCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM.ViewRecord(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);
    }
}

/**
 *  @brief View records and their values. Also getting the index of the fields.
 *  @see   lugizmo::Dataframe.ViewRecordIndexed(R const& index)       -> DFViewIndexed<T, FldI const>        requires DFValIndex<RecI>;
 *         lugizmo::Dataframe.ViewRecordIndexed(R const& index) const -> DFViewIndexed<T const, FldI const>  requires DFValIndex<RecI>;
 */
TEST(lugizmo_dataframe_views_row_major, view_record_idx_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const recName : DFRecords)
        {
            auto const recView = df.ViewRecordIndexed(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView)
            {
                EXPECT_EQ(fld.val, DFData[recCount][fldCount]);
                EXPECT_EQ(fld.idx, DFFields[fldCount++]);
            }

            ++recCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM.ViewRecordIndexed(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const& recName : DFRecords)
        {
            auto const recView = df.ViewRecordIndexed(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView)
            {
                EXPECT_EQ(fld.val, DFData[recCount][fldCount]);
                EXPECT_EQ(fld.idx.Get(), DFFields[fldCount++]); // TODO get rid of the Get()
            }

            ++recCount;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM.ViewRecordIndexed(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);
    }
}

/**
 *  @brief View fields and their values using ranges/selectors.
 *  @see   lugizmo::Dataframe.operator|(SelectField<F> const& index)       -> DFView<T, RecI>       requires DFValIndex<FldI>
 *         lugizmo::Dataframe.operator|(SelectField<F> const& index) const -> DFView<T const, RecI> requires DFValIndex<FldI>
 */
TEST(lugizmo_dataframe_views_row_major, select_field_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            auto const fldView = df | SelectField(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView) EXPECT_EQ(rec, DFData[recCount++][fldCount]);

            fldCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM | SelectField(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            auto const fldView = df | SelectField(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView) EXPECT_EQ(rec, DFData[recCount++][fldCount]);

            fldCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM | SelectField(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);
    }
}

/**
 *  @brief View records and their values using ranges/selectors.
 *  @see   lugizmo::Dataframe.operator|(SelectRecord<R> const& index)       -> DFView<T, FldI>       requires DFValIndex<RecI>
 *         lugizmo::Dataframe.operator|(SelectRecord<R> const& index) const -> DFView<T const, FldI> requires DFValIndex<RecI>
 */
TEST(lugizmo_dataframe_views_row_major, select_record_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const recName : DFRecords)
        {
            auto const recView = df | SelectRecord(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView) EXPECT_EQ(fld, DFData[recCount][fldCount++]);

            recCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM | SelectRecord(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const& recName : DFRecords)
        {
            auto const recView = df | SelectRecord(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView) EXPECT_EQ(fld, DFData[recCount][fldCount++]);

            recCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM | SelectRecord(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);
    }
}

/**
 *  @brief View fields and their values using ranges/selectors. Also getting the index of the records.
 *  @see   lugizmo::Dataframe.operator|(SelectFieldIndexed<F> const& index)        -> DFViewIndexed<T, RecI const>       requires DFValIndex<FldI>
 *         lugizmo::Dataframe.operator|(SelectFieldIndexed<F> const& index) const  -> DFViewIndexed<T const, RecI const> requires DFValIndex<FldI>
 */
TEST(lugizmo_dataframe_views_row_major, select_field_idx_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const fldName : DFFields)
        {
            auto const fldView = df | SelectFieldIndexed(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView)
            {
                EXPECT_EQ(rec.val, DFData[recCount][fldCount]);
                EXPECT_EQ(rec.idx, DFRecords[recCount++]);
            }

            fldCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM | SelectFieldIndexed(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t fldCount = 0;
        for(auto const& fldName : DFFields)
        {
            auto const fldView = df | SelectFieldIndexed(fldName);
            static_assert(std::is_const_v<decltype(fldView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);

            EXPECT_EQ(fldView.Size(), DFRecCount);
            EXPECT_FALSE(fldView.Empty());

            size_t recCount = 0;
            for(auto const rec : fldView)
            {
                EXPECT_EQ(rec.val, DFData[recCount][fldCount]);
                EXPECT_EQ(rec.idx.Get(), DFRecords[recCount++]);    // TODO get rid of the Get()
            }

            fldCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto fldView = dfM | SelectFieldIndexed(DFFields[0]);
        static_assert(not std::is_const_v<decltype(fldView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(fldView.At("0"))>>);
    }
}

/**
 *  @brief View records and their values using ranges/selectors. Also getting the index of the fields.
 *  @see   lugizmo::Dataframe.operator|(SelectRecordIndexed<R> const& index)       -> DFViewIndexed<T, RecI const>       requires DFValIndex<RecI>
 *         lugizmo::Dataframe.operator|(SelectRecordIndexed<R> const& index) const -> DFViewIndexed<T const, RecI const> requires DFValIndex<RecI>
 */
TEST(lugizmo_dataframe_views_row_major, select_record_idx_val)
{
    using namespace lugizmo;
    using namespace lugizmo::test;

    {
        using namespace lugizmo::test::integer;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const recName : DFRecords)
        {
            auto const recView = df | SelectRecordIndexed(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView)
            {
                EXPECT_EQ(fld.val, DFData[recCount][fldCount]);
                EXPECT_EQ(fld.idx, DFFields[fldCount++]);
            }

            recCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM | SelectRecordIndexed(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At(0))>>);
    }

    {
        using namespace lugizmo::test::str;

        // const versions
        auto const df = DefaultDataframe();

        size_t recCount = 0;
        for(auto const& recName : DFRecords)
        {
            auto const recView = df | SelectRecordIndexed(recName);
            static_assert(std::is_const_v<decltype(recView)>);
            static_assert(std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);

            EXPECT_EQ(recView.Size(), DFFldCount);
            EXPECT_FALSE(recView.Empty());

            size_t fldCount = 0;
            for(auto const fld : recView)
            {
                EXPECT_EQ(fld.val, DFData[recCount][fldCount]);
                EXPECT_EQ(fld.idx.Get(), DFFields[fldCount++]);    // TODO get rid of the Get()
            }

            recCount++;
        }

        // mutable versions
        [[maybe_unused]] auto dfM     = DefaultDataframe();
        [[maybe_unused]] auto recView = dfM | SelectRecordIndexed(DFRecords[0]);
        static_assert(not std::is_const_v<decltype(recView)>);
        static_assert(not std::is_const_v<std::remove_pointer_t<decltype(recView.At("0"))>>);
    }
}