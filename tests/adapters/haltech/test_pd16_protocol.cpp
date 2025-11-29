/**
 * @file test_pd16_protocol.cpp
 * @brief Unit tests for PD16Protocol CAN decoder.
 *
 * Tests cover:
 * - Device configuration (A/B/C/D selection, base ID calculation)
 * - Multiplexer byte decoding (IO type, IO index extraction)
 * - Frame filtering (ignores wrong device, accepts correct device)
 * - Input status decoding (SPI, AVI channels)
 * - Output status decoding (25A, 8A channels)
 * - Device status decoding (firmware version)
 * - IO type name lookup
 */

#include "adapters/haltech/PD16Protocol.h"

#include <QCanBusFrame>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace {

//=============================================================================
// Test Constants - Device Base IDs
//=============================================================================

/// Device A base CAN ID
constexpr uint32_t BASE_ID_DEVICE_A = 0x6D0;

/// Device B base CAN ID
constexpr uint32_t BASE_ID_DEVICE_B = 0x6D8;

/// Device C base CAN ID
constexpr uint32_t BASE_ID_DEVICE_C = 0x6E0;

/// Device D base CAN ID
constexpr uint32_t BASE_ID_DEVICE_D = 0x6E8;

//=============================================================================
// Test Constants - Frame Offsets
//=============================================================================

/// Input status frame offset from base
constexpr int FRAME_OFFSET_INPUT_STATUS = 3;

/// Output status frame offset from base
constexpr int FRAME_OFFSET_OUTPUT_STATUS = 4;

/// Device status frame offset from base
constexpr int FRAME_OFFSET_DEVICE_STATUS = 5;

//=============================================================================
// Test Constants - Mux Byte Values
//=============================================================================

/// Mux byte for 25A output, index 0: type=0, index=0
constexpr uint8_t MUX_25A_INDEX_0 = 0x00;

/// Mux byte for 8A output, index 0: type=1 (0b001 << 5), index=0
constexpr uint8_t MUX_8A_INDEX_0 = 0x20;

/// Mux byte for Half Bridge, index 0: type=2 (0b010 << 5), index=0
constexpr uint8_t MUX_HBO_INDEX_0 = 0x40;

/// Mux byte for Speed Pulse Input, index 1: type=3 (0b011 << 5), index=1
constexpr uint8_t MUX_SPI_INDEX_1 = 0x61;

/// Mux byte for Speed Pulse Input, index 2: type=3 (0b011 << 5), index=2
constexpr uint8_t MUX_SPI_INDEX_2 = 0x62;

/// Mux byte for Analog Voltage Input, index 0: type=4 (0b100 << 5), index=0
constexpr uint8_t MUX_AVI_INDEX_0 = 0x80;

//=============================================================================
// Test Constants - Expected Values
//=============================================================================

/// Test voltage (5.0V = 5000mV)
constexpr double TEST_VOLTAGE_V = 5.0;
constexpr uint16_t TEST_VOLTAGE_MV = 5000;

/// Test voltage (12.0V = 12000mV)
constexpr double TEST_12V_VOLTAGE = 12.0;
constexpr uint16_t TEST_12V_VOLTAGE_MV = 12000;

/// Test duty cycle (75.0% = 750 raw with 0.1% resolution)
constexpr double TEST_DUTY_PERCENT = 75.0;
constexpr uint16_t TEST_DUTY_RAW = 750;

/// Test frequency (1000 Hz)
constexpr double TEST_FREQUENCY_HZ = 1000.0;
constexpr uint16_t TEST_FREQUENCY_RAW = 1000;

/// Test current (15A = 15000mA)
constexpr double TEST_CURRENT_A = 15.0;
constexpr uint16_t TEST_CURRENT_MA = 15000;

/// Test load percentage
constexpr double TEST_LOAD_PERCENT = 80.0;
constexpr uint8_t TEST_LOAD_RAW = 80;

/// Test firmware version (2.15.3 = 2 + 15/100 + 3/10000)
constexpr double TEST_FW_VERSION = 2.1503;
constexpr uint8_t TEST_FW_MAJOR = 2;
constexpr uint8_t TEST_FW_MINOR = 15;
constexpr uint8_t TEST_FW_BUGFIX = 3;

//=============================================================================
// Test Constants - Tolerances
//=============================================================================

