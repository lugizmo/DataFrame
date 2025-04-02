// Filename: IndexUniqueTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <unordered_set>
#include <format>
#include <functional>

#include "lugizmo/memory/OptionalRef.h"

TEST(lugizmo_memory_optional_ref, construct)
{
    using namespace lugizmo;

    int x = 5;
    OptionalRef const ref1(x);
    OptionalRef ref2(&x);
    constexpr OptionalRef<int> empty;

    EXPECT_TRUE(ref1.has_value());
    EXPECT_TRUE(ref2.has_value());
    EXPECT_FALSE(empty.has_value());
    EXPECT_TRUE(ref1.HasValue());
    EXPECT_TRUE(ref2.HasValue());
    EXPECT_FALSE(empty.HasValue());

    EXPECT_EQ(*ref1, 5);
    EXPECT_EQ(ref1.value(), 5);
    EXPECT_EQ(ref1.Value(), 5);

    int x2 = 10;
    ref2 = &x2;
    EXPECT_EQ(ref1.value(), 5);
    EXPECT_EQ(ref2.value(), 10);
}

TEST(lugizmo_memory_optional_ref, value_or_and_reset)
{
    using namespace lugizmo;

    int x = 1;
    OptionalRef ref(x);
    constexpr OptionalRef<int> empty;

    EXPECT_EQ(ref.value_or(100), 1);
    EXPECT_EQ(ref.ValueOr(100), 1);
    EXPECT_EQ(empty.value_or(100), 100);
    EXPECT_EQ(empty.ValueOr(100), 100);

    ref.reset();
    EXPECT_FALSE(ref.has_value());
    ref.Emplace(x);
    EXPECT_TRUE(ref.HasValue());
    ref.Reset();
    EXPECT_FALSE(ref.HasValue());
}

TEST(lugizmo_memory_optional_ref, emplace_and_swap)
{
    using namespace lugizmo;

    int a = 3, b = 7;
    OptionalRef<int> ref1(a), ref2(b);

    ref1.emplace(b);
    EXPECT_EQ(ref1.Value(), 7);
    ref1.Emplace(&a);
    EXPECT_EQ(ref1.Value(), 3);

    ref1.swap(ref2);
    EXPECT_EQ(*ref1, 7);
    EXPECT_EQ(*ref2, 3);
    ref1.Swap(ref2);
    EXPECT_EQ(*ref1, 3);
    EXPECT_EQ(*ref2, 7);
}

TEST(lugizmo_memory_optional_ref, comparisons)
{
    using namespace lugizmo;

    int a = 1, b = 1, c = 2;
    OptionalRef<int> const r1(a), r2(b), r3(c), empty;

    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 == r3);
    EXPECT_TRUE(r1 != r3);

    EXPECT_TRUE(r1 > empty);
    EXPECT_TRUE(empty < r3);
    EXPECT_EQ(r1 <=> r2, std::strong_ordering::equal);
}

TEST(lugizmo_memory_optional_ref, monadic)
{
    using namespace lugizmo;

    int val = 10, fallback = 99;
    OptionalRef opt(val);
    OptionalRef<int> none;

    auto const andThen = opt.and_then([](int& x) { return OptionalRef(x); });
    EXPECT_TRUE(andThen.has_value());
    EXPECT_EQ(*andThen, 10);

    auto const AndThen = opt.AndThen([](int& x) { return OptionalRef(x); });
    EXPECT_TRUE(AndThen.HasValue());
    EXPECT_EQ(*AndThen, 10);

    auto const transform = opt.transform([](int& x) -> int& { return x; });
    EXPECT_TRUE(transform.has_value());
    EXPECT_EQ(*transform, 10);

    auto const Transform = opt.Transform([](int& x) -> int& { return x; });
    EXPECT_TRUE(Transform.HasValue());
    EXPECT_EQ(*Transform, 10);

    auto const orElse = none.or_else([&] { return OptionalRef(fallback); });
    EXPECT_TRUE(orElse.has_value());
    EXPECT_EQ(*orElse, 99);

    auto const OrElse = none.OrElse([&] { return OptionalRef(fallback); });
    EXPECT_TRUE(OrElse.HasValue());
    EXPECT_EQ(*OrElse, 99);
}

TEST(lugizmo_memory_optional_ref, std_swap_support)
{
    using namespace lugizmo;

    int a = 3, b = 9;
    OptionalRef<int> r1(a), r2(b);

    std::swap(r1, r2);
    EXPECT_EQ(*r1, 9);
    EXPECT_EQ(*r2, 3);
}

TEST(lugizmo_memory_optional_ref, hash_support)
{
    using namespace lugizmo;

    int x = 42, y = 43;
    OptionalRef const ref1(x);
    OptionalRef const ref2(y);
    constexpr OptionalRef<int> none;

    std::unordered_set<size_t> hashes;
    hashes.insert(std::hash<OptionalRef<int>>{}(ref1));
    hashes.insert(std::hash<OptionalRef<int>>{}(ref2));
    hashes.insert(std::hash<OptionalRef<int>>{}(none));

    EXPECT_EQ(hashes.size(), 3);
}

TEST(lugizmo_memory_optional_ref, formatter_support)
{
    using namespace lugizmo;

    int val = 123;
    OptionalRef ref(val);
    OptionalRef<int> none;

    auto const formatted = std::format("ref = {}, none = {}", ref, none);
    EXPECT_EQ(formatted, "ref = 123, none = null");
}

TEST(lugizmo_memory_optional_ref, is_optional_ref)
{
    using namespace lugizmo;

    static_assert(OptionalRefType<OptionalRef<int>>);

    static_assert(not OptionalRefType<std::optional<int>>);
    static_assert(not OptionalRefType<std::optional<float>>);

    static_assert(!OptionalRefType<int>);
    static_assert(!OptionalRefType<float*>);
    static_assert(!OptionalRefType<std::nullopt_t>);
    static_assert(!OptionalRefType<std::optional<std::optional<int>>>);
}