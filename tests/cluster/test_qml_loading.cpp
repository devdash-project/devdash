/**
 * @file test_qml_loading.cpp
 * @brief Tests that verify QML files load without errors
 *
 * These tests catch QML compilation errors, circular dependencies,
 * and initialization issues that would otherwise only show up at runtime.
 */

#include <catch2/catch_test_macros.hpp>

#include <iostream>

#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTest>

#include "core/broker/DataBroker.h"

namespace devdash {

/**
 * @brief Test that ClusterMain.qml loads without QML errors
 *
 * This catches:
 * - Syntax errors in QML
 * - Missing imports
 * - Circular module dependencies
 * - Type resolution failures
 *
 * Note: Uses QQmlComponent instead of QQmlApplicationEngine to avoid
 * requiring a display/screen in headless test environments.
 */
TEST_CASE("ClusterMain.qml loads without errors", "[qml][cluster]") {
    // Create engine with DataBroker context (required by ClusterMain.qml)
    // Use QQmlApplicationEngine (same as real app) to ensure module plugins load
    DataBroker broker;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("dataBroker", &broker);

    // Capture any QML errors during load
    bool hasErrors = false;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [&hasErrors](const QList<QQmlError>& warnings) {
        hasErrors = true;
        std::cerr << "\n=== ClusterMain.qml QML Errors ===" << std::endl;
        for (const auto& error : warnings) {
            std::cerr << "  " << error.toString().toStdString() << std::endl;
        }
        std::cerr << "==================================\n" << std::endl;
    });

    // Load ClusterMain.qml (same way as real app)
    engine.load(QUrl("qrc:/DevDash/Cluster/qml/ClusterMain.qml"));

    // Verify load succeeded
    REQUIRE(!hasErrors);
    REQUIRE(!engine.rootObjects().isEmpty());

    // Verify root object is a Window
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    REQUIRE(window != nullptr);
}

/**
 * @brief Test that Tachometer.qml component loads without errors
 *
 * Tests individual gauge component in isolation to catch
 * component-specific issues.
 */
TEST_CASE("Tachometer.qml component loads", "[qml][cluster][gauges]") {
    QQmlEngine engine;

    // Load Tachometer component
    QQmlComponent component(&engine, QUrl("qrc:/DevDash/Cluster/qml/gauges/Tachometer.qml"));

    // Check for errors during component creation
    REQUIRE(component.status() != QQmlComponent::Error);
    if (component.status() == QQmlComponent::Error) {
        qWarning() << "Tachometer.qml errors:";
        for (const auto& error : component.errors()) {
            qWarning() << "  " << error.toString();
        }
    }
    REQUIRE(component.errorString().isEmpty());

    // Actually instantiate the component
    QObject* obj = component.create();
    REQUIRE(obj != nullptr);

    // Verify required properties exist
    REQUIRE(obj->property("value").isValid());
    REQUIRE(obj->property("maxValue").isValid());
    REQUIRE(obj->property("label").isValid());

    delete obj;
}

/**
 * @brief Test that all QML files in cluster module compile
 *
 * This is a meta-test that ensures the QML module itself is valid.
 */
TEST_CASE("DevDash.Cluster module is importable", "[qml][cluster]") {
    QQmlEngine engine;

    // Try to import the module
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import DevDash.Cluster

        Item {
            // Just test that import works
        }
    )", QUrl());

    REQUIRE(component.status() != QQmlComponent::Error);
    if (component.status() == QQmlComponent::Error) {
        qWarning() << "DevDash.Cluster import errors:";
        for (const auto& error : component.errors()) {
            qWarning() << "  " << error.toString();
        }
    }

    QObject* obj = component.create();
    REQUIRE(obj != nullptr);
    delete obj;
}

} // namespace devdash
