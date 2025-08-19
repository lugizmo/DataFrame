# Filename: fetch_gtest.cmake
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

include_guard(GLOBAL)

if(LUGIZMO_BUILD_TESTING)
# Build Tests

    set(LUGIZMO_GTEST_MINIMUM 1.15)
    set(LUGIZMO_GTEST_VERSION v1.16.0)

    if(NOT TARGET gtest AND NOT TARGET gtest_main)
        # Check if dependencies are present

        if(NOT LUGIZMO_DOWNLOAD_GTEST)
            # Find local package
            message(STATUS "Lugizmo DF - Checking for system-installed GoogleTest...")
            find_package(GTest ${LUGIZMO_GTEST_MINIMUM}... EXACT QUIET)
        endif()

        if(GTest_FOUND AND NOT LUGIZMO_DOWNLOAD_GTEST)
            # Found local package
            message(STATUS "Lugizmo DF - GoogleTest ${GTest_VERSION}")
        elseif(NOT GTest_FOUND AND NOT LUGIZMO_DOWNLOAD_GTEST)
            # Couldn't find local package even though should be provided
            message(FATAL_ERROR "Lugizmo DF - GoogleTest not found! Please install GoogleTest >= ${LUGIZMO_GTEST_MINIMUM} or enable LUGIZMO_DOWNLOAD_GTEST to fetch it.")
        else()
            # Get gtest lib from remote
            message(STATUS "Lugizmo DF - Fetching GoogleTest...")
            include(FetchContent)
            FetchContent_Declare(
                    gtest
                    GIT_REPOSITORY https://github.com/google/googletest.git
                    GIT_TAG ${LUGIZMO_GTEST_VERSION}
            )
            FetchContent_MakeAvailable(gtest)
        endif()

    else()
        # Check target properties TODO this does not work
        # get_target_property(GTEST_VERSION gtest INTERFACE_VERSION)
        # if(GTEST_VERSION VERSION_LESS ${LUGIZMO_GTEST_MINIMUM})
        #     message(FATAL_ERROR "GTest version ${GTEST_VERSION} is less than required ${LUGIZMO_GTEST_MINIMUM}.")
        # else()
        #     message(STATUS "Using provided GTest version ${GTEST_VERSION}.")
        # endif()
        message(STATUS "Using provided GTest")
    endif()
endif()
