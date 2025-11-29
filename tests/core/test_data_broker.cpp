#include "core/DataBroker.h"
#include "core/IProtocolAdapter.h"

#include <QJsonObject>
#include <QSignalSpy>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

// Test constants - no magic numbers
constexpr double TEST_RPM_VALUE = 3500.0;
constexpr double TEST_THROTTLE_PERCENT = 75.5;
constexpr double TEST_COOLANT_TEMP_CELSIUS = 92.3;
constexpr int TEST_GEAR_VALUE = 3;
constexpr double TEST_GEAR_AS_DOUBLE = 3.0;

/**
 * @brief Mock adapter for testing DataBroker.
 *
 * Provides controlled channel emission for verifying DataBroker
 * correctly processes and maps incoming telemetry data.
 */
class MockAdapter : public devdash::IProtocolAdapter {
    Q_OBJECT

public:
    explicit MockAdapter(QObject* parent = nullptr) : devdash::IProtocolAdapter(parent) {}

    [[nodiscard]] bool start() override {
        m_running = true;
        emit connectionStateChanged(true);
        return true;
    }

    void stop() override {
        m_running = false;
        emit connectionStateChanged(false);
    }

    [[nodiscard]] bool isRunning() const override { return m_running; }

    [[nodiscard]] std::optional<devdash::ChannelValue>
    getChannel(const QString& channelName) const override {
        auto it = m_channels.find(channelName);
        if (it != m_channels.end()) {
            return *it;
        }
        return std::nullopt;
    }

    [[nodiscard]] QStringList availableChannels() const override { return m_channels.keys(); }

    [[nodiscard]] QString adapterName() const override { return QStringLiteral("Mock"); }

    /**
     * @brief Emit a channel update for testing.
     * @param name Channel name as it would come from the protocol
     * @param value Decoded value
     * @param unit Unit string for display
     */
    void emitChannelUpdate(const QString& name, double value, const QString& unit) {
        devdash::ChannelValue channelValue{value, unit, true};
        m_channels[name] = channelValue;
        emit channelUpdated(name, channelValue);
    }

private:
    bool m_running{false};
    QHash<QString, devdash::ChannelValue> m_channels;
};

/**
 * @brief Create a test profile with Haltech-style channel mappings.
 * @return JSON object suitable for DataBroker::loadProfileFromJson()
 */
QJsonObject createHaltechTestProfile() {
    QJsonObject mappings;
    mappings["RPM"] = "rpm";
    mappings["TPS"] = "throttlePosition";
    mappings["MAP"] = "manifoldPressure";
    mappings["ECT"] = "coolantTemperature";
    mappings["Engine Oil Temperature"] = "oilTemperature";
    mappings["IAT"] = "intakeAirTemperature";
    mappings["Engine Oil Pressure"] = "oilPressure";
    mappings["Fuel Pressure"] = "fuelPressure";
    mappings["Fuel Level"] = "fuelLevel";
    mappings["Wideband Overall"] = "airFuelRatio";
    mappings["Battery Voltage"] = "batteryVoltage";
    mappings["Vehicle Speed"] = "vehicleSpeed";
    mappings["Gear"] = "gear";

    QJsonObject profile;
    profile["channelMappings"] = mappings;
    return profile;
}

/**
 * @brief Create a minimal test profile with only a few mappings.
 * @return JSON object with subset of channel mappings
 */
QJsonObject createMinimalTestProfile() {
    QJsonObject mappings;
    mappings["rpm"] = "rpm";
    mappings["throttlePosition"] = "throttlePosition";
    mappings["gear"] = "gear";

    QJsonObject profile;
    profile["channelMappings"] = mappings;
    return profile;
}

} // namespace

TEST_CASE("DataBroker initialization", "[core][databroker]") {
    devdash::DataBroker broker;

    SECTION("initial values are zero") {
        REQUIRE(broker.rpm() == 0.0);
        REQUIRE(broker.throttlePosition() == 0.0);
        REQUIRE(broker.coolantTemperature() == 0.0);
        REQUIRE(broker.vehicleSpeed() == 0.0);
        REQUIRE(broker.gear() == 0);
        REQUIRE_FALSE(broker.isConnected());
    }

    SECTION("start fails without adapter") {
        REQUIRE_FALSE(broker.start());
    }
}

