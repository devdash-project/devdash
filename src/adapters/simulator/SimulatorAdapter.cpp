#include "SimulatorAdapter.h"

#include <QRandomGenerator>

#include <cmath>

namespace devdash {

SimulatorAdapter::SimulatorAdapter(const QJsonObject& config, QObject* parent)
    : IProtocolAdapter(parent) {
    m_updateIntervalMs = config["updateIntervalMs"].toInt(50);

    connect(&m_timer, &QTimer::timeout, this, &SimulatorAdapter::generateData);
}

SimulatorAdapter::~SimulatorAdapter() {
    stop();
}

bool SimulatorAdapter::start() {
    if (m_running) {
        return true;
    }

    // Initialize with idle values
    m_simulatedRpm = 800.0;
    m_simulatedThrottle = 0.0;
    m_rpmTarget = 800.0;

    m_timer.start(m_updateIntervalMs);
    m_running = true;
    emit connectionStateChanged(true);

    return true;
}

void SimulatorAdapter::stop() {
    if (!m_running) {
        return;
    }

    m_timer.stop();
    m_running = false;
    emit connectionStateChanged(false);
}

bool SimulatorAdapter::isRunning() const {
    return m_running;
}

std::optional<ChannelValue> SimulatorAdapter::getChannel(const QString& channelName) const {
    auto it = m_channels.find(channelName);
    if (it != m_channels.end()) {
        return *it;
    }
    return std::nullopt;
}

QStringList SimulatorAdapter::availableChannels() const {
    return m_channels.keys();
}

QString SimulatorAdapter::adapterName() const {
    return QStringLiteral("Simulator");
}

void SimulatorAdapter::generateData() {
    auto* rng = QRandomGenerator::global();

    // Simulate throttle behavior (random acceleration/deceleration)
    if (rng->bounded(100) < 5) {
        m_accelerating = !m_accelerating;
    }

    if (m_accelerating) {
        m_simulatedThrottle = std::min(100.0, m_simulatedThrottle + rng->bounded(5.0));
        m_rpmTarget = 800.0 + (m_simulatedThrottle / 100.0) * 7200.0;
    } else {
        m_simulatedThrottle = std::max(0.0, m_simulatedThrottle - rng->bounded(3.0));
        m_rpmTarget = 800.0 + (m_simulatedThrottle / 100.0) * 7200.0;
    }

    // RPM follows target with some lag
    double rpmDiff = m_rpmTarget - m_simulatedRpm;
    m_simulatedRpm += rpmDiff * 0.1;

    // Add some noise
    m_simulatedRpm += rng->bounded(20.0) - 10.0;
    m_simulatedRpm = std::clamp(m_simulatedRpm, 0.0, 8000.0);

    // Calculate derived values
    double coolantTemp = 85.0 + rng->bounded(5.0) - 2.5;
    double oilTemp = 90.0 + rng->bounded(5.0) - 2.5;
    double iat = 35.0 + rng->bounded(3.0) - 1.5;
    double oilPressure = 200.0 + (m_simulatedRpm / 8000.0) * 300.0 + rng->bounded(20.0) - 10.0;
    double mapPressure = 30.0 + (m_simulatedThrottle / 100.0) * 170.0;
    double batteryVoltage = 13.8 + rng->bounded(0.4) - 0.2;
    double speed = std::max(0.0, (m_simulatedRpm - 800.0) / 7200.0 * 250.0);
    int gear = (speed < 10) ? 0 : std::min(6, static_cast<int>(speed / 40) + 1);

    // Update channels
    auto updateChannel = [this](const QString& name, double value, const QString& unit) {
        ChannelValue cv{value, unit, true};
        m_channels[name] = cv;
        emit channelUpdated(name, cv);
    };

    updateChannel("rpm", m_simulatedRpm, "RPM");
    updateChannel("throttlePosition", m_simulatedThrottle, "%");
    updateChannel("coolantTemperature", coolantTemp, "°C");
    updateChannel("oilTemperature", oilTemp, "°C");
    updateChannel("intakeAirTemperature", iat, "°C");
    updateChannel("oilPressure", oilPressure, "kPa");
    updateChannel("manifoldPressure", mapPressure, "kPa");
    updateChannel("batteryVoltage", batteryVoltage, "V");
    updateChannel("vehicleSpeed", speed, "km/h");
    updateChannel("gear", static_cast<double>(gear), "");
}

} // namespace devdash