/// Voltage comparison tolerance (V)
constexpr double VOLTAGE_TOLERANCE = 0.01;

/// Percentage comparison tolerance
constexpr double PERCENT_TOLERANCE = 0.1;

/// Firmware version comparison tolerance
constexpr double FW_VERSION_TOLERANCE = 0.0001;

//=============================================================================
// Test Constants - Other
//=============================================================================

/// ECU frame ID (for negative testing - should be ignored by PD16)
constexpr uint32_t ECU_FRAME_ID = 0x360;

//=============================================================================
// Test Helpers
//=============================================================================

/**
 * @brief Create a valid CAN frame with given ID and hex payload.
 */
QCanBusFrame makeFrame(uint32_t frameId, const char* hexPayload) {
    return QCanBusFrame(frameId, QByteArray::fromHex(hexPayload));
}

/**
 * @brief Find a channel by name substring in decode results.
 */
const devdash::ChannelValue* findChannel(
    const std::vector<std::pair<QString, devdash::ChannelValue>>& results,
    const QString& nameContains) {
    
    for (const auto& [name, value] : results) {
        if (name.contains(nameContains, Qt::CaseInsensitive)) {
            return &value;
        }
    }
    return nullptr;
}

/**
 * @brief Check if any channel name contains the given substring.
 */
bool hasChannel(
    const std::vector<std::pair<QString, devdash::ChannelValue>>& results,
    const QString& nameContains) {
    return findChannel(results, nameContains) != nullptr;
}

} // anonymous namespace

//=============================================================================
// Device Configuration Tests
//=============================================================================

TEST_CASE("PD16Protocol device configuration", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;

    SECTION("default device is A") {
        REQUIRE(protocol.deviceId() == devdash::PD16Protocol::DeviceId::A);
        REQUIRE(protocol.baseId() == BASE_ID_DEVICE_A);
        REQUIRE(protocol.devicePrefix() == "pd16_A");
    }

    SECTION("can set device B") {
        protocol.setDeviceId(devdash::PD16Protocol::DeviceId::B);
        REQUIRE(protocol.deviceId() == devdash::PD16Protocol::DeviceId::B);
        REQUIRE(protocol.baseId() == BASE_ID_DEVICE_B);
        REQUIRE(protocol.devicePrefix() == "pd16_B");
    }

    SECTION("can set device C") {
        protocol.setDeviceId(devdash::PD16Protocol::DeviceId::C);
        REQUIRE(protocol.baseId() == BASE_ID_DEVICE_C);
        REQUIRE(protocol.devicePrefix() == "pd16_C");
    }

    SECTION("can set device D") {
        protocol.setDeviceId(devdash::PD16Protocol::DeviceId::D);
        REQUIRE(protocol.baseId() == BASE_ID_DEVICE_D);
        REQUIRE(protocol.devicePrefix() == "pd16_D");
    }
}

//=============================================================================
// Multiplexer Byte Decoding Tests
//=============================================================================

TEST_CASE("PD16Protocol mux byte decoding", "[pd16][protocol]") {
    using IOType = devdash::PD16Protocol::IOType;

    SECTION("extracts IO type correctly") {
        REQUIRE(devdash::PD16Protocol::getMuxType(MUX_25A_INDEX_0) == IOType::Output25A);
        REQUIRE(devdash::PD16Protocol::getMuxType(MUX_8A_INDEX_0) == IOType::Output8A);
        REQUIRE(devdash::PD16Protocol::getMuxType(MUX_HBO_INDEX_0) == IOType::HalfBridge);
        REQUIRE(devdash::PD16Protocol::getMuxType(MUX_SPI_INDEX_1) == IOType::SpeedPulse);
        REQUIRE(devdash::PD16Protocol::getMuxType(MUX_AVI_INDEX_0) == IOType::AnalogVoltage);
    }

    SECTION("extracts IO index correctly") {
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b00000000) == 0);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b00000101) == 5);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b00001111) == 15);
        // Upper bits (type) should be ignored
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b11110011) == 3);
    }

    SECTION("combines type and index") {
        // Type=SPI (3=0b011), Index=2 → 0b01100010 = 0x62
        REQUIRE(devdash::PD16Protocol::getMuxType(MUX_SPI_INDEX_2) == IOType::SpeedPulse);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(MUX_SPI_INDEX_2) == 2);
    }
}

