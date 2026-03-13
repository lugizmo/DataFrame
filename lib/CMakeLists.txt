# Filename: CMakeLists.txt
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

set(LUGIZMO_DF_INCLUDES
        include/lugizmo/memory/Memory.h
#        include/lugizmo/memory/DFMemory.h

        include/lugizmo/container/Concepts.h
        include/lugizmo/container/DataFrameMap.h
        include/lugizmo/container/OptionalRef.h

        include/lugizmo/meta/DeducingThis.h

        include/lugizmo/Assert.h
        include/lugizmo/dataframe/LayoutColMajor.h
        include/lugizmo/dataframe/LayoutRowMajor.h
        include/lugizmo/dataframe/IndexBase.h
        include/lugizmo/dataframe/IndexUnique.h
        include/lugizmo/dataframe/IndexRange.h
        include/lugizmo/dataframe/Selector.h
        include/lugizmo/dataframe/View.h
        include/lugizmo/dataframe/ViewIndexed.h

        include/lugizmo/Error.h
        include/lugizmo/DataFrame.h
)

set(LUGIZMO_DF_SOURCES
        src/Error.cpp
)

add_library(DataFrame ${LUGIZMO_DF_SOURCES})
target_sources(DataFrame PUBLIC ${LUGIZMO_DF_INCLUDES})
target_include_directories(DataFrame PUBLIC include/)
target_compile_definitions(DataFrame PUBLIC
        LUGIZMO_DF_ENABLE_ASSERT=$<IF:$<BOOL:${LUGIZMO_DF_ENABLE_ASSERT}>,1,0>
        LUGIZMO_DF_ENABLE_ASSERT_TRACE=$<IF:$<BOOL:${LUGIZMO_DF_ENABLE_ASSERT_TRACE}>,1,0>
)

target_compile_options(DataFrame PRIVATE ${LUGIZMO_CXX_FLAGS})
target_link_options(DataFrame PRIVATE ${LUGIZMO_LINK_FLAGS})

# TODO add include what you use
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
   CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(DataFrame PRIVATE -fno-exceptions)
else()
    message(FATAL_ERROR "Unsupported compiler - Only Clang and GCC are supported!")
endif()

set_target_properties(DataFrame PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON)

#target_include_interface_directories(DataFrame "${CMAKE_CURRENT_SOURCE_DIR}/include/")
