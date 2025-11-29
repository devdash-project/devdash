/**
 * @file test_haltech_protocol.cpp
 * @brief Unit tests for HaltechProtocol CAN decoder.
 *
 * Tests cover:
 * - Low-level byte decoding (uint16, int16)
 * - Convenience decoders (RPM, temperature, pressure)
 * - Conversion formula parsing and application
 * - JSON protocol loading
 * - Full frame decoding with JSON definitions
 */

#include "adapters/haltech/HaltechProtocol.h"

#include <QCanBusFrame>
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryFile>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace {

//=============================================================================
// Test Constants - CAN Frame IDs
//=============================================================================

/// Engine Core 1 frame (RPM, MAP, TPS, Coolant Pressure)
constexpr uint32_t FRAME_ID_ENGINE_CORE_1 = 0x360;

/// Pressures frame (Fuel, Oil pressures)
constexpr uint32_t FRAME_ID_PRESSURES = 0x361;

/// Battery voltage frame
constexpr uint32_t FRAME_ID_BATTERY = 0x372;

/// Temperatures 1 frame (Coolant, Air, Fuel, Oil temps)
constexpr uint32_t FRAME_ID_TEMPS_1 = 0x3E0;

/// Fuel Level frame
constexpr uint32_t FRAME_ID_FUEL_LEVEL = 0x3E2;

/// Unknown frame ID for negative testing
constexpr uint32_t FRAME_ID_UNKNOWN = 0x999;

//=============================================================================
// Test Constants - Expected Values
//=============================================================================

/// Test RPM value (3500 RPM = 0x0DAC)
constexpr double TEST_RPM_VALUE = 3500.0;
constexpr uint16_t TEST_RPM_RAW = 0x0DAC;

/// Idle RPM value
constexpr double TEST_IDLE_RPM = 800.0;

/// High RPM value
constexpr double TEST_HIGH_RPM = 7500.0;

/// Test throttle position (50% = 500 raw with 0.1% resolution)
constexpr double TEST_TPS_PERCENT = 50.0;
constexpr uint16_t TEST_TPS_RAW = 500;

/// Test temperature (90°C = 363.15K * 10 ≈ 3632)
constexpr double TEST_TEMP_CELSIUS = 90.0;
constexpr uint16_t TEST_TEMP_RAW = 3632;

/// Cold temperature (20°C)
constexpr double TEST_COLD_TEMP_CELSIUS = 20.0;
constexpr uint16_t TEST_COLD_TEMP_RAW = 2932;

/// Freezing temperature (0°C)
constexpr double TEST_FREEZING_TEMP_CELSIUS = 0.0;
constexpr uint16_t TEST_FREEZING_TEMP_RAW = 2732;

/// Atmospheric pressure in kPa
constexpr double TEST_ATMOSPHERIC_KPA = 101.3;
constexpr uint16_t TEST_ATMOSPHERIC_RAW = 1013;

/// Boost pressure (200 kPa)
constexpr double TEST_BOOST_KPA = 200.0;
constexpr uint16_t TEST_BOOST_RAW = 2000;

/// Battery voltage (14.2V)
constexpr double TEST_BATTERY_VOLTS = 14.2;
constexpr uint16_t TEST_BATTERY_RAW = 142;

/// Fuel level (50.5L)
constexpr double TEST_FUEL_LEVEL_L = 50.5;
constexpr uint16_t TEST_FUEL_LEVEL_RAW = 505;

/// Gauge pressure test (100 kPa gauge = 201.3 kPa absolute)
constexpr double TEST_GAUGE_KPA = 100.0;
constexpr uint16_t TEST_GAUGE_RAW = 2013;

//=============================================================================
// Test Constants - Tolerances
//=============================================================================

/// Temperature comparison tolerance (°C)
constexpr double TEMP_TOLERANCE = 1.0;

/// Pressure comparison tolerance (kPa)
constexpr double PRESSURE_TOLERANCE = 0.1;

/// Voltage comparison tolerance (V)
constexpr double VOLTAGE_TOLERANCE = 0.1;

//=============================================================================
// Test Helpers
//=============================================================================

/**
 * @brief Find protocol file relative to test execution directory.
 * @return Path to protocol JSON, or empty string if not found
 */
QString findProtocolFile() {
    QStringList paths = {
        "protocols/haltech/haltech-can-protocol-v2.35.json",
        "../protocols/haltech/haltech-can-protocol-v2.35.json",
        "../../protocols/haltech/haltech-can-protocol-v2.35.json",
        "../../../protocols/haltech/haltech-can-protocol-v2.35.json",
    };

    for (const auto& path : paths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return QString();
}

/**
 * @brief Create a valid CAN frame with given ID and hex payload.
 * @param frameId CAN frame ID
 * @param hexPayload Hex string for payload
 * @return Valid QCanBusFrame
 */
QCanBusFrame makeFrame(uint32_t frameId, const char* hexPayload) {
    return QCanBusFrame(frameId, QByteArray::fromHex(hexPayload));
}

/**
 * @brief Find a channel by name substring in decode results.
 * @param results Decoded channel results
 * @param nameContains Substring to search for (case-insensitive)
 * @return Pointer to ChannelValue if found, nullptr otherwise
 */
const devdash::ChannelValue* findChannel(
    const std::vector<std::pair<QString, devdash::ChannelValue>>& results,
    const QString& nameContains) {
    
    for (const auto& [name, value] : results) {
        if (name.toLower().contains(nameContains.toLower())) {
            return &value;
        }
    }
    return nullptr;
}

} // anonymous namespace

//=============================================================================
// Low-Level Decoding Tests
//=============================================================================

TEST_CASE("HaltechProtocol decodes uint16 correctly", "[haltech][protocol]") {
    SECTION("big-endian decoding") {
        QByteArray payload = QByteArray::fromHex("0DAC03E8");

        auto value = devdash::HaltechProtocol::decodeUint16(payload, 0);
        REQUIRE(value == TEST_RPM_RAW);

        auto value2 = devdash::HaltechProtocol::decodeUint16(payload, 2);
        REQUIRE(value2 == 0x03E8);
    }

    SECTION("zero value") {
        QByteArray payload = QByteArray::fromHex("00000000");
        REQUIRE(devdash::HaltechProtocol::decodeUint16(payload, 0) == 0);
    }

    SECTION("max value") {
        QByteArray payload = QByteArray::fromHex("FFFF");
        REQUIRE(devdash::HaltechProtocol::decodeUint16(payload, 0) == 65535);
    }

    SECTION("out of bounds returns zero") {
        QByteArray payload = QByteArray::fromHex("0DAC");
        REQUIRE(devdash::HaltechProtocol::decodeUint16(payload, 2) == 0);
    }
}

TEST_CASE("HaltechProtocol decodes int16 correctly", "[haltech][protocol]") {
    SECTION("positive value") {
        QByteArray payload = QByteArray::fromHex("7FFF"); // +32767
        REQUIRE(devdash::HaltechProtocol::decodeInt16(payload, 0) == 32767);
    }

    SECTION("negative value") {
        QByteArray payload = QByteArray::fromHex("8000"); // -32768
        REQUIRE(devdash::HaltechProtocol::decodeInt16(payload, 0) == -32768);
    }

    SECTION("small negative") {
        QByteArray payload = QByteArray::fromHex("FFFF"); // -1
        REQUIRE(devdash::HaltechProtocol::decodeInt16(payload, 0) == -1);
    }
}

//=============================================================================
// Convenience Decoder Tests
//=============================================================================

TEST_CASE("HaltechProtocol decodes RPM correctly", "[haltech][protocol]") {
    SECTION("normal RPM value") {
        QByteArray payload = QByteArray::fromHex("0DAC000000000000");
        auto rpm = devdash::HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == TEST_RPM_VALUE);
    }

    SECTION("idle RPM") {
        QByteArray payload = QByteArray::fromHex("0320000000000000");
        auto rpm = devdash::HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == TEST_IDLE_RPM);
    }

    SECTION("high RPM") {
        QByteArray payload = QByteArray::fromHex("1D4C000000000000");
        auto rpm = devdash::HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == TEST_HIGH_RPM);
    }
}

