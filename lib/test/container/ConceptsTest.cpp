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

TEST(lugizmo_container_concepts_test, comparable)
{
    using namespace lugizmo;
    static_assert(ComparableType<int, int>);
    static_assert(ComparableType<int, float>);
    static_assert(not ComparableType<int, std::string>);
    static_assert(not ComparableType<std::string, int>);
}

TEST(lugizmo_container_concepts_test, is_optional)
{
    using namespace lugizmo;

    using T    = int;
    using Op   = std::optional<T>;
    using OpOp = std::optional<Op>;

    static_assert(not OptionalType<T>);
    static_assert(OptionalType<Op>);
    static_assert(OptionalType<OpOp>);
}

TEST(lugizmo_container_concepts_test, removed_optional)
{
    using namespace lugizmo;

    using T    = int;
    using Op   = std::optional<T>;
    using OpOp = std::optional<Op>;

    static_assert(std::is_same_v<RemovedOptional<Op>, T>);
    static_assert(std::is_same_v<RemovedOptional<T>, T>);
    static_assert(std::is_same_v<RemovedOptional<OpOp>, Op>);
}

//TEST(lugizmo_container_concepts_test, normalized_optional)
//{
//    using namespace lugizmo;
//
//    using NoOp = int;
//    static_assert(std::is_same_v<NormalizedOptional<NoOp>, std::optional<int>>);
//
//    using Op = std::optional<int>;
//    static_assert(std::is_same_v<NormalizedOptional<Op>, std::optional<int>>);
//
//    using OpOp = std::optional<std::optional<int>>;
//    static_assert(std::is_same_v<NormalizedOptional<OpOp>, std::optional<int>>);
//
//    using OpOpOp = std::optional<std::optional<std::optional<int>>>;
//    static_assert(std::is_same_v<NormalizedOptional<OpOpOp>, std::optional<int>>);
//}