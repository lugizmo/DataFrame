// Filename: main.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "gtest/gtest.h"

auto main(int argc, char** argv) -> int
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