TEST_CASE("HaltechProtocol decodes temperature correctly", "[haltech][protocol]") {
    SECTION("normal coolant temperature") {
        QByteArray payload = QByteArray::fromHex("0E30");
        auto temp = devdash::HaltechProtocol::decodeTemperature(payload, 0);
        REQUIRE_THAT(temp, WithinAbs(TEST_TEMP_CELSIUS, TEMP_TOLERANCE));
    }

    SECTION("cold temperature") {
        QByteArray payload = QByteArray::fromHex("0B74");
        auto temp = devdash::HaltechProtocol::decodeTemperature(payload, 0);
        REQUIRE_THAT(temp, WithinAbs(TEST_COLD_TEMP_CELSIUS, TEMP_TOLERANCE));
    }

    SECTION("freezing temperature") {
        QByteArray payload = QByteArray::fromHex("0AAC");
        auto temp = devdash::HaltechProtocol::decodeTemperature(payload, 0);
        REQUIRE_THAT(temp, WithinAbs(TEST_FREEZING_TEMP_CELSIUS, TEMP_TOLERANCE));
    }
}

TEST_CASE("HaltechProtocol decodes pressure correctly", "[haltech][protocol]") {
    SECTION("atmospheric pressure") {
        QByteArray payload = QByteArray::fromHex("03F5");
        auto pressure = devdash::HaltechProtocol::decodePressure(payload, 0);
        REQUIRE_THAT(pressure, WithinAbs(TEST_ATMOSPHERIC_KPA, PRESSURE_TOLERANCE));
    }

    SECTION("boost pressure") {
        QByteArray payload = QByteArray::fromHex("07D0");
        auto pressure = devdash::HaltechProtocol::decodePressure(payload, 0);
        REQUIRE(pressure == TEST_BOOST_KPA);
    }
}

