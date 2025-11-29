/**
 * @file test_protocol_adapter_factory.cpp
 * @brief Unit tests for ProtocolAdapterFactory.
 *
 * Tests cover:
 * - Creating adapters by type name
 * - Creating adapters from JSON config objects
 * - Creating adapters from profile files
 * - Error handling for invalid inputs
 */

#include "adapters/ProtocolAdapterFactory.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>

#include <catch2/catch_test_macros.hpp>

using devdash::ProtocolAdapterFactory;

namespace {

//=============================================================================
// Test Constants - Adapter Types
//=============================================================================

constexpr const char* ADAPTER_TYPE_HALTECH = "haltech";
constexpr const char* ADAPTER_TYPE_SIMULATOR = "simulator";
constexpr const char* ADAPTER_TYPE_UNKNOWN = "nonexistent";

//=============================================================================
// Test Constants - Config Keys
//=============================================================================

constexpr const char* KEY_ADAPTER = "adapter";
constexpr const char* KEY_ADAPTER_CONFIG = "adapterConfig";
constexpr const char* KEY_INTERFACE = "interface";
constexpr const char* KEY_PROTOCOL_FILE = "protocolFile";

//=============================================================================
// Test Constants - Values
//=============================================================================

constexpr const char* TEST_INTERFACE = "vcan0";
constexpr const char* TEST_PROTOCOL_FILE = "protocols/haltech/haltech-can-protocol-v2.35.json";

//=============================================================================
// Test Helpers
//=============================================================================

/**
 * @brief Create a minimal adapter config object.
 */
QJsonObject makeAdapterConfig(const char* interface = TEST_INTERFACE,
                              const char* protocolFile = nullptr) {
    QJsonObject config;
    config[KEY_INTERFACE] = interface;
    if (protocolFile != nullptr) {
        config[KEY_PROTOCOL_FILE] = protocolFile;
    }
    return config;
}

/**
 * @brief Create a full profile config object.
 */
QJsonObject makeProfileConfig(const char* adapterType, const QJsonObject& adapterConfig = {}) {
    QJsonObject config;
    config[KEY_ADAPTER] = adapterType;
    config[KEY_ADAPTER_CONFIG] = adapterConfig;
    return config;
}

/**
 * @brief Create a temporary profile file with given config.
 * @return Path to temporary file (file stays valid while QTemporaryFile exists)
 */
std::pair<QString, std::unique_ptr<QTemporaryFile>> createTempProfile(const QJsonObject& config) {
    auto tempFile = std::make_unique<QTemporaryFile>();
    tempFile->setAutoRemove(true);

    if (!tempFile->open()) {
        return {"", nullptr};
    }

    QJsonDocument doc(config);
    tempFile->write(doc.toJson());
    tempFile->close();

    return {tempFile->fileName(), std::move(tempFile)};
}

} // anonymous namespace

//=============================================================================
// Create by Type Tests
//=============================================================================

TEST_CASE("ProtocolAdapterFactory::create by type", "[factory][adapter]") {

    SECTION("creates haltech adapter") {
        auto config = makeAdapterConfig();
        auto adapter = ProtocolAdapterFactory::create(ADAPTER_TYPE_HALTECH, config);

        REQUIRE(adapter != nullptr);
        REQUIRE(adapter->adapterName() == "Haltech CAN");
    }

    SECTION("creates simulator adapter") {
        QJsonObject config;
        auto adapter = ProtocolAdapterFactory::create(ADAPTER_TYPE_SIMULATOR, config);

        REQUIRE(adapter != nullptr);
        // Simulator should have a name containing "simulator" (case-insensitive check)
        REQUIRE(adapter->adapterName().toLower().contains("simulator"));
    }

    SECTION("returns nullptr for unknown adapter type") {
        QJsonObject config;
        auto adapter = ProtocolAdapterFactory::create(ADAPTER_TYPE_UNKNOWN, config);

        REQUIRE(adapter == nullptr);
    }

    SECTION("returns nullptr for empty adapter type") {
        QJsonObject config;
        auto adapter = ProtocolAdapterFactory::create("", config);

        REQUIRE(adapter == nullptr);
    }
}

//=============================================================================
// Create from Config Tests
//=============================================================================

