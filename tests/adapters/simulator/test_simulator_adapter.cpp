/**
 * @file test_simulator_adapter.cpp
 * @brief Unit tests for SimulatorAdapter.
 *
 * Tests cover:
 * - Construction and configuration
 * - Start/stop lifecycle
 * - Channel availability and retrieval
 * - Data generation and signal emission
 * - Value ranges and validity
 */

#include "adapters/simulator/SimulatorAdapter.h"

#include <QJsonObject>
#include <QSignalSpy>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;
using devdash::ChannelValue;
using devdash::SimulatorAdapter;

namespace {

//=============================================================================
// Test Constants - Configuration
//=============================================================================

constexpr const char* CONFIG_KEY_UPDATE_INTERVAL = "updateIntervalMs";
constexpr int TEST_UPDATE_INTERVAL_MS = 100;
constexpr int DEFAULT_UPDATE_INTERVAL_MS = 50;

//=============================================================================
// Test Constants - Channel Names
//=============================================================================

constexpr const char* CHANNEL_RPM = "rpm";
constexpr const char* CHANNEL_THROTTLE = "throttlePosition";
constexpr const char* CHANNEL_COOLANT_TEMP = "coolantTemperature";
constexpr const char* CHANNEL_OIL_TEMP = "oilTemperature";
constexpr const char* CHANNEL_IAT = "intakeAirTemperature";
constexpr const char* CHANNEL_OIL_PRESSURE = "oilPressure";
constexpr const char* CHANNEL_MAP = "manifoldPressure";
constexpr const char* CHANNEL_BATTERY = "batteryVoltage";
constexpr const char* CHANNEL_SPEED = "vehicleSpeed";
constexpr const char* CHANNEL_GEAR = "gear";

//=============================================================================
// Test Constants - Value Ranges
//=============================================================================

constexpr double MIN_RPM = 0.0;
constexpr double MAX_RPM = 8000.0;

constexpr double MIN_THROTTLE = 0.0;
constexpr double MAX_THROTTLE = 100.0;

constexpr double MIN_COOLANT_TEMP = 80.0;
constexpr double MAX_COOLANT_TEMP = 95.0;

constexpr double MIN_OIL_TEMP = 85.0;
constexpr double MAX_OIL_TEMP = 100.0;

constexpr double MIN_IAT = 30.0;
constexpr double MAX_IAT = 45.0;

constexpr double MIN_OIL_PRESSURE = 150.0;
constexpr double MAX_OIL_PRESSURE = 550.0;

constexpr double MIN_MAP = 25.0;
constexpr double MAX_MAP = 210.0;

constexpr double MIN_BATTERY_VOLTAGE = 13.4;
constexpr double MAX_BATTERY_VOLTAGE = 14.2;

constexpr double MIN_SPEED = 0.0;
constexpr double MAX_SPEED = 260.0;

constexpr int MIN_GEAR = 0;
constexpr int MAX_GEAR = 6;

//=============================================================================
// Test Constants - Timing
//=============================================================================

/// Time to wait for signals (ms)
constexpr int SIGNAL_WAIT_MS = 200;

/// Number of data generation cycles to test
constexpr int TEST_CYCLES = 5;

//=============================================================================
// Test Helpers
//=============================================================================

/**
 * @brief Create default test configuration.
 */
QJsonObject makeTestConfig(int updateIntervalMs = TEST_UPDATE_INTERVAL_MS) {
    QJsonObject config;
    config[CONFIG_KEY_UPDATE_INTERVAL] = updateIntervalMs;
    return config;
}

/**
 * @brief Check if a value is within expected range.
 */
bool isInRange(double value, double min, double max) {
    return value >= min && value <= max;
}

} // anonymous namespace

//=============================================================================
// Construction Tests
//=============================================================================

TEST_CASE("SimulatorAdapter construction", "[simulator][adapter]") {

    SECTION("constructs with default config") {
        QJsonObject config;
        SimulatorAdapter adapter(config);

        REQUIRE(adapter.adapterName() == "Simulator");
        REQUIRE_FALSE(adapter.isRunning());
    }

    SECTION("constructs with custom update interval") {
        auto config = makeTestConfig(TEST_UPDATE_INTERVAL_MS);
        SimulatorAdapter adapter(config);

        REQUIRE_FALSE(adapter.isRunning());
    }

    SECTION("availableChannels empty before start") {
        SimulatorAdapter adapter(QJsonObject{});

        // Channels are populated on first data generation
        REQUIRE(adapter.availableChannels().isEmpty());
    }
}

