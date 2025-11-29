/**
 * @file SimulatorAdapter.cpp
 * @brief Implementation of simulator adapter.
 */

#include "SimulatorAdapter.h"

#include <QRandomGenerator>

#include <cmath>

namespace devdash {

namespace {

//=============================================================================
// Configuration
//=============================================================================

/// Config key for update interval
constexpr const char* CONFIG_KEY_UPDATE_INTERVAL = "updateIntervalMs";

/// Default update interval (50ms = 20 Hz)
constexpr int DEFAULT_UPDATE_INTERVAL_MS = 50;

//=============================================================================
// Engine Simulation Parameters
//=============================================================================

/// Idle RPM for simulated engine
constexpr double IDLE_RPM = 800.0;

/// Maximum RPM for simulated engine
constexpr double MAX_RPM = 8000.0;

/// RPM range above idle (MAX_RPM - IDLE_RPM)
constexpr double RPM_RANGE = 7200.0;

/// RPM response factor (0.0-1.0, higher = faster response)
constexpr double RPM_LAG_FACTOR = 0.1;

/// RPM noise amplitude (+/- this value)
constexpr double RPM_NOISE_AMPLITUDE = 10.0;

//=============================================================================
// Throttle Simulation Parameters
//=============================================================================

/// Maximum throttle percentage
constexpr double MAX_THROTTLE_PERCENT = 100.0;

/// Minimum throttle percentage
constexpr double MIN_THROTTLE_PERCENT = 0.0;

/// Maximum throttle increase per tick
constexpr double THROTTLE_INCREASE_RATE = 5.0;

/// Maximum throttle decrease per tick
constexpr double THROTTLE_DECREASE_RATE = 3.0;

/// Probability of changing acceleration state (percent per tick)
constexpr int ACCELERATION_CHANGE_PROBABILITY = 5;

/// Probability check range
constexpr int PROBABILITY_RANGE = 100;

//=============================================================================
// Temperature Simulation Parameters (Celsius)
//=============================================================================

/// Base coolant temperature
constexpr double COOLANT_TEMP_BASE = 85.0;

/// Coolant temperature variation (+/-)
constexpr double COOLANT_TEMP_VARIANCE = 2.5;

/// Base oil temperature
constexpr double OIL_TEMP_BASE = 90.0;

/// Oil temperature variation (+/-)
constexpr double OIL_TEMP_VARIANCE = 2.5;

/// Base intake air temperature
constexpr double IAT_BASE = 35.0;

/// IAT variation (+/-)
constexpr double IAT_VARIANCE = 1.5;

//=============================================================================
// Pressure Simulation Parameters (kPa)
//=============================================================================

/// Base oil pressure at idle
constexpr double OIL_PRESSURE_BASE = 200.0;

/// Additional oil pressure at max RPM
constexpr double OIL_PRESSURE_RPM_FACTOR = 300.0;

/// Oil pressure noise amplitude (+/-)
constexpr double OIL_PRESSURE_NOISE = 10.0;

/// Base manifold pressure (vacuum at idle)
constexpr double MAP_BASE = 30.0;

/// Additional MAP at full throttle
constexpr double MAP_THROTTLE_FACTOR = 170.0;

//=============================================================================
// Electrical Simulation Parameters
//=============================================================================

/// Base battery voltage
constexpr double BATTERY_VOLTAGE_BASE = 13.8;

/// Battery voltage variation (+/-)
constexpr double BATTERY_VOLTAGE_VARIANCE = 0.2;

//=============================================================================
// Speed/Gear Simulation Parameters
//=============================================================================

/// Maximum vehicle speed (km/h)
constexpr double MAX_SPEED_KMH = 250.0;

/// Speed threshold below which gear is neutral
constexpr double NEUTRAL_SPEED_THRESHOLD = 10.0;

/// Speed per gear step (km/h)
constexpr double SPEED_PER_GEAR = 40.0;

/// Maximum gear number
constexpr int MAX_GEAR = 6;

//=============================================================================
// Noise Generation
//=============================================================================

/// Multiplier to convert +/- variance to full range (variance * 2 = range)
constexpr double VARIANCE_TO_RANGE = 2.0;

//=============================================================================
// Channel Names
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
// Unit Strings
//=============================================================================

constexpr const char* UNIT_RPM = "RPM";
constexpr const char* UNIT_PERCENT = "%";
constexpr const char* UNIT_KPA = "kPa";
constexpr const char* UNIT_VOLTS = "V";
constexpr const char* UNIT_KMH = "km/h";
constexpr const char* UNIT_NONE = "";

const QString UNIT_CELSIUS = QString::fromUtf8("°C");

} // anonymous namespace

//=============================================================================
// Construction / Destruction
//=============================================================================

SimulatorAdapter::SimulatorAdapter(const QJsonObject& config, QObject* parent)
    : IProtocolAdapter(parent),
      m_updateIntervalMs(config[CONFIG_KEY_UPDATE_INTERVAL].toInt(DEFAULT_UPDATE_INTERVAL_MS)) {

    connect(&m_timer, &QTimer::timeout, this, &SimulatorAdapter::generateData);
}

SimulatorAdapter::~SimulatorAdapter() {
    stop();
}

//=============================================================================
// IProtocolAdapter Interface
//=============================================================================

bool SimulatorAdapter::start() {
    if (m_running) {
        return true;
    }

    // Initialize with idle values
    m_simulatedRpm = IDLE_RPM;
    m_simulatedThrottle = MIN_THROTTLE_PERCENT;
    m_rpmTarget = IDLE_RPM;
    m_accelerating = false;

    m_timer.start(m_updateIntervalMs);
    m_running = true;
    emit connectionStateChanged(true);

    qInfo() << "SimulatorAdapter: Started with" << m_updateIntervalMs << "ms interval";
    return true;
}

void SimulatorAdapter::stop() {
    if (!m_running) {
        return;
    }

    m_timer.stop();
    m_running = false;
    emit connectionStateChanged(false);

    qInfo() << "SimulatorAdapter: Stopped";
}

bool SimulatorAdapter::isRunning() const {
    return m_running;
}

std::optional<ChannelValue> SimulatorAdapter::getChannel(const QString& channelName) const {
    auto it = m_channels.find(channelName);
    if (it != m_channels.end()) {
        return it.value();
    }
    return std::nullopt;
}

QStringList SimulatorAdapter::availableChannels() const {
    return m_channels.keys();
}

QString SimulatorAdapter::adapterName() const {
    return QStringLiteral("Simulator");
}

//=============================================================================
// Data Generation
//=============================================================================

void SimulatorAdapter::generateData() {
    updateThrottleSimulation();
    updateRpmSimulation();
    emitDerivedChannels();
}

void SimulatorAdapter::updateThrottleSimulation() {
    auto* rng = QRandomGenerator::global();

    // Randomly change acceleration state
    if (rng->bounded(PROBABILITY_RANGE) < ACCELERATION_CHANGE_PROBABILITY) {
        m_accelerating = !m_accelerating;
    }

    // Update throttle based on acceleration state
    if (m_accelerating) {
        double increase = rng->bounded(THROTTLE_INCREASE_RATE);
        m_simulatedThrottle = std::min(MAX_THROTTLE_PERCENT, m_simulatedThrottle + increase);
    } else {
        double decrease = rng->bounded(THROTTLE_DECREASE_RATE);
        m_simulatedThrottle = std::max(MIN_THROTTLE_PERCENT, m_simulatedThrottle - decrease);
    }

    // Calculate target RPM from throttle position
    double throttleRatio = m_simulatedThrottle / MAX_THROTTLE_PERCENT;
    m_rpmTarget = IDLE_RPM + (throttleRatio * RPM_RANGE);
}

void SimulatorAdapter::updateRpmSimulation() {
    auto* rng = QRandomGenerator::global();

    // RPM follows target with lag
    double rpmDiff = m_rpmTarget - m_simulatedRpm;
    m_simulatedRpm += rpmDiff * RPM_LAG_FACTOR;

    // Add noise (random value in range -AMPLITUDE to +AMPLITUDE)
    double noiseRange = RPM_NOISE_AMPLITUDE * VARIANCE_TO_RANGE;
    double noise = rng->bounded(noiseRange) - RPM_NOISE_AMPLITUDE;
    m_simulatedRpm += noise;

    // Clamp to valid range
    m_simulatedRpm = std::clamp(m_simulatedRpm, 0.0, MAX_RPM);
}

void SimulatorAdapter::emitDerivedChannels() {
    auto* rng = QRandomGenerator::global();

    // Calculate derived values
    double coolantTemp =
        COOLANT_TEMP_BASE +
        (rng->bounded(COOLANT_TEMP_VARIANCE * VARIANCE_TO_RANGE) - COOLANT_TEMP_VARIANCE);

    double oilTemp =
        OIL_TEMP_BASE + (rng->bounded(OIL_TEMP_VARIANCE * VARIANCE_TO_RANGE) - OIL_TEMP_VARIANCE);

    double iat = IAT_BASE + (rng->bounded(IAT_VARIANCE * VARIANCE_TO_RANGE) - IAT_VARIANCE);

    double rpmRatio = m_simulatedRpm / MAX_RPM;
    double oilPressure =
        OIL_PRESSURE_BASE + (rpmRatio * OIL_PRESSURE_RPM_FACTOR) +
        (rng->bounded(OIL_PRESSURE_NOISE * VARIANCE_TO_RANGE) - OIL_PRESSURE_NOISE);

    double throttleRatio = m_simulatedThrottle / MAX_THROTTLE_PERCENT;
    double mapPressure = MAP_BASE + (throttleRatio * MAP_THROTTLE_FACTOR);

    double batteryVoltage =
        BATTERY_VOLTAGE_BASE +
        (rng->bounded(BATTERY_VOLTAGE_VARIANCE * VARIANCE_TO_RANGE) - BATTERY_VOLTAGE_VARIANCE);

    double rpmAboveIdle = std::max(0.0, m_simulatedRpm - IDLE_RPM);
    double speed = (rpmAboveIdle / RPM_RANGE) * MAX_SPEED_KMH;

    int gear = 0;
    if (speed >= NEUTRAL_SPEED_THRESHOLD) {
        gear = std::min(MAX_GEAR, static_cast<int>(speed / SPEED_PER_GEAR) + 1);
    }

    // Emit all channels
    updateChannel(CHANNEL_RPM, m_simulatedRpm, UNIT_RPM);
    updateChannel(CHANNEL_THROTTLE, m_simulatedThrottle, UNIT_PERCENT);
    updateChannel(CHANNEL_COOLANT_TEMP, coolantTemp, UNIT_CELSIUS);
    updateChannel(CHANNEL_OIL_TEMP, oilTemp, UNIT_CELSIUS);
    updateChannel(CHANNEL_IAT, iat, UNIT_CELSIUS);
    updateChannel(CHANNEL_OIL_PRESSURE, oilPressure, UNIT_KPA);
    updateChannel(CHANNEL_MAP, mapPressure, UNIT_KPA);
    updateChannel(CHANNEL_BATTERY, batteryVoltage, UNIT_VOLTS);
    updateChannel(CHANNEL_SPEED, speed, UNIT_KMH);
    updateChannel(CHANNEL_GEAR, static_cast<double>(gear), UNIT_NONE);
}

void SimulatorAdapter::updateChannel(const QString& name, double value, const QString& unit) {
    ChannelValue channelValue{value, unit, true};
    m_channels[name] = channelValue;
    emit channelUpdated(name, channelValue);
}

} // namespace devdash