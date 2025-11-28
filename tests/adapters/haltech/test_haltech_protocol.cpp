#include "adapters/haltech/HaltechProtocol.h"

#include <QCanBusFrame>
#include <QCoreApplication>
#include <QDir>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace {
// Helper to find protocol file relative to source directory
QString findProtocolFile() {
    // Try various paths depending on where tests are run from
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
} // namespace

TEST_CASE("HaltechProtocol decodes uint16 correctly", "[haltech][protocol]") {
    SECTION("big-endian decoding") {
        QByteArray payload = QByteArray::fromHex("0DAC03E8");

        auto value = devdash::HaltechProtocol::decodeUint16(payload, 0);
        REQUIRE(value == 0x0DAC); // 3500

        auto value2 = devdash::HaltechProtocol::decodeUint16(payload, 2);
        REQUIRE(value2 == 0x03E8); // 1000
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

TEST_CASE("HaltechProtocol decodes RPM correctly", "[haltech][protocol]") {
    SECTION("normal RPM value") {
        // 3500 RPM = 0x0DAC in big-endian
        QByteArray payload = QByteArray::fromHex("0DAC000000000000");
        auto rpm = devdash::HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == 3500.0);
    }

    SECTION("idle RPM") {
        // 800 RPM = 0x0320 in big-endian
        QByteArray payload = QByteArray::fromHex("0320000000000000");
        auto rpm = devdash::HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == 800.0);
    }

    SECTION("high RPM") {
        // 7500 RPM = 0x1D4C in big-endian
        QByteArray payload = QByteArray::fromHex("1D4C000000000000");
        auto rpm = devdash::HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == 7500.0);
    }
}

TEST_CASE("HaltechProtocol decodes temperature correctly", "[haltech][protocol]") {
    SECTION("normal coolant temperature") {
        // 90°C = 363.15K * 10 = 3631.5 ≈ 3632 = 0x0E30
        // Actually: (temp_kelvin * 10) stored
        // 90°C = 363.15K, 363.15 * 10 = 3631.5
        QByteArray payload = QByteArray::fromHex("0E30");
        auto temp = devdash::HaltechProtocol::decodeTemperature(payload, 0);
        REQUIRE_THAT(temp, WithinAbs(90.0, 1.0)); // Within 1°C
    }

    SECTION("cold temperature") {
        // 20°C = 293.15K * 10 = 2931.5 ≈ 2932 = 0x0B74
        QByteArray payload = QByteArray::fromHex("0B74");
        auto temp = devdash::HaltechProtocol::decodeTemperature(payload, 0);
        REQUIRE_THAT(temp, WithinAbs(20.0, 1.0));
    }

    SECTION("freezing temperature") {
        // 0°C = 273.15K * 10 = 2731.5 ≈ 2732 = 0x0AAC
        QByteArray payload = QByteArray::fromHex("0AAC");
        auto temp = devdash::HaltechProtocol::decodeTemperature(payload, 0);
        REQUIRE_THAT(temp, WithinAbs(0.0, 1.0));
    }
}

TEST_CASE("HaltechProtocol decodes pressure correctly", "[haltech][protocol]") {
    SECTION("atmospheric pressure") {
        // 101.3 kPa * 10 = 1013 = 0x03F5
        QByteArray payload = QByteArray::fromHex("03F5");
        auto pressure = devdash::HaltechProtocol::decodePressure(payload, 0);
        REQUIRE_THAT(pressure, WithinAbs(101.3, 0.1));
    }

    SECTION("boost pressure") {
        // 200 kPa * 10 = 2000 = 0x07D0
        QByteArray payload = QByteArray::fromHex("07D0");
        auto pressure = devdash::HaltechProtocol::decodePressure(payload, 0);
        REQUIRE(pressure == 200.0);
    }
}

