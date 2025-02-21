// Filename: LayoutColMajor.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_LAYOUT_COL_MAJOR_H
#define LUGIZMO_DF_LAYOUT_COL_MAJOR_H

namespace lugizmo {

    struct DFColMajor final
    {
        static constexpr bool IsRowMajor = false;
        static constexpr bool IsColMajor = true;
    };
}

#endif // LUGIZMO_DF_LAYOUT_COL_MAJOR_H