//=============================================================================
// Frame Filtering Tests
//=============================================================================

TEST_CASE("PD16Protocol ignores wrong device frames", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("ignores device B frames") {
        uint32_t deviceBFrame = BASE_ID_DEVICE_B + FRAME_OFFSET_INPUT_STATUS;
        QCanBusFrame frame = makeFrame(deviceBFrame, "6000000000000000");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("ignores device C frames") {
        uint32_t deviceCFrame = BASE_ID_DEVICE_C + FRAME_OFFSET_INPUT_STATUS;
        QCanBusFrame frame = makeFrame(deviceCFrame, "6000000000000000");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("ignores ECU frames") {
        QCanBusFrame frame = makeFrame(ECU_FRAME_ID, "0DAC000000000000");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("accepts device A frames") {
        uint32_t deviceAFrame = BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS;
        QCanBusFrame frame = makeFrame(deviceAFrame, "6001000000000000");
        auto results = protocol.decode(frame);
        REQUIRE(!results.empty());
    }
}

//=============================================================================
// Input Status Decoding Tests
//=============================================================================

TEST_CASE("PD16Protocol decodes input status (SPI)", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("decodes SPI input with voltage and frequency") {
        // Frame: Input Status (+3) = 0x6D3
        // Mux: Type=SPI (3), Index=1 → 0x61
        // State: ON (bit 0 = 1)
        // Voltage: 5000mV (5.0V) = 0x1388
        // Duty: 750 (75.0%) = 0x02EE
        // Frequency: 1000 Hz = 0x03E8
        uint32_t frameId = BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS;
        QCanBusFrame frame = makeFrame(frameId, "6101138802EE03E8");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        // Check state
        auto* state = findChannel(results, "state");
        REQUIRE(state != nullptr);
        REQUIRE(state->value == 1.0);

        // Check voltage
        auto* voltage = findChannel(results, "voltage");
        REQUIRE(voltage != nullptr);
        REQUIRE_THAT(voltage->value, WithinAbs(TEST_VOLTAGE_V, VOLTAGE_TOLERANCE));

        // Check duty cycle
        auto* duty = findChannel(results, "dutyCycle");
        REQUIRE(duty != nullptr);
        REQUIRE_THAT(duty->value, WithinAbs(TEST_DUTY_PERCENT, PERCENT_TOLERANCE));

        // Check frequency
        auto* freq = findChannel(results, "frequency");
        REQUIRE(freq != nullptr);
        REQUIRE(freq->value == TEST_FREQUENCY_HZ);
    }

    SECTION("decodes SPI input OFF state") {
        uint32_t frameId = BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS;
        // State byte = 0x00 (OFF)
        QCanBusFrame frame = makeFrame(frameId, "6100138802EE03E8");
        auto results = protocol.decode(frame);

        auto* state = findChannel(results, "state");
        REQUIRE(state != nullptr);
        REQUIRE(state->value == 0.0);
    }
}

TEST_CASE("PD16Protocol decodes input status (AVI)", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("decodes analog voltage input") {
        // Mux: Type=AVI (4), Index=0 → 0x80
        uint32_t frameId = BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS;
        QCanBusFrame frame = makeFrame(frameId, "8001138800000000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());
        REQUIRE(hasChannel(results, "voltage"));
        REQUIRE(hasChannel(results, "state"));
    }
}

//=============================================================================
// Output Status Decoding Tests
//=============================================================================

TEST_CASE("PD16Protocol decodes output status (25A)", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("decodes 25A output status") {
        // Frame: Output Status (+4) = 0x6D4
        // Mux: Type=25A (0), Index=0 → 0x00
        // Load: 80%
        // Voltage: 12000mV (12.0V) = 0x2EE0
        // Low side current: 15000mA (15A) = 0x3A98
        // High side current: 50 (0.050A)
        // Status byte: retry=0, pinState=0
        uint32_t frameId = BASE_ID_DEVICE_A + FRAME_OFFSET_OUTPUT_STATUS;
        QCanBusFrame frame = makeFrame(frameId, "00502EE03A983200");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        // Check load
        auto* load = findChannel(results, "load");
        REQUIRE(load != nullptr);
        REQUIRE(load->value == TEST_LOAD_PERCENT);

        // Check voltage
        auto* voltage = findChannel(results, "voltage");
        REQUIRE(voltage != nullptr);
        REQUIRE_THAT(voltage->value, WithinAbs(TEST_12V_VOLTAGE, VOLTAGE_TOLERANCE));

        // Check low side current
        auto* currentLow = findChannel(results, "currentLow");
        REQUIRE(currentLow != nullptr);
        REQUIRE_THAT(currentLow->value, WithinAbs(TEST_CURRENT_A, VOLTAGE_TOLERANCE));
    }
}

TEST_CASE("PD16Protocol decodes output status (8A)", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("decodes 8A output status") {
        // Mux: Type=8A (1), Index=0 → 0x20
        uint32_t frameId = BASE_ID_DEVICE_A + FRAME_OFFSET_OUTPUT_STATUS;
        // Byte 1: retry/pinState, Bytes 2-3: voltage, Bytes 4-5: current, Byte 6: load
        QCanBusFrame frame = makeFrame(frameId, "20002EE03A985000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());
        REQUIRE(hasChannel(results, "voltage"));
        REQUIRE(hasChannel(results, "current"));
        REQUIRE(hasChannel(results, "load"));
    }
}

//=============================================================================
// Device Status Decoding Tests
//=============================================================================

TEST_CASE("PD16Protocol decodes device status", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::B);

    SECTION("decodes firmware version") {
        // Frame: Device Status (+5) for device B = 0x6DD
        // Status: 1 (in_firmware) = bits 7-4 → 0x10
        // FW version: 2.15.3 → Major=2, Minor=15, Bugfix=3
        uint32_t frameId = BASE_ID_DEVICE_B + FRAME_OFFSET_DEVICE_STATUS;
        QCanBusFrame frame = makeFrame(frameId, "10020F0300000000");
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        auto* version = findChannel(results, "firmwareVersion");
        REQUIRE(version != nullptr);
        REQUIRE_THAT(version->value, WithinAbs(TEST_FW_VERSION, FW_VERSION_TOLERANCE));
    }

    SECTION("decodes device status field") {
        uint32_t frameId = BASE_ID_DEVICE_B + FRAME_OFFSET_DEVICE_STATUS;
        QCanBusFrame frame = makeFrame(frameId, "10020F0300000000");
        auto results = protocol.decode(frame);

        auto* status = findChannel(results, "_status");
        REQUIRE(status != nullptr);
        REQUIRE(status->value == 1.0); // in_firmware status
    }
}

//=============================================================================
// IO Type Name Tests
//=============================================================================

TEST_CASE("PD16Protocol IO type names", "[pd16][protocol]") {
    using PD16 = devdash::PD16Protocol;

    REQUIRE(PD16::getIOTypeName(PD16::IOType::Output25A) == "25A");
    REQUIRE(PD16::getIOTypeName(PD16::IOType::Output8A) == "8A");
    REQUIRE(PD16::getIOTypeName(PD16::IOType::HalfBridge) == "HBO");
    REQUIRE(PD16::getIOTypeName(PD16::IOType::SpeedPulse) == "SPI");
    REQUIRE(PD16::getIOTypeName(PD16::IOType::AnalogVoltage) == "AVI");
}

//=============================================================================
// Edge Cases and Error Handling
//=============================================================================

TEST_CASE("PD16Protocol handles edge cases", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;

    SECTION("empty payload returns empty") {
        QCanBusFrame frame(BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS, QByteArray());
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("invalid frame returns empty") {
        QCanBusFrame frame;
        frame.setFrameId(BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS);
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("unhandled frame offset returns empty") {
        // Offset 0, 1, 2 are RX frames (to PD16), not decoded
        QCanBusFrame frame = makeFrame(BASE_ID_DEVICE_A + 0, "0000000000000000");
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("fallback mode works without JSON") {
        REQUIRE(protocol.usingFallback());
        
        // Should still decode with hardcoded fallback
        uint32_t frameId = BASE_ID_DEVICE_A + FRAME_OFFSET_INPUT_STATUS;
        QCanBusFrame frame = makeFrame(frameId, "6101138802EE03E8");
        auto results = protocol.decode(frame);
        REQUIRE(!results.empty());
    }
}