include_guard()
include(FetchContent)

FetchContent_Declare(
        project_options
        GIT_REPOSITORY https://github.com/aminya/project_options.git
        GIT_TAG        v0.41.0
)

FetchContent_MakeAvailable(project_options)