TEST_CASE("ProtocolAdapterFactory::createFromConfig", "[factory][adapter]") {

    SECTION("creates adapter from valid config") {
        auto adapterConfig = makeAdapterConfig();
        auto profileConfig = makeProfileConfig(ADAPTER_TYPE_HALTECH, adapterConfig);

        auto adapter = ProtocolAdapterFactory::createFromConfig(profileConfig);

        REQUIRE(adapter != nullptr);
        REQUIRE(adapter->adapterName() == "Haltech CAN");
    }

    SECTION("extracts adapterConfig correctly") {
        auto adapterConfig = makeAdapterConfig("can1");
        auto profileConfig = makeProfileConfig(ADAPTER_TYPE_HALTECH, adapterConfig);

        auto adapter = ProtocolAdapterFactory::createFromConfig(profileConfig);

        REQUIRE(adapter != nullptr);
        // Adapter should have been configured with "can1" interface
        // (We can't easily verify this without exposing internals,
        //  but at least it was created successfully)
    }

    SECTION("returns nullptr when adapter type missing") {
        QJsonObject config;
        config[KEY_ADAPTER_CONFIG] = makeAdapterConfig();
        // Missing "adapter" key

        auto adapter = ProtocolAdapterFactory::createFromConfig(config);

        REQUIRE(adapter == nullptr);
    }

    SECTION("handles missing adapterConfig gracefully") {
        QJsonObject config;
        config[KEY_ADAPTER] = ADAPTER_TYPE_SIMULATOR;
        // Missing "adapterConfig" - should use empty object

        auto adapter = ProtocolAdapterFactory::createFromConfig(config);

        // Simulator should work without config
        REQUIRE(adapter != nullptr);
    }
}

//=============================================================================
// Create from Profile File Tests
//=============================================================================

TEST_CASE("ProtocolAdapterFactory::createFromProfile", "[factory][adapter]") {

    SECTION("creates adapter from valid profile file") {
        auto adapterConfig = makeAdapterConfig();
        auto profileConfig = makeProfileConfig(ADAPTER_TYPE_SIMULATOR, adapterConfig);

        auto [path, tempFile] = createTempProfile(profileConfig);
        REQUIRE(!path.isEmpty());

        auto adapter = ProtocolAdapterFactory::createFromProfile(path);

        REQUIRE(adapter != nullptr);
    }

    SECTION("returns nullptr for nonexistent file") {
        auto adapter = ProtocolAdapterFactory::createFromProfile("/nonexistent/path/profile.json");

        REQUIRE(adapter == nullptr);
    }

    SECTION("returns nullptr for invalid JSON") {
        QTemporaryFile tempFile;
        tempFile.open();
        tempFile.write("{ invalid json }");
        tempFile.close();

        auto adapter = ProtocolAdapterFactory::createFromProfile(tempFile.fileName());

        REQUIRE(adapter == nullptr);
    }

    SECTION("returns nullptr for non-object JSON") {
        QTemporaryFile tempFile;
        tempFile.open();
        tempFile.write("[\"array\", \"not\", \"object\"]");
        tempFile.close();

        auto adapter = ProtocolAdapterFactory::createFromProfile(tempFile.fileName());

        REQUIRE(adapter == nullptr);
    }

    SECTION("returns nullptr for empty file") {
        QTemporaryFile tempFile;
        tempFile.open();
        tempFile.close();

        auto adapter = ProtocolAdapterFactory::createFromProfile(tempFile.fileName());

        REQUIRE(adapter == nullptr);
    }
}

//=============================================================================
// Integration Tests
//=============================================================================

TEST_CASE("ProtocolAdapterFactory full profile workflow", "[factory][adapter][integration]") {

    SECTION("haltech profile with protocol file") {
        auto adapterConfig = makeAdapterConfig(TEST_INTERFACE, TEST_PROTOCOL_FILE);
        auto profileConfig = makeProfileConfig(ADAPTER_TYPE_HALTECH, adapterConfig);

        auto [path, tempFile] = createTempProfile(profileConfig);
        REQUIRE(!path.isEmpty());

        auto adapter = ProtocolAdapterFactory::createFromProfile(path);

        REQUIRE(adapter != nullptr);
        REQUIRE(adapter->adapterName() == "Haltech CAN");
        REQUIRE_FALSE(adapter->isRunning());
    }
}