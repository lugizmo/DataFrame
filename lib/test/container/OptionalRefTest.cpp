// Filename: OptionalRefTest.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the OptionalRef container (a lightweight optional reference / pointer wrapper).
// The following functions are tested here (with names of tests):
//
// ✅ EmptyAndReset        - HasValue / SetReference / Reset / operator==(nullopt)
// ✅ MutateThroughRef     - Value / operator* / Pointer
// ✅ ValueOrAndConversion - ValueOr / conversion to OptionalRef<T const>
// ✅ PointerStyleUsage    - operator->
//

#include "gtest/gtest.h"

#include <optional>
#include <string>
#include <type_traits>

#include "lugizmo/container/OptionalRef.h"

using namespace lugizmo;

static_assert(std::is_trivially_copyable_v<OptionalRef<int>>, "OptionalRef should stay a lightweight pointer wrapper.");

/**
 *  @brief A default OptionalRef is empty; SetReference fills it and Reset clears it.
 *  @see   lugizmo::OptionalRef::HasValue / SetReference / Reset
 */
TEST(ContainerOptionalRef, EmptyAndReset)
{
    auto ref = OptionalRef<int>();

    {
        // default-constructed -> empty, compares equal to nullopt
        EXPECT_FALSE(ref.HasValue());
        EXPECT_TRUE(ref == std::nullopt);
    }

    {
        // SetReference binds a referent
        int val = 42;
        ref.SetReference(val);
        EXPECT_TRUE(ref.HasValue());
        EXPECT_EQ(*ref, 42);
    }

    {
        // Reset clears it again
        ref.Reset();
        EXPECT_FALSE(ref.HasValue());
        EXPECT_TRUE(std::nullopt == ref);
    }
}

/**
 *  @brief Value / operator* / Pointer all mutate the referent.
 *  @see   lugizmo::OptionalRef::Value / operator* / Pointer
 */
TEST(ContainerOptionalRef, MutateThroughRef)
{
    int v    = 5;
    auto ref = OptionalRef(v);
    ASSERT_TRUE(ref.HasValue());

    {
        // Value() yields the referent
        ref.Value() = 9;
        EXPECT_EQ(v, 9);
    }

    {
        // operator* writes through
        *ref = 11;
        EXPECT_EQ(v, 11);
    }

    {
        // Pointer() points at the referent
        EXPECT_EQ(*ref.Pointer(), 11);
    }
}

/**
 *  @brief ValueOr returns a fallback when empty; OptionalRef<T const> binds a const view.
 *  @see   lugizmo::OptionalRef::ValueOr
 */
TEST(ContainerOptionalRef, ValueOrAndConversion)
{
    int value    = 1;
    int fallback = 2;

    auto filled = OptionalRef(value);
    auto empty  = OptionalRef<int>();

    {
        // ValueOr: held value vs fallback
        EXPECT_EQ(filled.ValueOr(fallback), 1);
        EXPECT_EQ(empty.ValueOr(fallback), 2);
    }

    {
        // a const view yields const access to the same referent
        auto constRef = OptionalRef<int const>(filled);
        static_assert(std::is_same_v<decltype(constRef.Value()), int const&>);
        ASSERT_TRUE(constRef.HasValue());
        EXPECT_EQ(constRef.Value(), 1);
    }
}

/**
 *  @brief operator-> forwards to the referent's members.
 *  @see   lugizmo::OptionalRef::operator->
 */
TEST(ContainerOptionalRef, PointerStyleUsage)
{
    auto text = std::string("abc");
    auto ref  = OptionalRef(text);

    EXPECT_EQ(ref->size(), 3U);
    ref->push_back('d');
    EXPECT_EQ(text, "abcd");
}
