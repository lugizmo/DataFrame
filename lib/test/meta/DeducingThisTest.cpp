// Filename: DeducingThisTest.cpp
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

#include <functional>
#include <optional>
#include <span>
#include <type_traits>

#include "lugizmo/meta/DeducingThis.h"

TEST(lugizmo_meta_deducing_this_test, is_const_this)
{
    using namespace lugizmo::meta;

    constexpr auto mut    = IsConstThis<int&>();
    constexpr auto cst    = IsConstThis<int const&>();
    constexpr auto cstRv  = IsConstThis<int const&&>();
    constexpr auto noRef  = IsConstThis<int>();
    constexpr auto noRefC = IsConstThis<int const>();

    ASSERT_FALSE(mut);
    ASSERT_TRUE(cst);
    ASSERT_TRUE(cstRv);
    ASSERT_FALSE(noRef);
    ASSERT_TRUE(noRefC);
}

TEST(lugizmo_meta_deducing_this_test, value_and_reference_type_aliases)
{
    using namespace lugizmo::meta;

    constexpr auto mutValue    = std::is_same_v<ThisValueT<int&, long>, long>;
    constexpr auto constValue  = std::is_same_v<ThisValueT<int const&, long>, long const>;
    constexpr auto mutValueRef = std::is_same_v<ThisValueRefT<int&, long>, long&>;
    constexpr auto cstValueRef = std::is_same_v<ThisValueRefT<int const&, long>, long const&>;

    ASSERT_TRUE(mutValue);
    ASSERT_TRUE(constValue);
    ASSERT_TRUE(mutValueRef);
    ASSERT_TRUE(cstValueRef);
}

TEST(lugizmo_meta_deducing_this_test, span_and_reference_wrapper_aliases)
{
    using namespace lugizmo::meta;

    constexpr auto mutSpan = std::is_same_v<ThisValueSpanT<int&, int>, std::span<int>>;
    constexpr auto cstSpan = std::is_same_v<ThisValueSpanT<int const&, int>, std::span<int const>>;
    constexpr auto mutWrap = std::is_same_v<ThisRefWrapperT<int&, int>, std::reference_wrapper<int>>;
    constexpr auto cstWrap = std::is_same_v<ThisRefWrapperT<int const&, int>, std::reference_wrapper<int const>>;
    constexpr auto mutOpt  = std::is_same_v<ThisRefWrapperOptT<int&, int>, std::optional<std::reference_wrapper<int>>>;
    constexpr auto cstOpt  = std::is_same_v<ThisRefWrapperOptT<int const&, int>, std::optional<std::reference_wrapper<int const>>>;

    ASSERT_TRUE(mutSpan);
    ASSERT_TRUE(cstSpan);
    ASSERT_TRUE(mutWrap);
    ASSERT_TRUE(cstWrap);
    ASSERT_TRUE(mutOpt);
    ASSERT_TRUE(cstOpt);
}
