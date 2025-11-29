/**
 * @file test_main.cpp
 * @brief Catch2 test runner with Qt integration.
 *
 * Provides a custom main() that initializes QCoreApplication before
 * running Catch2 tests. This is required for tests that:
 * - Use Qt event loop (signals/slots, timers)
 * - Use Qt types that require Q_DECLARE_METATYPE registration
 * - Access Qt resources or plugins
 *
 * @note This file should be compiled separately from the test sources
 *       and linked with CATCH_CONFIG_RUNNER defined.
 */

#include <QCoreApplication>

#include <catch2/catch_session.hpp>

namespace {

/// Test application name for Qt identification
constexpr const char* TEST_APP_NAME = "devdash_tests";

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Initialize Qt before any tests run
    // Required for QObject-based tests, signal/slot connections, etc.
    QCoreApplication app(argc, argv);
    app.setApplicationName(TEST_APP_NAME);

    // Run Catch2 test session
    // Catch2 handles all test discovery and execution
    return Catch::Session().run(argc, argv);
}