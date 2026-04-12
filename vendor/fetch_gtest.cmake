# Filename: fetch_gtest.cmake
# Copyright 2024-2026 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

if(NOT LUGIZMO_DF_BUILD_TESTING)
    return()
endif()

# ====== SETTINGS ======================================================================================================

set(LUGIZMO_DF_GTEST_MINIMUM_VERSION "1.15")
set(LUGIZMO_DF_GTEST_FETCH_VERSION   "v1.15.2")

# ====== PROVIDED TARGETS ==============================================================================================

if(TARGET GTest::gtest)
    message(STATUS "Lugizmo DF - Using provided GoogleTest targets.")
    return()
endif()

if(TARGET gtest)
    add_library(GTest::gtest ALIAS gtest)

    if(TARGET gtest_main)
        add_library(GTest::gtest_main ALIAS gtest_main)
    endif()

    message(STATUS "Lugizmo DF - Using provided GoogleTest targets.")
    return()
endif()

# ====== LOCAL PACKAGE =================================================================================================

if(NOT LUGIZMO_DF_DOWNLOAD_GTEST)
    message(STATUS "Lugizmo DF - Checking for installed GoogleTest...")
    find_package(GTest ${LUGIZMO_DF_GTEST_MINIMUM_VERSION} QUIET)
endif()

if(TARGET GTest::gtest)
    message(STATUS "Lugizmo DF - Using installed GoogleTest ${GTest_VERSION}.")
    return()
endif()

# ====== FETCH CONTENT ================================================================================================

if(NOT LUGIZMO_DF_DOWNLOAD_GTEST)
    message(FATAL_ERROR
            "Lugizmo DF - GoogleTest not found. "
            "Provide GTest::gtest from a parent project, install GoogleTest >= "
            "${LUGIZMO_DF_GTEST_MINIMUM_VERSION}, or enable LUGIZMO_DF_DOWNLOAD_GTEST.")
endif()

message(STATUS "Lugizmo DF - Fetching GoogleTest...")
include(FetchContent)

set(BUILD_GMOCK OFF CACHE BOOL "Build GoogleMock." FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable GoogleTest install rules." FORCE)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG ${LUGIZMO_DF_GTEST_FETCH_VERSION}
)

FetchContent_MakeAvailable(googletest)
