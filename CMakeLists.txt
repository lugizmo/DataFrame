cmake_minimum_required(VERSION 3.29)
set(LUGIZMO_DF_PROJECT_NAME DataFrame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 99)

# ====== DEPENDENCIES ======================================================================

include(vendor/fetch_project_options.cmake)
include(vendor/fetch_gtest.cmake)               # TODO add if tests

# ====== SOURCE ============================================================================
project(${LUGIZMO_DF_PROJECT_NAME}
        VERSION 0.0.1
        DESCRIPTION "DataFrame type to store tabular data."
        LANGUAGES CXX)

if(CMAKE_GENERATOR STREQUAL "Xcode")
    message(STATUS "Finalize in Xcode")
    project_options(PREFIX ${LUGIZMO_DF_PROJECT_NAME}
            DISABLE_EXCEPTIONS
            DISABLE_RTTI
            #ENABLE_CLANG_TIDY
            ENABLE_COVERAGE
            WARNINGS_AS_ERRORS
            #ENABLE_INCLUDE_WHAT_YOU_USE
            ENABLE_SANITIZER_ADDRESS
            ENABLE_SANITIZER_UNDEFINED)

elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "Finalize in Debug")
    project_options(PREFIX ${LUGIZMO_DF_PROJECT_NAME}
            DISABLE_EXCEPTIONS
            DISABLE_RTTI
            #ENABLE_CLANG_TIDY
            ENABLE_COVERAGE
            WARNINGS_AS_ERRORS
            #ENABLE_INCLUDE_WHAT_YOU_USE
            ENABLE_SANITIZER_ADDRESS
            ENABLE_SANITIZER_UNDEFINED)
else()
    project_options(PREFIX ${LUGIZMO_DF_PROJECT_NAME}
            DISABLE_EXCEPTIONS
            DISABLE_RTTI
            #ENABLE_CLANG_TIDY
            ENABLE_COVERAGE
            WARNINGS_AS_ERRORS
            #ENABLE_INCLUDE_WHAT_YOU_USE
            ENABLE_INTERPROCEDURAL_OPTIMIZATION
            ENABLE_NATIVE_OPTIMIZATION)
endif()

add_subdirectory(lib)