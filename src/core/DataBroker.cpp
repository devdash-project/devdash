#include "DataBroker.h"

#include <QDebug>

namespace devdash {

DataBroker::DataBroker(QObject* parent) : QObject(parent) {}

DataBroker::~DataBroker() {
    stop();
}

void DataBroker::setAdapter(std::unique_ptr<IProtocolAdapter> adapter) {
    if (m_adapter) {
        stop();
        disconnect(m_adapter.get(), nullptr, this, nullptr);
    }

    m_adapter = std::move(adapter);

    if (m_adapter) {
        connect(m_adapter.get(), &IProtocolAdapter::channelUpdated, this,
                &DataBroker::onChannelUpdated);
        connect(m_adapter.get(), &IProtocolAdapter::connectionStateChanged, this,
                &DataBroker::onConnectionStateChanged);
    }
}

bool DataBroker::start() {
    if (!m_adapter) {
        qWarning() << "DataBroker: No adapter set";
        return false;
    }
    return m_adapter->start();
}

void DataBroker::stop() {
    if (m_adapter && m_adapter->isRunning()) {
        m_adapter->stop();
    }
}

void DataBroker::onChannelUpdated(const QString& channelName, const ChannelValue& value) {
    if (!value.valid) {
        return;
    }

    // Map channel names to properties
    // TODO: Implement proper channel name mapping from profile
    if (channelName == "rpm" || channelName == "RPM") {
        if (m_rpm != value.value) {
            m_rpm = value.value;
            emit rpmChanged();
        }
    } else if (channelName == "throttlePosition" || channelName == "TPS") {
        if (m_throttlePosition != value.value) {
            m_throttlePosition = value.value;
            emit throttlePositionChanged();
        }
    } else if (channelName == "manifoldPressure" || channelName == "MAP") {
        if (m_manifoldPressure != value.value) {
            m_manifoldPressure = value.value;
            emit manifoldPressureChanged();
        }
    } else if (channelName == "coolantTemperature" || channelName == "ECT") {
        if (m_coolantTemperature != value.value) {
            m_coolantTemperature = value.value;
            emit coolantTemperatureChanged();
        }
    } else if (channelName == "oilTemperature") {
        if (m_oilTemperature != value.value) {
            m_oilTemperature = value.value;
            emit oilTemperatureChanged();
        }
    } else if (channelName == "intakeAirTemperature" || channelName == "IAT") {
        if (m_intakeAirTemperature != value.value) {
            m_intakeAirTemperature = value.value;
            emit intakeAirTemperatureChanged();
        }
    } else if (channelName == "oilPressure") {
        if (m_oilPressure != value.value) {
            m_oilPressure = value.value;
            emit oilPressureChanged();
        }
    } else if (channelName == "fuelPressure") {
        if (m_fuelPressure != value.value) {
            m_fuelPressure = value.value;
            emit fuelPressureChanged();
        }
    } else if (channelName == "fuelLevel") {
        if (m_fuelLevel != value.value) {
            m_fuelLevel = value.value;
            emit fuelLevelChanged();
        }
    } else if (channelName == "airFuelRatio" || channelName == "AFR") {
        if (m_airFuelRatio != value.value) {
            m_airFuelRatio = value.value;
            emit airFuelRatioChanged();
        }
    } else if (channelName == "batteryVoltage") {
        if (m_batteryVoltage != value.value) {
            m_batteryVoltage = value.value;
            emit batteryVoltageChanged();
        }
    } else if (channelName == "vehicleSpeed" || channelName == "speed") {
        if (m_vehicleSpeed != value.value) {
            m_vehicleSpeed = value.value;
            emit vehicleSpeedChanged();
        }
    } else if (channelName == "gear") {
        auto gearValue = static_cast<int>(value.value);
        if (m_gear != gearValue) {
            m_gear = gearValue;
            emit gearChanged();
        }
    }
}

void DataBroker::onConnectionStateChanged(bool connected) {
    if (m_isConnected != connected) {
        m_isConnected = connected;
        emit isConnectedChanged();
    }
}

// Property getters
double DataBroker::rpm() const {
    return m_rpm;
}

double DataBroker::throttlePosition() const {
    return m_throttlePosition;
}

double DataBroker::manifoldPressure() const {
    return m_manifoldPressure;
}

double DataBroker::coolantTemperature() const {
    return m_coolantTemperature;
}

double DataBroker::oilTemperature() const {
    return m_oilTemperature;
}

double DataBroker::intakeAirTemperature() const {
    return m_intakeAirTemperature;
}

double DataBroker::oilPressure() const {
    return m_oilPressure;
}

double DataBroker::fuelPressure() const {
    return m_fuelPressure;
}

double DataBroker::fuelLevel() const {
    return m_fuelLevel;
}

double DataBroker::airFuelRatio() const {
    return m_airFuelRatio;
}

double DataBroker::batteryVoltage() const {
    return m_batteryVoltage;
}

double DataBroker::vehicleSpeed() const {
    return m_vehicleSpeed;
}

int DataBroker::gear() const {
    return m_gear;
}

bool DataBroker::isConnected() const {
    return m_isConnected;
}

} // namespace devdash
