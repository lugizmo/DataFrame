// Filename: DataFrame.cpp
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#include "lugizmo/Error.h"

#include <iostream>
#include <print>
#include <exception>

namespace lugizmo {

    [[noreturn]] __attribute__((weak))
    void CrashHandler(char const* msg, std::thread::id const threadId) noexcept
    {
        std::println(std::cerr, "[{}] Crash in lugizmo-lib: {}", threadId, msg);
        std::terminate();
    }

    [[noreturn]]
    void Crash(char const* msg, std::thread::id const threadId) noexcept
    {
        CrashHandler(msg, threadId);
    }

} // namespace mylib