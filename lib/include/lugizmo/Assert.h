// Filename: Assert.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_ASSERT_H
#define LUGIZMO_DF_ASSERT_H

#include <cstdio>
#include <cstdlib>
#include <source_location>

namespace lugizmo {

    namespace internal {

        [[noreturn]]
        inline void PrintAndAbortAssertFailure(char const* message,
                                               std::source_location const location) noexcept
        {
            std::fprintf(stderr,
                         "[lugizmo] Assertion failed%s%s\n"
                         "  file: %s:%lu:%lu\n"
                         "  func: %s\n",
                         message != nullptr ? ": " : "",
                         message != nullptr ? message : "",
                         location.file_name(),
                         static_cast<unsigned long>(location.line()),
                         static_cast<unsigned long>(location.column()),
                         location.function_name());
            std::fflush(stderr);
            std::abort();
        }

    } // namespace internal

    /**
     * @brief Indicates if default asserts are enabled for this build.
     */
    [[nodiscard]]
    consteval auto AssertEnabled() noexcept -> bool
    {
#if defined(LUGIZMO_DF_ENABLE_ASSERT) && LUGIZMO_DF_ENABLE_ASSERT
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Indicates if trace/internal asserts are enabled for this build.
     */
    [[nodiscard]]
    consteval auto AssertTraceEnabled() noexcept -> bool
    {
#if defined(LUGIZMO_DF_ENABLE_ASSERT_TRACE) && LUGIZMO_DF_ENABLE_ASSERT_TRACE
        return true;
#else
        return false;
#endif
    }

} // namespace lugizmo

#if defined(LUGIZMO_DF_ENABLE_ASSERT) && LUGIZMO_DF_ENABLE_ASSERT
#define LUGIZMO_ASSERT(condition, message)                                                                     \
    [&]() noexcept                                                                                             \
    {                                                                                                          \
        if(!(condition))                                                                                       \
        {                                                                                                      \
            ::lugizmo::internal::PrintAndAbortAssertFailure((message), std::source_location::current());       \
        }                                                                                                      \
    }()
#else
#define LUGIZMO_ASSERT(condition, message)                                                                     \
    []() noexcept                                                                                              \
    {                                                                                                          \
    }()
#endif

#if defined(LUGIZMO_DF_ENABLE_ASSERT_TRACE) && LUGIZMO_DF_ENABLE_ASSERT_TRACE
#define LUGIZMO_ASSERT_TRACE(condition, message)                                                               \
    [&]() noexcept                                                                                             \
    {                                                                                                          \
        if(!(condition))                                                                                       \
        {                                                                                                      \
            ::lugizmo::internal::PrintAndAbortAssertFailure((message), std::source_location::current());       \
        }                                                                                                      \
    }()
#else
#define LUGIZMO_ASSERT_TRACE(condition, message)                                                               \
    []() noexcept                                                                                              \
    {                                                                                                          \
    }()
#endif

#endif // LUGIZMO_DF_ASSERT_H
