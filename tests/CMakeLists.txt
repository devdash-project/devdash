# Tests configuration

include(Catch)

# Main test executable with custom main for Qt integration
add_executable(devdash_tests
    test_main.cpp
    core/broker/test_data_broker.cpp
    core/conversion/test_default_unit_converter.cpp
    adapters/haltech/test_haltech_protocol.cpp
    adapters/haltech/test_pd16_protocol.cpp
    cluster/test_qml_loading.cpp
)

target_include_directories(devdash_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(devdash_tests PRIVATE
    BUILD_TESTING
    SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(devdash_tests PRIVATE
    devdash_core
    devdash_adapters
    devdash_cluster
    Catch2::Catch2
    Qt6::Core
    Qt6::Test
    Qt6::Qml
    Qt6::Quick
)

# Enable sanitizers for test executable (needed when linking sanitized libraries)
enable_sanitizers(devdash_tests)

# Discover tests for CTest integration
catch_discover_tests(devdash_tests)