TEST_CASE("HaltechProtocol decodes CAN frames", "[haltech][protocol]") {
    devdash::HaltechProtocol protocol;

    SECTION("RPM/TPS frame (0x360)") {
        // Frame ID 0x360: RPM = 3500, TPS = 50%
        // RPM: 0x0DAC (3500), TPS: 0x01F4 (500 = 50.0%)
        QCanBusFrame frame(0x360, QByteArray::fromHex("0DAC01F400000000"));
        auto results = protocol.decode(frame);

        REQUIRE(results.size() == 2);

        // Check RPM
        bool foundRpm = false;
        bool foundTps = false;
        for (const auto& [name, value] : results) {
            if (name == "rpm") {
                REQUIRE(value.value == 3500.0);
                REQUIRE(value.valid);
                foundRpm = true;
            }
            if (name == "throttlePosition") {
                REQUIRE(value.value == 50.0);
                REQUIRE(value.valid);
                foundTps = true;
            }
        }
        REQUIRE(foundRpm);
        REQUIRE(foundTps);
    }

    SECTION("invalid frame returns empty") {
        QCanBusFrame frame;
        frame.setFrameId(0x360);
        // Invalid frame (not set as valid)
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("unknown frame ID returns empty") {
        QCanBusFrame frame(0x999, QByteArray::fromHex("0DAC01F400000000"));
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }

    SECTION("short payload handled gracefully") {
        QCanBusFrame frame(0x360, QByteArray::fromHex("0D")); // Only 1 byte
        auto results = protocol.decode(frame);
        REQUIRE(results.empty());
    }
}

TEST_CASE("HaltechProtocol conversion functions", "[haltech][protocol]") {
    SECTION("parseConversion identifies patterns") {
        using devdash::ConversionType;
        using devdash::HaltechProtocol;

        REQUIRE(HaltechProtocol::parseConversion("x") == ConversionType::Identity);
        REQUIRE(HaltechProtocol::parseConversion("x / 10") == ConversionType::DivideBy10);
        REQUIRE(HaltechProtocol::parseConversion("x / 1000") == ConversionType::DivideBy1000);
        REQUIRE(HaltechProtocol::parseConversion("(x / 10) - 101.3") ==
                ConversionType::GaugePressure);
    }

    SECTION("applyConversion calculates correctly") {
        using devdash::ConversionType;
        using devdash::HaltechProtocol;

        REQUIRE(HaltechProtocol::applyConversion(ConversionType::Identity, 100.0) == 100.0);
        REQUIRE(HaltechProtocol::applyConversion(ConversionType::DivideBy10, 1000.0) == 100.0);
        REQUIRE(HaltechProtocol::applyConversion(ConversionType::DivideBy1000, 1000.0) == 1.0);

        // Gauge pressure: (x / 10) - 101.3
        // 2013 raw = 201.3 kPa absolute = 100.0 kPa gauge
        REQUIRE_THAT(HaltechProtocol::applyConversion(ConversionType::GaugePressure, 2013.0),
                     WithinAbs(100.0, 0.1));

        // Kelvin to Celsius: (x / 10) - 273.15
        // 3632 raw = 363.2 K = 90.05°C
        REQUIRE_THAT(HaltechProtocol::applyConversion(ConversionType::KelvinToCelsius, 3632.0),
                     WithinAbs(90.0, 0.1));
    }
}

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
        REQUIRE(frameIds.contains(0x360)); // Engine Core 1
        REQUIRE(frameIds.contains(0x361)); // Pressures
        REQUIRE(frameIds.contains(0x3E0)); // Temperatures 1
    }
}

TEST_CASE("HaltechProtocol decodes with JSON definition", "[haltech][protocol][json]") {
    QString protocolPath = findProtocolFile();
    if (protocolPath.isEmpty()) {
        SKIP("Protocol file not found - run tests from project root");
    }

    devdash::HaltechProtocol protocol;
    REQUIRE(protocol.loadDefinition(protocolPath));

    SECTION("decodes RPM from 0x360") {
        // 0x360: RPM=3500 (0x0DAC), MAP=1013 (101.3 kPa), TPS=500 (50%), Coolant Pressure=0
        QCanBusFrame frame(0x360, QByteArray::fromHex("0DAC03F501F40000"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        // Find RPM channel
        bool foundRpm = false;
        for (const auto& [name, value] : results) {
            if (name.toLower().contains("rpm")) {
                REQUIRE(value.value == 3500.0);
                foundRpm = true;
                break;
            }
        }
        REQUIRE(foundRpm);
    }

    SECTION("decodes temperatures from 0x3E0 with Kelvin conversion") {
        // 0x3E0: Coolant=3632 (90°C), Air=2932 (20°C), Fuel, Oil temps
        // 3632 = 363.2K = 90.05°C, 2932 = 293.2K = 20.05°C
        QCanBusFrame frame(0x3E0, QByteArray::fromHex("0E300B740B740E30"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        // Find coolant temperature
        bool foundCoolant = false;
        for (const auto& [name, value] : results) {
            if (name.toLower().contains("coolant")) {
                REQUIRE_THAT(value.value, WithinAbs(90.0, 1.0));
                REQUIRE(value.unit == QString::fromUtf8("°C"));
                foundCoolant = true;
                break;
            }
        }
        REQUIRE(foundCoolant);
    }

    SECTION("decodes gauge pressure from 0x361") {
        // 0x361: Fuel pressure, Oil pressure (gauge = absolute - 101.3)
        // Raw 2013 = 201.3 kPa absolute = 100.0 kPa gauge
        QCanBusFrame frame(0x361, QByteArray::fromHex("07DD07DD00000000"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        // Fuel pressure should be gauge pressure
        bool foundFuel = false;
        for (const auto& [name, value] : results) {
            if (name.toLower().contains("fuel") && name.toLower().contains("pressure")) {
                // 0x07DD = 2013, gauge = (2013/10) - 101.3 = 100.0
                REQUIRE_THAT(value.value, WithinAbs(100.0, 0.1));
                foundFuel = true;
                break;
            }
        }
        REQUIRE(foundFuel);
    }

    SECTION("decodes battery voltage from 0x372") {
        // 0x372: Battery voltage=14.2V (142 = 0x008E)
        QCanBusFrame frame(0x372, QByteArray::fromHex("008E000000000000"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        bool foundBattery = false;
        for (const auto& [name, value] : results) {
            if (name.toLower().contains("battery") && name.toLower().contains("voltage")) {
                REQUIRE_THAT(value.value, WithinAbs(14.2, 0.1));
                foundBattery = true;
                break;
            }
        }
        REQUIRE(foundBattery);
    }

    SECTION("decodes fuel level from 0x3E2") {
        // 0x3E2: Fuel level=50.5L (505 = 0x01F9)
        QCanBusFrame frame(0x3E2, QByteArray::fromHex("01F9000000000000"));
        auto results = protocol.decode(frame);

        REQUIRE(!results.empty());

        bool foundFuelLevel = false;
        for (const auto& [name, value] : results) {
            if (name.toLower().contains("fuel") && name.toLower().contains("level")) {
                REQUIRE_THAT(value.value, WithinAbs(50.5, 0.1));
                foundFuelLevel = true;
                break;
            }
        }
        REQUIRE(foundFuelLevel);
    }
}
