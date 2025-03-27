include(FetchContent)

# # -- Tests --

# FetchContent_Declare(googletest
#     URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)

# FetchContent_MakeAvailable(googletest)
# FetchContent_GetProperties(googletest)

# if (NOT googletest_POPULATED)
#     FetchContent_Populate(googletest)
#     add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
# endif()

# -- JSON --

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2  # Specify a release tag
)

FetchContent_MakeAvailable(json)
FetchContent_GetProperties(json)

if (NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR})
endif()
