#include "core/DataBroker.h"
#include "core/IProtocolAdapter.h"

#include <QSignalSpy>

#include <catch2/catch_test_macros.hpp>

namespace {

// Mock adapter for testing DataBroker
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

    // Test helper to emit channel updates
    void emitChannelUpdate(const QString& name, double value, const QString& unit) {
        devdash::ChannelValue cv{value, unit, true};
        m_channels[name] = cv;
        emit channelUpdated(name, cv);
    }

  private:
    bool m_running{false};
    QHash<QString, devdash::ChannelValue> m_channels;
};

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

TEST_CASE("DataBroker receives channel updates", "[core][databroker]") {
    devdash::DataBroker broker;
    auto* mockAdapter = new MockAdapter();
    broker.setAdapter(std::unique_ptr<devdash::IProtocolAdapter>(mockAdapter));

    REQUIRE(broker.start());
    REQUIRE(broker.isConnected());

    SECTION("RPM updates") {
        QSignalSpy spy(&broker, &devdash::DataBroker::rpmChanged);
        mockAdapter->emitChannelUpdate("rpm", 3500.0, "RPM");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.rpm() == 3500.0);
    }

    SECTION("throttle position updates") {
        QSignalSpy spy(&broker, &devdash::DataBroker::throttlePositionChanged);
        mockAdapter->emitChannelUpdate("throttlePosition", 75.5, "%");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.throttlePosition() == 75.5);
    }

    SECTION("coolant temperature updates") {
        QSignalSpy spy(&broker, &devdash::DataBroker::coolantTemperatureChanged);
        mockAdapter->emitChannelUpdate("coolantTemperature", 92.3, "°C");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.coolantTemperature() == 92.3);
    }

    SECTION("gear updates") {
        QSignalSpy spy(&broker, &devdash::DataBroker::gearChanged);
        mockAdapter->emitChannelUpdate("gear", 3.0, "");

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.gear() == 3);
    }

    SECTION("duplicate values don't emit signals") {
        mockAdapter->emitChannelUpdate("rpm", 3500.0, "RPM");

        QSignalSpy spy(&broker, &devdash::DataBroker::rpmChanged);
        mockAdapter->emitChannelUpdate("rpm", 3500.0, "RPM");

        REQUIRE(spy.count() == 0);
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

#include "test_data_broker.moc"
