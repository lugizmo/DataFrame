# Filename: CMakeLists.txt
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

# ====== SOURCES =======================================================================================================

set(LUGIZMO_DF_PUBLIC_HEADERS
        include/lugizmo/Assert.h
        include/lugizmo/DataFrame.h
        include/lugizmo/container/Concepts.h
        include/lugizmo/container/DataFrameMap.h
        include/lugizmo/container/DataFrameMapSupport.h
        include/lugizmo/dataframe/IndexBase.h
        include/lugizmo/dataframe/IndexRange.h
        include/lugizmo/dataframe/IndexUnique.h
        include/lugizmo/dataframe/LayoutColMajor.h
        include/lugizmo/dataframe/LayoutRowMajor.h
        include/lugizmo/dataframe/Selector.h
        include/lugizmo/dataframe/Span.h
        include/lugizmo/dataframe/View.h
        include/lugizmo/dataframe/ViewIndexed.h
        include/lugizmo/memory/Memory.h
        include/lugizmo/support/Compiler.h
        include/lugizmo/meta/DeducingThis.h
)

# ====== TARGET ========================================================================================================

add_library(${LUGIZMO_DF_TARGET_NAME} INTERFACE)
add_library(${LUGIZMO_DF_NAMESPACE}DataFrame ALIAS ${LUGIZMO_DF_TARGET_NAME})

target_sources(${LUGIZMO_DF_TARGET_NAME}
        INTERFACE
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES ${LUGIZMO_DF_PUBLIC_HEADERS}
)

target_include_directories(${LUGIZMO_DF_TARGET_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(${LUGIZMO_DF_TARGET_NAME}
        INTERFACE
        cxx_std_${LUGIZMO_DF_CXX_STANDARD}
)

target_compile_definitions(${LUGIZMO_DF_TARGET_NAME}
        INTERFACE
        LUGIZMO_DF_ENABLE_ASSERT=$<IF:$<BOOL:${LUGIZMO_DF_ENABLE_ASSERT}>,1,0>
        LUGIZMO_DF_ENABLE_ASSERT_TRACE=$<IF:$<BOOL:${LUGIZMO_DF_ENABLE_ASSERT_TRACE}>,1,0>
)

set_target_properties(${LUGIZMO_DF_TARGET_NAME} PROPERTIES
        EXPORT_NAME DataFrame
)

# ====== INSTALL =======================================================================================================

install(
        TARGETS ${LUGIZMO_DF_TARGET_NAME}
        EXPORT LuDataFrameTargets
        FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
