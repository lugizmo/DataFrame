// Filename: RM_Dataframe_TestData.h
// Copyright 2025 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_TEST_TESTDATA_H
#define LUGIZMO_DF_TEST_TESTDATA_H

#include <cstddef>
#include <array>
#include <string>

#include "lugizmo/DataFrame.h"

namespace lugizmo::test::integer {

    inline constexpr size_t DFFldCount = 5;
    inline constexpr size_t DFRecCount = 10;

    inline constexpr std::array DFFields  = {0, 1, 2, 3, 4};
    inline constexpr std::array DFRecords = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    inline constexpr std::array DFData = {
            std::array{0,  1,  2,  3,  4 },
            std::array{5,  6,  7,  8,  9 },
            std::array{10, 11, 12, 13, 14},
            std::array{15, 16, 17, 18, 19},
            std::array{20, 21, 22, 23, 24},
            std::array{25, 26, 27, 28, 29},
            std::array{30, 31, 32, 33, 34},
            std::array{35, 36, 37, 38, 39},
            std::array{40, 41, 42, 43, 44},
            std::array{45, 46, 47, 48, 49}
    };

    inline auto DefaultDataframe() -> DataFrame<int, int, int>
    {
        return DataFrame<int, int, int>::FromFieldsAndRecords(DFFields, DFRecords, DFData);
    }
}

namespace lugizmo::test::str {

    inline constexpr size_t DFFldCount = 5;
    inline constexpr size_t DFRecCount = 10;

    inline constexpr std::array<std::string, DFFldCount> DFFields  = {"0", "1", "2", "3", "4"};
    inline constexpr std::array<std::string, DFRecCount> DFRecords = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

    inline constexpr auto const& DFData = integer::DFData;

    inline auto DefaultDataframe() -> DataFrame<int, std::string, std::string>
    {
        return DataFrame<int, std::string, std::string>::FromFieldsAndRecords(DFFields, DFRecords, DFData);
    }
}

namespace lugizmo::test::sequence {

    inline constexpr size_t DFFldCount = 5;
    inline constexpr size_t DFRecCount = 10;

    inline constexpr auto DFFields  = DFRangeIndexBounds{.lower = -2, .upper = 3};
    inline constexpr auto DFRecords = DFRangeIndexBounds{.lower = -5, .upper = 5};

    static_assert(DFFields.Size()  == DFFldCount);
    static_assert(DFRecords.Size() == DFRecCount);

    inline constexpr std::array DFData = {
        //         -2 -1   0   1   2
        std::array{0,  1,  2,  3,  4 }, // -5
        std::array{5,  6,  7,  8,  9 }, // -4
        std::array{10, 11, 12, 13, 14}, // -3
        std::array{15, 16, 17, 18, 19}, // -2
        std::array{20, 21, 22, 23, 24}, // -1
        std::array{25, 26, 27, 28, 29}, //  0
        std::array{30, 31, 32, 33, 34}, //  1
        std::array{35, 36, 37, 38, 39}, //  2
        std::array{40, 41, 42, 43, 44}, //  3
        std::array{45, 46, 47, 48, 49}  //  4
    };

    inline auto DefaultDataframe() -> DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>
    {
        return DataFrame<int, DFRangeIndex<int>, DFRangeIndex<int>>::FromFieldsAndRecords(DFFields, DFRecords, DFData);
    }
}

#endif // LUGIZMO_DF_TEST_TESTDATA_H
