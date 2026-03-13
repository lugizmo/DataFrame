// Filename: OptionalRefTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <optional>
#include <string>
#include <type_traits>

#include "lugizmo/container/OptionalRef.h"

static_assert(std::is_trivially_copyable_v<lugizmo::OptionalRef<int>>, "OptionalRef should stay a lightweight pointer wrapper.");

TEST(lugizmo_container_optional_ref_test, empty_and_reset)
{
    using namespace lugizmo;

    auto ref = OptionalRef<int>();
    ASSERT_FALSE(ref.HasValue());
    ASSERT_TRUE(ref == std::nullopt);

    int val = 42;
    ref.SetReference(val);
    ASSERT_TRUE(ref.HasValue());
    ASSERT_EQ(*ref, 42);

    ref.Reset();
    ASSERT_FALSE(ref.HasValue());
    ASSERT_TRUE(std::nullopt == ref);
}

TEST(lugizmo_container_optional_ref_test, mutate_through_reference)
{
    using namespace lugizmo;

    int v = 5;
    auto ref = OptionalRef(v);

    ASSERT_TRUE(ref.HasValue());
    ref.Value() = 9;
    ASSERT_EQ(v, 9);

    *ref = 11;
    ASSERT_EQ(v, 11);

    ASSERT_EQ(*ref.Pointer(), 11);
}

TEST(lugizmo_container_optional_ref_test, value_or_and_conversion)
{
    using namespace lugizmo;

    int value = 1;
    int fallback = 2;

    auto filled = OptionalRef(value);
    auto empty  = OptionalRef<int>();

    ASSERT_EQ(filled.ValueOr(fallback), 1);
    ASSERT_EQ(empty.ValueOr(fallback), 2);

    auto constRef = OptionalRef<int const>(filled);
    static_assert(std::is_same_v<decltype(constRef.Value()), int const&>);
    ASSERT_TRUE(constRef.HasValue());
    ASSERT_EQ(constRef.Value(), 1);
}

TEST(lugizmo_container_optional_ref_test, pointer_style_usage)
{
    using namespace lugizmo;

    auto text = std::string("abc");
    auto ref = OptionalRef(text);

    ASSERT_EQ(ref->size(), 3U);
    ref->push_back('d');
    ASSERT_EQ(text, "abcd");
}
