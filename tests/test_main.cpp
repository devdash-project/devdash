// Custom test main to initialize QCoreApplication for Qt-dependent tests

#include <QCoreApplication>

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    // Initialize Qt for tests that need the event loop or Qt types
    QCoreApplication app(argc, argv);
    app.setApplicationName("devdash_tests");

    // Run Catch2 tests
    return Catch::Session().run(argc, argv);
}
