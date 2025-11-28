#include "adapters/haltech/PD16Protocol.h"

#include <QCanBusFrame>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("PD16Protocol device configuration", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;

    SECTION("default device is A") {
        REQUIRE(protocol.deviceId() == devdash::PD16Protocol::DeviceId::A);
        REQUIRE(protocol.baseId() == 0x6D0);
    }

    SECTION("can set device B") {
        protocol.setDeviceId(devdash::PD16Protocol::DeviceId::B);
        REQUIRE(protocol.deviceId() == devdash::PD16Protocol::DeviceId::B);
        REQUIRE(protocol.baseId() == 0x6D8);
    }

    SECTION("can set device C") {
        protocol.setDeviceId(devdash::PD16Protocol::DeviceId::C);
        REQUIRE(protocol.baseId() == 0x6E0);
    }

    SECTION("can set device D") {
        protocol.setDeviceId(devdash::PD16Protocol::DeviceId::D);
        REQUIRE(protocol.baseId() == 0x6E8);
    }
}

TEST_CASE("PD16Protocol mux byte decoding", "[pd16][protocol]") {
    SECTION("extracts IO type correctly") {
        // Type in bits 7-5
        REQUIRE(devdash::PD16Protocol::getMuxType(0b00000000) ==
                devdash::PD16Protocol::IOType::Output25A);
        REQUIRE(devdash::PD16Protocol::getMuxType(0b00100000) ==
                devdash::PD16Protocol::IOType::Output8A);
        REQUIRE(devdash::PD16Protocol::getMuxType(0b01000000) ==
                devdash::PD16Protocol::IOType::HalfBridge);
        REQUIRE(devdash::PD16Protocol::getMuxType(0b01100000) ==
                devdash::PD16Protocol::IOType::SpeedPulse);
        REQUIRE(devdash::PD16Protocol::getMuxType(0b10000000) ==
                devdash::PD16Protocol::IOType::AnalogVoltage);
    }

    SECTION("extracts IO index correctly") {
        // Index in bits 3-0
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b00000000) == 0);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b00000101) == 5);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b00001111) == 15);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(0b11110011) == 3); // Ignore upper bits
    }

    SECTION("combines type and index") {
        // Type=SPI (3=0b011), Index=2 → 0b01100010 = 0x62
        uint8_t muxByte = 0x62;
        REQUIRE(devdash::PD16Protocol::getMuxType(muxByte) ==
                devdash::PD16Protocol::IOType::SpeedPulse);
        REQUIRE(devdash::PD16Protocol::getMuxIndex(muxByte) == 2);
    }
}

TEST_CASE("PD16Protocol ignores wrong device frames", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A); // Base 0x6D0

    SECTION("ignores device B frames") {
        QCanBusFrame frame(0x6D8 + 3, QByteArray::fromHex("6000000000000000")); // Device B
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("ignores ECU frames") {
        QCanBusFrame frame(0x360, QByteArray::fromHex("0DAC000000000000")); // ECU frame
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("accepts device A frames") {
        QCanBusFrame frame(0x6D0 + 3, QByteArray::fromHex("6000000000000000")); // Device A
        auto results = protocol.decode(frame);
        REQUIRE(!results.empty()); // Should decode
    }
}

TEST_CASE("PD16Protocol decodes input status (SPI)", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("decodes SPI input with voltage and frequency") {
        // Frame: Input Status (+3) = 0x6D3
        // Mux: Type=SPI (3=0b011), Index=1 → 0b01100001 = 0x61
        // State: ON (bit 0 = 1)
        // Voltage: 5000mV (5.0V) = 0x1388
        // Duty: 750 (75.0%) = 0x02EE
        // Frequency: 1000 Hz = 0x03E8
        QCanBusFrame frame(0x6D3, QByteArray::fromHex("6101138802EE03E8"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        // Check for state
        bool foundState = false;
        for (const auto& [name, value] : results) {
            if (name.contains("state")) {
                REQUIRE(value.value == 1.0); // ON
                foundState = true;
            }
            if (name.contains("voltage")) {
                REQUIRE_THAT(value.value, WithinAbs(5.0, 0.01));
            }
            if (name.contains("dutyCycle")) {
                REQUIRE_THAT(value.value, WithinAbs(75.0, 0.1));
            }
            if (name.contains("frequency")) {
                REQUIRE(value.value == 1000.0);
            }
        }
        REQUIRE(foundState);
    }
}

TEST_CASE("PD16Protocol decodes output status (25A)", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::A);

    SECTION("decodes 25A output status") {
        // Frame: Output Status (+4) = 0x6D4
        // Mux: Type=25A (0), Index=0 → 0x00
        // Load: 80%
        // Voltage: 12000mV (12.0V) = 0x2EE0
        // Low side current: 15000mA (15A) = 0x3A98
        // High side current: 50 (encoded as single byte, mA)
        // Retry count: 0, Pin state: 0 (operational)
        QCanBusFrame frame(0x6D4, QByteArray::fromHex("00502EE03A983200"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        bool foundLoad = false;
        bool foundVoltage = false;
        for (const auto& [name, value] : results) {
            if (name.contains("load")) {
                REQUIRE(value.value == 80.0);
                foundLoad = true;
            }
            if (name.contains("voltage")) {
                REQUIRE_THAT(value.value, WithinAbs(12.0, 0.01));
                foundVoltage = true;
            }
            if (name.contains("currentLow")) {
                REQUIRE_THAT(value.value, WithinAbs(15.0, 0.01));
            }
        }
        REQUIRE(foundLoad);
        REQUIRE(foundVoltage);
    }
}

TEST_CASE("PD16Protocol decodes device status", "[pd16][protocol]") {
    devdash::PD16Protocol protocol;
    protocol.setDeviceId(devdash::PD16Protocol::DeviceId::B);

    SECTION("decodes firmware version") {
        // Frame: Device Status (+5) = 0x6D8 + 5 = 0x6DD
        // Status: 1 (in_firmware) = bits 7-4 = 0x10
        // FW version: 2.15.3 → Major=2 (byte 1 bits 1-0), Minor=15, Bugfix=3
        QCanBusFrame frame(0x6DD, QByteArray::fromHex("10020F0300000000"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        bool foundVersion = false;
        for (const auto& [name, value] : results) {
            if (name.contains("firmwareVersion")) {
                // Version encoded as 2.1503 (2 + 15/100 + 3/10000)
                REQUIRE_THAT(value.value, WithinAbs(2.1503, 0.0001));
                foundVersion = true;
            }
        }
        REQUIRE(foundVersion);
    }
}

TEST_CASE("PD16Protocol IO type names", "[pd16][protocol]") {
    using devdash::PD16Protocol;

    REQUIRE(PD16Protocol::getIOTypeName(PD16Protocol::IOType::Output25A) == "25A");
    REQUIRE(PD16Protocol::getIOTypeName(PD16Protocol::IOType::Output8A) == "8A");
    REQUIRE(PD16Protocol::getIOTypeName(PD16Protocol::IOType::HalfBridge) == "HBO");
    REQUIRE(PD16Protocol::getIOTypeName(PD16Protocol::IOType::SpeedPulse) == "SPI");
    REQUIRE(PD16Protocol::getIOTypeName(PD16Protocol::IOType::AnalogVoltage) == "AVI");
}
