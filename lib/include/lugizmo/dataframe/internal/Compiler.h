// Filename: Compiler.h
// Copyright 2026 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_COMPILER_H
#define LUGIZMO_DF_COMPILER_H

#if defined(__clang__) || defined(__GNUC__)
#define LUGIZMO_RETURNS_NONNULL [[gnu::returns_nonnull]]
#elif defined(_MSC_VER)
#include <sal.h>
#define LUGIZMO_RETURNS_NONNULL _Ret_notnull_
#else
#define LUGIZMO_RETURNS_NONNULL
#endif

#endif // LUGIZMO_DF_COMPILER_H