TEST_CASE("DataBroker profile loading", "[core][databroker]") {
    devdash::DataBroker broker;

    SECTION("loads valid profile from JSON") {
        auto profile = createHaltechTestProfile();
        REQUIRE(broker.loadProfileFromJson(profile));
    }

    SECTION("handles empty channelMappings gracefully") {
        QJsonObject profile;
        profile["channelMappings"] = QJsonObject{};
        REQUIRE(broker.loadProfileFromJson(profile));
    }

    SECTION("handles missing channelMappings gracefully") {
        QJsonObject profile;
        profile["name"] = "Test Profile";
        // No channelMappings key
        REQUIRE(broker.loadProfileFromJson(profile));
    }

    SECTION("rejects invalid channelMappings type") {
        QJsonObject profile;
        profile["channelMappings"] = "not an object";
        REQUIRE_FALSE(broker.loadProfileFromJson(profile));
    }

    SECTION("skips unknown property names with warning") {
        QJsonObject mappings;
        mappings["RPM"] = "rpm";  // Valid
        mappings["UNKNOWN_CHANNEL"] = "nonExistentProperty";  // Invalid target

        QJsonObject profile;
        profile["channelMappings"] = mappings;

        // Should succeed but log a warning
        REQUIRE(broker.loadProfileFromJson(profile));
    }
}

TEST_CASE("DataBroker receives channel updates with profile mapping", "[core][databroker]") {
    devdash::DataBroker broker;

    // Load Haltech-style profile where protocol uses abbreviations
    auto profile = createHaltechTestProfile();
    REQUIRE(broker.loadProfileFromJson(profile));

    auto* mockAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(mockAdapter));

    REQUIRE(broker.start());
    REQUIRE(broker.isConnected());

    SECTION("RPM updates via mapped channel name") {
        QSignalSpy spy(&broker, &devdash::DataBroker::rpmChanged);

        // Haltech sends "RPM", profile maps it to "rpm" property
        mockAdapter->emitChannelUpdate("RPM", TEST_RPM_VALUE, "RPM");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.rpm() == TEST_RPM_VALUE);
    }

    SECTION("throttle position updates via TPS abbreviation") {
        QSignalSpy spy(&broker, &devdash::DataBroker::throttlePositionChanged);

        // Haltech sends "TPS", profile maps it to "throttlePosition"
        mockAdapter->emitChannelUpdate("TPS", TEST_THROTTLE_PERCENT, "%");

        REQUIRE(spy.count() == 1);
        REQUIRE_THAT(broker.throttlePosition(),
                     Catch::Matchers::WithinRel(TEST_THROTTLE_PERCENT));
    }

    SECTION("coolant temperature updates via ECT abbreviation") {
        QSignalSpy spy(&broker, &devdash::DataBroker::coolantTemperatureChanged);

        // Haltech sends "ECT", profile maps it to "coolantTemperature"
        mockAdapter->emitChannelUpdate("ECT", TEST_COOLANT_TEMP_CELSIUS, "°C");

        REQUIRE(spy.count() == 1);
        REQUIRE_THAT(broker.coolantTemperature(),
                     Catch::Matchers::WithinRel(TEST_COOLANT_TEMP_CELSIUS));
    }

    SECTION("gear updates") {
        QSignalSpy spy(&broker, &devdash::DataBroker::gearChanged);
        mockAdapter->emitChannelUpdate("Gear", TEST_GEAR_AS_DOUBLE, "");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.gear() == TEST_GEAR_VALUE);
    }

    SECTION("duplicate values don't emit signals") {
        mockAdapter->emitChannelUpdate("RPM", TEST_RPM_VALUE, "RPM");

        QSignalSpy spy(&broker, &devdash::DataBroker::rpmChanged);
        mockAdapter->emitChannelUpdate("RPM", TEST_RPM_VALUE, "RPM");

        REQUIRE(spy.count() == 0);
    }

    SECTION("unmapped channels are silently ignored") {
        QSignalSpy rpmSpy(&broker, &devdash::DataBroker::rpmChanged);
        QSignalSpy throttleSpy(&broker, &devdash::DataBroker::throttlePositionChanged);

        // Send a channel that isn't in the profile mapping
        mockAdapter->emitChannelUpdate("SomeUnknownChannel", 42.0, "units");

        // No signals should fire
        REQUIRE(rpmSpy.count() == 0);
        REQUIRE(throttleSpy.count() == 0);
    }
}