//=============================================================================
// Lifecycle Tests
//=============================================================================

TEST_CASE("SimulatorAdapter lifecycle", "[simulator][adapter]") {

    SECTION("start succeeds") {
        SimulatorAdapter adapter(makeTestConfig());

        REQUIRE(adapter.start());
        REQUIRE(adapter.isRunning());

        adapter.stop();
    }

    SECTION("start is idempotent") {
        SimulatorAdapter adapter(makeTestConfig());

        REQUIRE(adapter.start());
        REQUIRE(adapter.start()); // Second start should also return true
        REQUIRE(adapter.isRunning());

        adapter.stop();
    }

    SECTION("stop when not running is safe") {
        SimulatorAdapter adapter(makeTestConfig());

        // Should not crash or throw
        adapter.stop();
        REQUIRE_FALSE(adapter.isRunning());
    }

    SECTION("stop actually stops") {
        SimulatorAdapter adapter(makeTestConfig());

        adapter.start();
        REQUIRE(adapter.isRunning());

        adapter.stop();
        REQUIRE_FALSE(adapter.isRunning());
    }

    SECTION("can restart after stop") {
        SimulatorAdapter adapter(makeTestConfig());

        adapter.start();
        adapter.stop();
        REQUIRE(adapter.start());
        REQUIRE(adapter.isRunning());

        adapter.stop();
    }

    SECTION("emits connectionStateChanged on start") {
        SimulatorAdapter adapter(makeTestConfig());
        QSignalSpy spy(&adapter, &SimulatorAdapter::connectionStateChanged);

        adapter.start();

        REQUIRE(spy.count() == 1);
        REQUIRE(spy.at(0).at(0).toBool() == true);

        adapter.stop();
    }

    SECTION("emits connectionStateChanged on stop") {
        SimulatorAdapter adapter(makeTestConfig());
        adapter.start();

        QSignalSpy spy(&adapter, &SimulatorAdapter::connectionStateChanged);
        adapter.stop();

        REQUIRE(spy.count() == 1);
        REQUIRE(spy.at(0).at(0).toBool() == false);
    }
}

//=============================================================================
// Channel Tests
//=============================================================================

TEST_CASE("SimulatorAdapter channels", "[simulator][adapter]") {
    SimulatorAdapter adapter(makeTestConfig());
    adapter.start();

    // Wait for at least one data generation cycle
    QSignalSpy spy(&adapter, &SimulatorAdapter::channelUpdated);
    REQUIRE(spy.wait(SIGNAL_WAIT_MS));

    SECTION("all expected channels available") {
        auto channels = adapter.availableChannels();

        REQUIRE(channels.contains(CHANNEL_RPM));
        REQUIRE(channels.contains(CHANNEL_THROTTLE));
        REQUIRE(channels.contains(CHANNEL_COOLANT_TEMP));
        REQUIRE(channels.contains(CHANNEL_OIL_TEMP));
        REQUIRE(channels.contains(CHANNEL_IAT));
        REQUIRE(channels.contains(CHANNEL_OIL_PRESSURE));
        REQUIRE(channels.contains(CHANNEL_MAP));
        REQUIRE(channels.contains(CHANNEL_BATTERY));
        REQUIRE(channels.contains(CHANNEL_SPEED));
        REQUIRE(channels.contains(CHANNEL_GEAR));
    }

    SECTION("getChannel returns valid data for known channel") {
        auto rpm = adapter.getChannel(CHANNEL_RPM);

        REQUIRE(rpm.has_value());
        REQUIRE(rpm->valid);
        REQUIRE(rpm->unit == "RPM");
    }

    SECTION("getChannel returns nullopt for unknown channel") {
        auto unknown = adapter.getChannel("nonexistent");

        REQUIRE_FALSE(unknown.has_value());
    }

    adapter.stop();
}

//=============================================================================
// Data Generation Tests
//=============================================================================

