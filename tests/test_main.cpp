/**
 * @file test_main.cpp
 * @brief Catch2 test runner with Qt integration.
 *
 * Provides a custom main() that initializes QGuiApplication before
 * running Catch2 tests. This is required for tests that:
 * - Use Qt event loop (signals/slots, timers)
 * - Use Qt types that require Q_DECLARE_METATYPE registration
 * - Access Qt resources or plugins
 * - Load QML components (requires QGuiApplication for font/rendering)
 *
 * @note Uses QGuiApplication instead of QCoreApplication to support
 *       QML component loading tests that need GUI subsystems.
 */

#include <QGuiApplication>

#include <catch2/catch_session.hpp>

namespace {

/// Test application name for Qt identification
constexpr const char* TEST_APP_NAME = "devdash_tests";

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Set platform to offscreen to avoid needing a display
    qputenv("QT_QPA_PLATFORM", "offscreen");

    // Initialize Qt GUI application before any tests run
    // Required for QML tests that need font database, rendering, etc.
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(TEST_APP_NAME);

    // Run Catch2 test session
    // Catch2 handles all test discovery and execution
    return Catch::Session().run(argc, argv);
}