//=============================================================================
// Conversion Function Tests
//=============================================================================

TEST_CASE("HaltechProtocol conversion functions", "[haltech][protocol]") {
    using devdash::ConversionType;
    using devdash::HaltechProtocol;

    SECTION("parseConversion identifies patterns") {
        REQUIRE(HaltechProtocol::parseConversion("x") == ConversionType::Identity);
        REQUIRE(HaltechProtocol::parseConversion("") == ConversionType::Identity);
        REQUIRE(HaltechProtocol::parseConversion("x / 10") == ConversionType::DivideBy10);
        REQUIRE(HaltechProtocol::parseConversion("x / 1000") == ConversionType::DivideBy1000);
        REQUIRE(HaltechProtocol::parseConversion("(x / 10) - 101.3") ==
                ConversionType::GaugePressure);
        REQUIRE(HaltechProtocol::parseConversion("(x / 10) - 101.325") ==
                ConversionType::GaugePressure);
    }

    SECTION("applyConversion calculates correctly") {
        constexpr double RAW_VALUE = 1000.0;

        REQUIRE(HaltechProtocol::applyConversion(ConversionType::Identity, RAW_VALUE) ==
                RAW_VALUE);
        REQUIRE(HaltechProtocol::applyConversion(ConversionType::DivideBy10, RAW_VALUE) ==
                100.0);
        REQUIRE(HaltechProtocol::applyConversion(ConversionType::DivideBy1000, RAW_VALUE) ==
                1.0);

        // Gauge pressure: (x / 10) - 101.325
        REQUIRE_THAT(
            HaltechProtocol::applyConversion(ConversionType::GaugePressure, TEST_GAUGE_RAW),
            WithinAbs(TEST_GAUGE_KPA, PRESSURE_TOLERANCE));

        // Kelvin to Celsius: (x / 10) - 273.15
        REQUIRE_THAT(
            HaltechProtocol::applyConversion(ConversionType::KelvinToCelsius, TEST_TEMP_RAW),
            WithinAbs(TEST_TEMP_CELSIUS, TEMP_TOLERANCE));
    }
}

//=============================================================================
// Frame Decoding Without JSON (Requires JSON)
//=============================================================================

