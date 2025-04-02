// Filename: Error.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_ERROR_H
#define LUGIZMO_DF_ERROR_H

#include <thread>

namespace lugizmo {

/**
 * TODO doc
 * @param msg
 * @param threadId
 */
    [[noreturn]] void Crash(char const* msg, std::thread::id threadId = std::this_thread::get_id()) noexcept;

    /**
     * TODO doc
     * @param msg
     * @param threadId
     */
    [[noreturn]] void CrashHandler(char const* msg, std::thread::id threadId = std::this_thread::get_id()) noexcept;
}

#endif // LUGIZMO_DF_ERROR_H
