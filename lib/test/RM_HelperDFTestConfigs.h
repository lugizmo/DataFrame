// Filename: RM_DataframeTestConfigs.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Index configurations for cross-index typed tests (TYPED_TEST).
//
// Each config-traits type exposes the same surface so an index-agnostic behaviour can be
// written once and run against every index configuration:
//
//   - DF                     : the DataFrame type
//   - FLD_COUNT / REC_COUNT  : field / record count of the built frame (3 x 3)
//   - Build()                : a populated FLD_COUNT x REC_COUNT frame (values default 0)
//   - FieldKey(i)            : key of the i-th field   (i in [0, FLD_COUNT))
//   - RecordKey(i)           : key of the i-th record  (i in [0, REC_COUNT))
//   - MissingField()         : a field key that is NOT a member of the frame
//   - MissingRecord()        : a record key that is NOT a member of the frame
//
// The traits hide how a frame is built (AddFields/AddRecords for unique vs.
// SetFieldRange/SetRecordRange for range) and what counts as a non-member key
// (out-of-set for unique/range, off-grid for the strided range).
//

#ifndef LUGIZMO_TEST_RM_DATAFRAME_TEST_CONFIGS_H
#define LUGIZMO_TEST_RM_DATAFRAME_TEST_CONFIGS_H

#include <array>
#include <cstddef>

#include "gtest/gtest.h"

#include "lugizmo/DataFrame.h"

namespace lugizmo::test {

    // All configs build a 3x3 frame and expose int field/record keys.
    inline constexpr std::size_t TEST_FLD_COUNT  = 3;
    inline constexpr std::size_t TEST_REC_COUNT = 3;

    /// @brief Unique x unique index (value index, hash-backed labels).
    struct Unique_IndexTest // NOLINT(readability-identifier-naming)
    {
        using DF = DataFrame<int, int, int>; // field/record key type int -> DFUniqueIndex<int>

        static constexpr std::size_t FLD_COUNT = TEST_FLD_COUNT;
        static constexpr std::size_t REC_COUNT = TEST_REC_COUNT;

        static auto Build() -> DF
        {
            DF df;
            df.AddFields(std::array{0, 1, 2});
            df.AddRecords(std::array{0, 1, 2});
            return df;
        }

        static auto FieldKey(std::size_t const i)  -> int { return static_cast<int>(i); }       // 0, 1, 2
        static auto RecordKey(std::size_t const i) -> int { return static_cast<int>(i); }       // 0, 1, 2
        static auto MissingField()                 -> int { return 99; }                        // out of the field set
        static auto MissingRecord()                -> int { return 99; }                        // out of the record set
    };

    /// @brief Unique field index x strided range record index.
    struct UniqueRange_IndexTest // NOLINT(readability-identifier-naming)
    {
        using DF = DataFrame<int, int, DFRangeIndex<int>>;

        static constexpr std::size_t FLD_COUNT = TEST_FLD_COUNT;
        static constexpr std::size_t REC_COUNT = TEST_REC_COUNT;

        static auto Build() -> DF
        {
            DF df;
            df.AddFields(std::array{10, 20, 30});
            df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 6, .step = 2}, 0); // 0, 2, 4
            return df;
        }

        static auto FieldKey(std::size_t const i)  -> int { return static_cast<int>((i + 1) * 10); } // 10, 20, 30
        static auto RecordKey(std::size_t const i) -> int { return static_cast<int>(i * 2); }         // 0, 2, 4
        static auto MissingField()                 -> int { return 99; }                              // out of the field set
        static auto MissingRecord()                -> int { return 1; }                               // off the step-2 grid
    };

    /// @brief Strided range field index x unique record index.
    struct RangeUnique_IndexTest // NOLINT(readability-identifier-naming)
    {
        using DF = DataFrame<int, DFRangeIndex<int>, int>;

        static constexpr std::size_t FLD_COUNT = TEST_FLD_COUNT;
        static constexpr std::size_t REC_COUNT = TEST_REC_COUNT;

        static auto Build() -> DF
        {
            DF df;
            df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 6, .step = 2}); // 0, 2, 4
            df.AddRecords(std::array{10, 20, 30});
            return df;
        }

        static auto FieldKey(std::size_t const i)  -> int { return static_cast<int>(i * 2); }          // 0, 2, 4
        static auto RecordKey(std::size_t const i) -> int { return static_cast<int>((i + 1) * 10); }  // 10, 20, 30
        static auto MissingField()                 -> int { return 1; }                               // off the step-2 grid
        static auto MissingRecord()                -> int { return 99; }                              // out of the record set
    };

    /// @brief Range x range index, step == 1 (contiguous).
    struct RangeS1_IndexTest // NOLINT(readability-identifier-naming)
    {
        using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

        static constexpr std::size_t FLD_COUNT = TEST_FLD_COUNT;
        static constexpr std::size_t REC_COUNT = TEST_REC_COUNT;

        static auto Build() -> DF
        {
            DF df;
            df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 3});      // 0, 1, 2
            df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 3}, 0);  // 0, 1, 2
            return df;
        }

        static auto FieldKey(std::size_t const i)  -> int { return static_cast<int>(i); }       // 0, 1, 2
        static auto RecordKey(std::size_t const i) -> int { return static_cast<int>(i); }       // 0, 1, 2
        static auto MissingField()                 -> int { return 99; }                        // out of the field range
        static auto MissingRecord()                -> int { return 99; }                        // out of the record range
    };

    /// @brief Range x range index, step != 1 (strided records).
    struct RangeS2_IndexTest // NOLINT(readability-identifier-naming)
    {
        using DF = DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>;

        static constexpr std::size_t FLD_COUNT = TEST_FLD_COUNT;
        static constexpr std::size_t REC_COUNT = TEST_REC_COUNT;

        static auto Build() -> DF
        {
            DF df;
            df.SetFieldRange(DFRangeIndexBounds{.lower = 0, .upper = 6, .step = 2});     // 0, 2, 4
            df.SetRecordRange(DFRangeIndexBounds{.lower = 0, .upper = 6, .step = 2}, 0);  // 0, 2, 4
            return df;
        }

        static auto FieldKey(std::size_t const i)  -> int { return static_cast<int>(i) * 2; }   // 0, 2, 4
        static auto RecordKey(std::size_t const i) -> int { return static_cast<int>(i) * 2; }   // 0, 2, 4
        static auto MissingField()                 -> int { return 1; }                         // in range, off the step-2 grid
        static auto MissingRecord()                -> int { return 1; }                         // in range, off the step-2 grid
    };

    /// @brief Type list of index configurations for TYPED_TEST_SUITE.
    using IndexConfigs = testing::Types<Unique_IndexTest,
                                        UniqueRange_IndexTest,
                                        RangeUnique_IndexTest,
                                        RangeS1_IndexTest,
                                        RangeS2_IndexTest>;

} // namespace lugizmo::test

#endif // LUGIZMO_TEST_RM_DATAFRAME_TEST_CONFIGS_H