TEST_CASE("SimulatorAdapter data generation", "[simulator][adapter]") {
    SimulatorAdapter adapter(makeTestConfig());
    QSignalSpy spy(&adapter, &SimulatorAdapter::channelUpdated);

    adapter.start();

    // Wait for multiple data cycles
    for (int i = 0; i < TEST_CYCLES; ++i) {
        spy.wait(SIGNAL_WAIT_MS);
    }

    SECTION("emits channelUpdated signals") {
        // Should have received many signals (10 channels * TEST_CYCLES cycles)
        REQUIRE(spy.count() >= 10);
    }

    SECTION("RPM is within valid range") {
        auto rpm = adapter.getChannel(CHANNEL_RPM);
        REQUIRE(rpm.has_value());
        REQUIRE(isInRange(rpm->value, MIN_RPM, MAX_RPM));
    }

    SECTION("throttle is within valid range") {
        auto throttle = adapter.getChannel(CHANNEL_THROTTLE);
        REQUIRE(throttle.has_value());
        REQUIRE(isInRange(throttle->value, MIN_THROTTLE, MAX_THROTTLE));
    }

    SECTION("temperatures are within valid ranges") {
        auto coolant = adapter.getChannel(CHANNEL_COOLANT_TEMP);
        REQUIRE(coolant.has_value());
        REQUIRE(isInRange(coolant->value, MIN_COOLANT_TEMP, MAX_COOLANT_TEMP));

        auto oilTemp = adapter.getChannel(CHANNEL_OIL_TEMP);
        REQUIRE(oilTemp.has_value());
        REQUIRE(isInRange(oilTemp->value, MIN_OIL_TEMP, MAX_OIL_TEMP));

        auto iat = adapter.getChannel(CHANNEL_IAT);
        REQUIRE(iat.has_value());
        REQUIRE(isInRange(iat->value, MIN_IAT, MAX_IAT));
    }

    SECTION("pressures are within valid ranges") {
        auto oilPressure = adapter.getChannel(CHANNEL_OIL_PRESSURE);
        REQUIRE(oilPressure.has_value());
        REQUIRE(isInRange(oilPressure->value, MIN_OIL_PRESSURE, MAX_OIL_PRESSURE));

        auto map = adapter.getChannel(CHANNEL_MAP);
        REQUIRE(map.has_value());
        REQUIRE(isInRange(map->value, MIN_MAP, MAX_MAP));
    }

    SECTION("battery voltage is within valid range") {
        auto battery = adapter.getChannel(CHANNEL_BATTERY);
        REQUIRE(battery.has_value());
        REQUIRE(isInRange(battery->value, MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE));
    }

    SECTION("speed is within valid range") {
        auto speed = adapter.getChannel(CHANNEL_SPEED);
        REQUIRE(speed.has_value());
        REQUIRE(isInRange(speed->value, MIN_SPEED, MAX_SPEED));
    }

    SECTION("gear is within valid range") {
        auto gear = adapter.getChannel(CHANNEL_GEAR);
        REQUIRE(gear.has_value());
        REQUIRE(isInRange(gear->value, MIN_GEAR, MAX_GEAR));
    }

    SECTION("all channel values are marked valid") {
        auto channels = adapter.availableChannels();
        for (const auto& name : channels) {
            auto value = adapter.getChannel(name);
            REQUIRE(value.has_value());
            REQUIRE(value->valid);
        }
    }

    adapter.stop();
}

//=============================================================================
// Unit String Tests
//=============================================================================

TEST_CASE("SimulatorAdapter units", "[simulator][adapter]") {
    SimulatorAdapter adapter(makeTestConfig());
    adapter.start();

    QSignalSpy spy(&adapter, &SimulatorAdapter::channelUpdated);
    spy.wait(SIGNAL_WAIT_MS);

    SECTION("RPM has correct unit") {
        auto rpm = adapter.getChannel(CHANNEL_RPM);
        REQUIRE(rpm->unit == "RPM");
    }

    SECTION("throttle has correct unit") {
        auto throttle = adapter.getChannel(CHANNEL_THROTTLE);
        REQUIRE(throttle->unit == "%");
    }

    SECTION("temperatures have correct unit") {
        auto coolant = adapter.getChannel(CHANNEL_COOLANT_TEMP);
        REQUIRE(coolant->unit == QString::fromUtf8("°C"));
    }

    SECTION("pressures have correct unit") {
        auto oilPressure = adapter.getChannel(CHANNEL_OIL_PRESSURE);
        REQUIRE(oilPressure->unit == "kPa");
    }

    SECTION("battery voltage has correct unit") {
        auto battery = adapter.getChannel(CHANNEL_BATTERY);
        REQUIRE(battery->unit == "V");
    }

    SECTION("speed has correct unit") {
        auto speed = adapter.getChannel(CHANNEL_SPEED);
        REQUIRE(speed->unit == "km/h");
    }

    SECTION("gear has empty unit") {
        auto gear = adapter.getChannel(CHANNEL_GEAR);
        REQUIRE(gear->unit.isEmpty());
    }

    adapter.stop();
}