include_guard()
include(FetchContent)

if(OPT_BUILD_BENCH)

    FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG        v1.9.0
    )

    FetchContent_MakeAvailable(benchmark)

endif(OPT_BUILD_BENCH)