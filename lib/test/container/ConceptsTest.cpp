// Filename: ConceptsTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the container concepts.
// The following concepts are tested here (with names of tests):
//
// ✅ Iterable             - Iterable<T>
// ✅ IterableOfIterables  - IterableOfIterables<T>
// ✅ IterableOfIterable   - IterableOfIterable<T>
// ✅ ComparableType       - ComparableType<T1, T2>
// ✅ OptionalType         - OptionalType<T>
// ✅ RemovedOptional      - RemovedOptional<T>
//

#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "lugizmo/container/Concepts.h"

using namespace lgz;

/**
 *  @brief Iterable accepts ranges and rejects scalars / raw arrays.
 *  @see   lgz::Iterable<T>
 */
TEST(ContainerConcepts, Iterable)
{
    static_assert(not Iterable<int>);
    static_assert(not Iterable<int[]>);

    static_assert(Iterable<std::vector<int>>);
    static_assert(Iterable<std::array<int, 10>>);
    static_assert(Iterable<std::string>);

    SUCCEED();
}

/**
 *  @brief IterableOfIterables accepts (recursively) nested ranges.
 *  @see   lgz::IterableOfIterables<T>
 */
TEST(ContainerConcepts, IterableOfIterables)
{
    static_assert(not IterableOfIterables<int>);
    static_assert(not IterableOfIterables<float>);

    static_assert(IterableOfIterables<std::string>);
    static_assert(IterableOfIterables<std::vector<std::vector<int>>>);
    static_assert(IterableOfIterables<std::vector<std::vector<std::vector<std::vector<int>>>>>);
    static_assert(IterableOfIterables<std::array<std::array<int, 100>, 2>>);
    static_assert(IterableOfIterables<std::vector<std::vector<std::array<std::vector<int>, 10>>>>);

    SUCCEED();
}

/**
 *  @brief IterableOfIterable accepts a range whose elements are themselves ranges (not strings).
 *  @see   lgz::IterableOfIterable<T>
 */
TEST(ContainerConcepts, IterableOfIterable)
{
    static_assert(not IterableOfIterable<std::string>);

    static_assert(IterableOfIterable<std::array<std::array<int, 1>, 1>>);
    static_assert(IterableOfIterable<std::vector<std::vector<int>>>);

    SUCCEED();
}

/**
 *  @brief ComparableType holds for mutually comparable types.
 *  @see   lgz::ComparableType<T1, T2>
 */
TEST(ContainerConcepts, ComparableType)
{
    static_assert(ComparableType<int, int>);
    static_assert(ComparableType<int, float>);
    static_assert(not ComparableType<int, std::string>);
    static_assert(not ComparableType<std::string, int>);

    SUCCEED();
}

/**
 *  @brief OptionalType detects std::optional (including nested).
 *  @see   lgz::OptionalType<T>
 */
TEST(ContainerConcepts, OptionalType)
{
    using T    = int;
    using Op   = std::optional<T>;
    using OpOp = std::optional<Op>;

    static_assert(not OptionalType<T>);
    static_assert(OptionalType<Op>);
    static_assert(OptionalType<OpOp>);

    SUCCEED();
}

/**
 *  @brief RemovedOptional strips one std::optional layer (and is identity for non-optionals).
 *  @see   lgz::RemovedOptional<T>
 */
TEST(ContainerConcepts, RemovedOptional)
{
    using T    = int;
    using Op   = std::optional<T>;
    using OpOp = std::optional<Op>;

    static_assert(std::is_same_v<RemovedOptional<Op>, T>);
    static_assert(std::is_same_v<RemovedOptional<T>, T>);
    static_assert(std::is_same_v<RemovedOptional<OpOp>, Op>);

    SUCCEED();
}

// TODO NormalizedOptional (flatten nested optionals) once implemented:
// TEST(ContainerConcepts, NormalizedOptional)
// {
//     static_assert(std::is_same_v<NormalizedOptional<int>, std::optional<int>>);
//     static_assert(std::is_same_v<NormalizedOptional<std::optional<int>>, std::optional<int>>);
//     static_assert(std::is_same_v<NormalizedOptional<std::optional<std::optional<int>>>, std::optional<int>>);
// }
