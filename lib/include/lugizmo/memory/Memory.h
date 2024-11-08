// Filename: Memory.h
// Copyright 2024 Lukas Guz
// Licensed under the Apache License, Version 2.0.
// See the LICENSE file in the project root or at
// http://www.apache.org/licenses/LICENSE-2.0 for full license information.

#ifndef LUGIZMO_DF_MEMORY_H
#define LUGIZMO_DF_MEMORY_H

#include <memory>
#include <memory_resource>

namespace lugizmo {

    /// @brief TODO doc
    ///
    inline auto BackingResDefault() -> std::shared_ptr<std::pmr::memory_resource>
    {
        // create a non owning reference to the global default resource
        static auto defaultResource = std::shared_ptr<std::pmr::memory_resource>(
            std::pmr::get_default_resource(), [](std::pmr::memory_resource*) {});

        return defaultResource;
    }
}

#endif // LUGIZMO_DF_MEMORY_H