TEST_CASE("DataBroker works with direct channel names", "[core][databroker]") {
    devdash::DataBroker broker;

    // Load minimal profile where protocol sends standard names directly
    auto profile = createMinimalTestProfile();
    REQUIRE(broker.loadProfileFromJson(profile));

    auto* mockAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(mockAdapter));

    REQUIRE(broker.start());

    SECTION("direct channel names work") {
        QSignalSpy spy(&broker, &devdash::DataBroker::rpmChanged);

        // Protocol sends "rpm" directly, mapped 1:1
        mockAdapter->emitChannelUpdate("rpm", TEST_RPM_VALUE, "RPM");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.rpm() == TEST_RPM_VALUE);
    }
}

TEST_CASE("DataBroker handles no profile loaded", "[core][databroker]") {
    devdash::DataBroker broker;

    // Don't load any profile - mappings will be empty
    auto* mockAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(mockAdapter));

    REQUIRE(broker.start());  // Should succeed with warning

    SECTION("all channel updates are ignored without mappings") {
        QSignalSpy rpmSpy(&broker, &devdash::DataBroker::rpmChanged);

        mockAdapter->emitChannelUpdate("RPM", TEST_RPM_VALUE, "RPM");
        mockAdapter->emitChannelUpdate("rpm", TEST_RPM_VALUE, "RPM");

        // Nothing should be processed
        REQUIRE(rpmSpy.count() == 0);
        REQUIRE(broker.rpm() == 0.0);
    }
}

TEST_CASE("DataBroker connection state", "[core][databroker]") {
    devdash::DataBroker broker;
    auto* mockAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(mockAdapter));

    QSignalSpy spy(&broker, &devdash::DataBroker::isConnectedChanged);

    SECTION("connects when adapter starts") {
        broker.start();
        REQUIRE(spy.count() == 1);
        REQUIRE(broker.isConnected());
    }

    SECTION("disconnects when adapter stops") {
        broker.start();
        spy.clear();

        broker.stop();
        REQUIRE(spy.count() == 1);
        REQUIRE_FALSE(broker.isConnected());
    }
}

TEST_CASE("DataBroker adapter replacement", "[core][databroker]") {
    devdash::DataBroker broker;

    auto profile = createMinimalTestProfile();
    REQUIRE(broker.loadProfileFromJson(profile));

    // First adapter
    auto* firstAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(firstAdapter));
    broker.start();

    firstAdapter->emitChannelUpdate("rpm", TEST_RPM_VALUE, "RPM");
    REQUIRE(broker.rpm() == TEST_RPM_VALUE);

    SECTION("replacing adapter stops old one and clears connection") {
        QSignalSpy spy(&broker, &devdash::DataBroker::isConnectedChanged);

        // Replace with new adapter
        auto* secondAdapter = new MockAdapter();
        broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(secondAdapter));

        // Old adapter should have been stopped
        REQUIRE_FALSE(broker.isConnected());
    }

    SECTION("new adapter works after replacement") {
        constexpr double NEW_RPM_VALUE = 5000.0;

        auto* secondAdapter = new MockAdapter();
        broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(secondAdapter));
        broker.start();

        secondAdapter->emitChannelUpdate("rpm", NEW_RPM_VALUE, "RPM");
        REQUIRE(broker.rpm() == NEW_RPM_VALUE);
    }
}

TEST_CASE("DataBroker invalid channel values", "[core][databroker]") {
    devdash::DataBroker broker;

    auto profile = createMinimalTestProfile();
    REQUIRE(broker.loadProfileFromJson(profile));

    auto* mockAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(mockAdapter));
    broker.start();

    SECTION("invalid channel values are ignored") {
        // Set initial valid value
        mockAdapter->emitChannelUpdate("rpm", TEST_RPM_VALUE, "RPM");
        REQUIRE(broker.rpm() == TEST_RPM_VALUE);

        // Send invalid value - should be ignored
        devdash::ChannelValue invalidValue{0.0, "RPM", false};  // valid = false
        emit mockAdapter->channelUpdated("rpm", invalidValue);

        // Value should remain unchanged
        REQUIRE(broker.rpm() == TEST_RPM_VALUE);
    }
}

#include "test_data_broker.moc"