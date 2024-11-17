#include "gtest/gtest.h"

#include <vector>
#include <array>
#include <string>

#include "lugizmo/container/Concepts.h"

TEST(lugizmo_container_concepts_test, some_types_check)
{
    using namespace lugizmo;

    static_assert(not Iterable<int>);
    static_assert(not Iterable<int[]>);

    // single iterable
    static_assert(Iterable<std::vector<int>>);
    static_assert(Iterable<std::array<int, 10>>);
    static_assert(Iterable<std::string>);

    // iterable of iterables ...
    static_assert(not IterableOfIterables<int>);
    static_assert(not IterableOfIterables<float>);

    static_assert(IterableOfIterables<std::string>);
    static_assert(IterableOfIterables<std::vector<std::vector<int>>>);
    static_assert(IterableOfIterables<std::vector<std::vector<std::vector<std::vector<int>>>>>);
    static_assert(IterableOfIterables<std::array<std::array<int, 100>, 2>>);
    static_assert(IterableOfIterables<std::vector<std::vector<std::array<std::vector<int>, 10>>>>);

    // iterable of iterable specialization
    static_assert(not IterableOfIterable<std::string>);

    static_assert(IterableOfIterable<std::array<std::array<int, 1>, 1>>);
    static_assert(IterableOfIterable<std::vector<std::vector<int>>>);
}