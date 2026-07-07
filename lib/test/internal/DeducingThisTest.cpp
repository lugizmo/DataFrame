// Filename: DeducingThisTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

//
// Test the deducing-this meta helpers.
// The following helpers are tested here (with names of tests):
//
// ✅ IsConstThis        - IsConstThis<Self>
// ✅ ValueAliases       - ThisValueT / ThisValueRefT
// ✅ SpanAndWrapAliases - ThisValueSpanT / ThisRefWrapperT / ThisRefWrapperOptT
//

#include "gtest/gtest.h"

#include <functional>
#include <optional>
#include <span>
#include <type_traits>

#include "lugizmo/dataframe/internal/DeducingThis.h"

using namespace lgz::meta;

/**
 *  @brief IsConstThis reports constness of a deduced `this` type.
 *  @see   lgz::meta::IsConstThis<Self>
 */
TEST(MetaDeducingThis, IsConstThis)
{
    static_assert(not IsConstThis<int&>());
    static_assert(IsConstThis<int const&>());
    static_assert(IsConstThis<int const&&>());
    static_assert(not IsConstThis<int>());
    static_assert(IsConstThis<int const>());

    SUCCEED();
}

/**
 *  @brief ThisValueT / ThisValueRefT carry constness from the deduced `this` to a value type.
 *  @see   lgz::meta::ThisValueT / ThisValueRefT
 */
TEST(MetaDeducingThis, ValueAliases)
{
    static_assert(std::is_same_v<ThisValueT<int&, long>, long>);
    static_assert(std::is_same_v<ThisValueT<int const&, long>, long const>);
    static_assert(std::is_same_v<ThisValueRefT<int&, long>, long&>);
    static_assert(std::is_same_v<ThisValueRefT<int const&, long>, long const&>);

    SUCCEED();
}

/**
 *  @brief ThisValueSpanT / ThisRefWrapperT / ThisRefWrapperOptT carry constness into span/wrappers.
 *  @see   lgz::meta::ThisValueSpanT / ThisRefWrapperT / ThisRefWrapperOptT
 */
TEST(MetaDeducingThis, SpanAndWrapAliases)
{
    static_assert(std::is_same_v<ThisValueSpanT<int&, int>, std::span<int>>);
    static_assert(std::is_same_v<ThisValueSpanT<int const&, int>, std::span<int const>>);
    static_assert(std::is_same_v<ThisRefWrapperT<int&, int>, std::reference_wrapper<int>>);
    static_assert(std::is_same_v<ThisRefWrapperT<int const&, int>, std::reference_wrapper<int const>>);
    static_assert(std::is_same_v<ThisRefWrapperOptT<int&, int>, std::optional<std::reference_wrapper<int>>>);
    static_assert(std::is_same_v<ThisRefWrapperOptT<int const&, int>, std::optional<std::reference_wrapper<int const>>>);

    SUCCEED();
}
