include_guard()
include(FetchContent)

FetchContent_Declare(
        gtest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)

FetchContent_MakeAvailable(gtest)