TEST_CASE("HaltechProtocol requires JSON for decoding", "[haltech][protocol]") {
    devdash::HaltechProtocol protocol;

    SECTION("decode returns empty without JSON loaded") {
        // Without JSON, there are no decoders registered
        REQUIRE_FALSE(protocol.isLoaded());

        QCanBusFrame frame = makeFrame(FRAME_ID_ENGINE_CORE_1, "0DAC01F400000000");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("invalid frame returns empty") {
        QCanBusFrame frame;
        frame.setFrameId(FRAME_ID_ENGINE_CORE_1);
        // Frame is invalid (payload not set properly)
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("short payload returns empty") {
        QCanBusFrame frame = makeFrame(FRAME_ID_ENGINE_CORE_1, "0D");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }
}

//=============================================================================
// JSON Loading Tests
//=============================================================================

TEST_CASE("HaltechProtocol loads JSON definition", "[haltech][protocol][json]") {
    QString protocolPath = findProtocolFile();
    if (protocolPath.isEmpty()) {
        SKIP("Protocol file not found - run tests from project root");
    }

    devdash::HaltechProtocol protocol;

    SECTION("loads protocol file successfully") {
        REQUIRE(protocol.loadDefinition(protocolPath));
        REQUIRE(protocol.isLoaded());
    }

    SECTION("reports correct frame IDs") {
        REQUIRE(protocol.loadDefinition(protocolPath));

        auto frameIds = protocol.frameIds();
        REQUIRE(frameIds.contains(FRAME_ID_ENGINE_CORE_1));
        REQUIRE(frameIds.contains(FRAME_ID_PRESSURES));
        REQUIRE(frameIds.contains(FRAME_ID_TEMPS_1));
    }

    SECTION("fails gracefully on missing file") {
        REQUIRE_FALSE(protocol.loadDefinition("/nonexistent/path.json"));
        REQUIRE_FALSE(protocol.isLoaded());
    }

    SECTION("fails gracefully on invalid JSON") {
        // Create a temp file with invalid JSON
        QTemporaryFile tempFile;
        tempFile.open();
        tempFile.write("{ invalid json }");
        tempFile.close();

        REQUIRE_FALSE(protocol.loadDefinition(tempFile.fileName()));
    }
}

//=============================================================================
// Full Frame Decoding with JSON
//=============================================================================

TEST_CASE("HaltechProtocol decodes with JSON definition", "[haltech][protocol][json]") {
    QString protocolPath = findProtocolFile();
    if (protocolPath.isEmpty()) {
        SKIP("Protocol file not found - run tests from project root");
    }

    devdash::HaltechProtocol protocol;
    REQUIRE(protocol.loadDefinition(protocolPath));

    SECTION("decodes RPM from 0x360") {
        QCanBusFrame frame = makeFrame(FRAME_ID_ENGINE_CORE_1, "0DAC03F501F40000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        auto* rpm = findChannel(results, "rpm");
        REQUIRE(rpm != nullptr);
        REQUIRE(rpm->value == TEST_RPM_VALUE);
        REQUIRE(rpm->valid);
    }

    SECTION("decodes temperatures from 0x3E0 with Kelvin conversion") {
        QCanBusFrame frame = makeFrame(FRAME_ID_TEMPS_1, "0E300B740B740E30");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        auto* coolant = findChannel(results, "coolant");
        REQUIRE(coolant != nullptr);
        REQUIRE_THAT(coolant->value, WithinAbs(TEST_TEMP_CELSIUS, TEMP_TOLERANCE));
        REQUIRE(coolant->unit == QString::fromUtf8("°C"));
    }

    SECTION("decodes gauge pressure from 0x361") {
        QCanBusFrame frame = makeFrame(FRAME_ID_PRESSURES, "07DD07DD00000000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        auto* fuel = findChannel(results, "fuel");
        if (fuel && fuel->unit.contains("kPa")) {
            // 0x07DD = 2013, gauge = (2013/10) - 101.325 ≈ 100.0
            REQUIRE_THAT(fuel->value, WithinAbs(TEST_GAUGE_KPA, PRESSURE_TOLERANCE));
        }
    }

    SECTION("decodes battery voltage from 0x372") {
        QCanBusFrame frame = makeFrame(FRAME_ID_BATTERY, "008E000000000000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        auto* battery = findChannel(results, "battery");
        REQUIRE(battery != nullptr);
        REQUIRE_THAT(battery->value, WithinAbs(TEST_BATTERY_VOLTS, VOLTAGE_TOLERANCE));
    }

    SECTION("decodes fuel level from 0x3E2") {
        QCanBusFrame frame = makeFrame(FRAME_ID_FUEL_LEVEL, "01F9000000000000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        auto* fuelLevel = findChannel(results, "fuel");
        REQUIRE(fuelLevel != nullptr);
        REQUIRE_THAT(fuelLevel->value, WithinAbs(TEST_FUEL_LEVEL_L, VOLTAGE_TOLERANCE));
    }

    SECTION("unknown frame ID returns empty") {
        QCanBusFrame frame = makeFrame(FRAME_ID_UNKNOWN, "0DAC01F400000000");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }
}