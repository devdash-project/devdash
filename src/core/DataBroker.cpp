#include "DataBroker.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace devdash {

namespace {

/**
 * @brief Map string property names to StandardChannel enum values.
 *
 * Used when parsing profile JSON to convert human-readable property
 * names to type-safe enum values.
 */
const QHash<QString, StandardChannel> kPropertyNameToChannel = {
    {"rpm", StandardChannel::Rpm},
    {"throttlePosition", StandardChannel::ThrottlePosition},
    {"manifoldPressure", StandardChannel::ManifoldPressure},
    {"coolantTemperature", StandardChannel::CoolantTemperature},
    {"oilTemperature", StandardChannel::OilTemperature},
    {"intakeAirTemperature", StandardChannel::IntakeAirTemperature},
    {"oilPressure", StandardChannel::OilPressure},
    {"fuelPressure", StandardChannel::FuelPressure},
    {"fuelLevel", StandardChannel::FuelLevel},
    {"airFuelRatio", StandardChannel::AirFuelRatio},
    {"batteryVoltage", StandardChannel::BatteryVoltage},
    {"vehicleSpeed", StandardChannel::VehicleSpeed},
    {"gear", StandardChannel::Gear},
};

} // anonymous namespace

DataBroker::DataBroker(QObject* parent) : QObject(parent) {
    initializeChannelHandlers();
}

DataBroker::~DataBroker() {
    stop();
}

void DataBroker::initializeChannelHandlers() {
    // Register handlers for each standard channel
    // Each handler updates the property and emits the change signal

    m_channelHandlers[StandardChannel::Rpm] = [this](double value) {
        if (m_rpm != value) {
            m_rpm = value;
            emit rpmChanged();
        }
    };

    m_channelHandlers[StandardChannel::ThrottlePosition] = [this](double value) {
        if (m_throttlePosition != value) {
            m_throttlePosition = value;
            emit throttlePositionChanged();
        }
    };

    m_channelHandlers[StandardChannel::ManifoldPressure] = [this](double value) {
        if (m_manifoldPressure != value) {
            m_manifoldPressure = value;
            emit manifoldPressureChanged();
        }
    };

    m_channelHandlers[StandardChannel::CoolantTemperature] = [this](double value) {
        if (m_coolantTemperature != value) {
            m_coolantTemperature = value;
            emit coolantTemperatureChanged();
        }
    };

    m_channelHandlers[StandardChannel::OilTemperature] = [this](double value) {
        if (m_oilTemperature != value) {
            m_oilTemperature = value;
            emit oilTemperatureChanged();
        }
    };

    m_channelHandlers[StandardChannel::IntakeAirTemperature] = [this](double value) {
        if (m_intakeAirTemperature != value) {
            m_intakeAirTemperature = value;
            emit intakeAirTemperatureChanged();
        }
    };

    m_channelHandlers[StandardChannel::OilPressure] = [this](double value) {
        if (m_oilPressure != value) {
            m_oilPressure = value;
            emit oilPressureChanged();
        }
    };

    m_channelHandlers[StandardChannel::FuelPressure] = [this](double value) {
        if (m_fuelPressure != value) {
            m_fuelPressure = value;
            emit fuelPressureChanged();
        }
    };

    m_channelHandlers[StandardChannel::FuelLevel] = [this](double value) {
        if (m_fuelLevel != value) {
            m_fuelLevel = value;
            emit fuelLevelChanged();
        }
    };

    m_channelHandlers[StandardChannel::AirFuelRatio] = [this](double value) {
        if (m_airFuelRatio != value) {
            m_airFuelRatio = value;
            emit airFuelRatioChanged();
        }
    };

    m_channelHandlers[StandardChannel::BatteryVoltage] = [this](double value) {
        if (m_batteryVoltage != value) {
            m_batteryVoltage = value;
            emit batteryVoltageChanged();
        }
    };

    m_channelHandlers[StandardChannel::VehicleSpeed] = [this](double value) {
        if (m_vehicleSpeed != value) {
            m_vehicleSpeed = value;
            emit vehicleSpeedChanged();
        }
    };

    m_channelHandlers[StandardChannel::Gear] = [this](double value) {
        auto gearValue = static_cast<int>(value);
        if (m_gear != gearValue) {
            m_gear = gearValue;
            emit gearChanged();
        }
    };
}

bool DataBroker::loadProfile(const QString& profilePath) {
    QFile file(profilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "DataBroker: Failed to open profile:" << profilePath << "-"
                    << file.errorString();
        return false;
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCritical() << "DataBroker: Failed to parse profile:" << profilePath << "-"
                    << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qCritical() << "DataBroker: Profile must be a JSON object:" << profilePath;
        return false;
    }

    return loadProfileFromJson(doc.object());
}

bool DataBroker::loadProfileFromJson(const QJsonObject& profile) {
    m_channelMappings.clear();

    const auto MAPPINGS_VALUE = profile.value("channelMappings");
    if (MAPPINGS_VALUE.isUndefined() || MAPPINGS_VALUE.isNull()) {
        qWarning() << "DataBroker: Profile has no channelMappings - using empty mapping";
        return true; // Not an error, just no mappings
    }

    if (!MAPPINGS_VALUE.isObject()) {
        qCritical() << "DataBroker: channelMappings must be an object";
        return false;
    }

    const auto MAPPINGS = MAPPINGS_VALUE.toObject();
    for (auto it = MAPPINGS.begin(); it != MAPPINGS.end(); ++it) {
        const QString& protocolChannelName = it.key();
        const QString PROPERTY_NAME = it.value().toString();

        if (PROPERTY_NAME.isEmpty()) {
            qWarning() << "DataBroker: Skipping invalid mapping for" << protocolChannelName
                       << "- value must be a string";
            continue;
        }

        auto channelIt = kPropertyNameToChannel.find(PROPERTY_NAME);
        if (channelIt == kPropertyNameToChannel.end()) {
            qWarning() << "DataBroker: Unknown property name:" << PROPERTY_NAME << "for channel"
                       << protocolChannelName;
            continue;
        }

        m_channelMappings[protocolChannelName] = channelIt.value();
        qDebug() << "DataBroker: Mapped" << protocolChannelName << "->" << PROPERTY_NAME;
    }

    qInfo() << "DataBroker: Loaded" << m_channelMappings.size() << "channel mappings";
    return true;
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

    if (m_channelMappings.isEmpty()) {
        qWarning() << "DataBroker: No channel mappings loaded - data will be ignored";
    }

    return m_adapter->start();
}

void DataBroker::stop() {
    if (m_adapter && m_adapter->isRunning()) {
        m_adapter->stop();
    }
}

std::optional<StandardChannel>
DataBroker::mapToStandardChannel(const QString& protocolChannelName) const {
    auto it = m_channelMappings.find(protocolChannelName);
    if (it != m_channelMappings.end()) {
        return it.value();
    }
    return std::nullopt;
}

void DataBroker::onChannelUpdated(const QString& channelName, const ChannelValue& value) {
    if (!value.valid) {
        return;
    }

    // Map protocol channel name to standard channel
    auto standardChannel = mapToStandardChannel(channelName);
    if (!standardChannel.has_value()) {
        // Unmapped channel - this is normal for channels we don't care about
        return;
    }

    // Find and invoke the handler for this channel
    auto handlerIt = m_channelHandlers.find(standardChannel.value());
    if (handlerIt != m_channelHandlers.end()) {
        handlerIt.value()(value.value);
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