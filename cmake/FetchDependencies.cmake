# External dependencies via FetchContent

include(FetchContent)

# Catch2 for testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)

# nlohmann/json for config parsing
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# moodycamel::ConcurrentQueue for lock-free queue
FetchContent_Declare(
    concurrentqueue
    GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
    GIT_TAG v1.0.4
)

FetchContent_MakeAvailable(Catch2 nlohmann_json concurrentqueue)

# Add Catch2 CMake module path for catch_discover_tests
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
