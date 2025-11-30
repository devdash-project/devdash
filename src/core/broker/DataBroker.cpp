#include "DataBroker.h"

#include "core/logging/LogCategories.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace devdash {

namespace {

/// Default maximum forward gear for manual transmissions
constexpr int DEFAULT_MAX_GEAR = 6;

/**
 * @brief Load gear mapping from profile JSON.
 *
 * Parses the "gearMapping" section of a vehicle profile to map numeric
 * gear values to display strings. This allows profiles to define
 * transmission-specific gear labels.
 *
 * @param profile The vehicle profile JSON object
 * @return Gear mapping hash, or default manual transmission mapping if not specified
 *
 * ## Profile Format
 *
 * Manual transmission example:
 * ```json
 * "gearMapping": {
 *     "-1": "R",   // Reverse
 *     "0": "N",    // Neutral
 *     "1": "1",    // First gear
 *     "2": "2",    // Second gear
 *     ...
 * }
 * ```
 *
 * Automatic transmission example:
 * ```json
 * "gearMapping": {
 *     "-2": "P",   // Park
 *     "-1": "R",   // Reverse
 *     "0": "N",    // Neutral
 *     "1": "D",    // Drive
 *     "2": "S",    // Sport
 *     "3": "M",    // Manual mode
 *     "4": "L"     // Low
 * }
 * ```
 *
 * @note Keys must be numeric strings, values must be non-empty strings
 * @note If not specified in profile, returns default manual transmission mapping
 */
QHash<int, QString> loadGearMappingFromProfile(const QJsonObject& profile) {
    QHash<int, QString> gearMapping;
    const auto gearMappingValue = profile.value("gearMapping");

    if (!gearMappingValue.isUndefined() && gearMappingValue.isObject()) {
        const auto mappingObj = gearMappingValue.toObject();
        for (auto it = mappingObj.begin(); it != mappingObj.end(); ++it) {
            bool conversionOk = false;
            const int gearNumber = it.key().toInt(&conversionOk);
            const QString gearLabel = it.value().toString();

            if (!conversionOk) {
                qWarning() << "DataBroker: Invalid gear mapping key (must be numeric):"
                           << it.key();
                continue;
            }

            if (gearLabel.isEmpty()) {
                qWarning() << "DataBroker: Invalid gear mapping value for" << gearNumber
                           << "- must be a non-empty string";
                continue;
            }

            gearMapping[gearNumber] = gearLabel;
            qDebug() << "DataBroker: Gear mapping:" << gearNumber << "->" << gearLabel;
        }
        qInfo() << "DataBroker: Loaded" << gearMapping.size() << "gear mappings";
    } else {
        // Default manual transmission mapping if not specified
        gearMapping[-1] = "R";
        gearMapping[0] = "N";
        for (int i = 1; i <= DEFAULT_MAX_GEAR; ++i) {
            gearMapping[i] = QString::number(i);
        }
        qInfo() << "DataBroker: Using default manual transmission gear mapping";
    }

    return gearMapping;
}

/**
 * @brief Map string property names to StandardChannel enum values.
 *
 * Used when parsing profile JSON to convert human-readable property
 * names to type-safe enum values.
 */
const QHash<QString, StandardChannel> PROPERTY_NAME_TO_CHANNEL = {
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

    // Set up 60Hz timer for queue processing (16.67ms = 60Hz)
    m_queueTimer.setInterval(16);
    m_queueTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_queueTimer, &QTimer::timeout, this, &DataBroker::processQueue);
}

DataBroker::~DataBroker() {
    stop();
}

void DataBroker::initializeChannelHandlers() {
    // Register handlers for each standard channel using the helper template
    // This reduces cognitive complexity by extracting the common update-and-emit pattern
    m_channelHandlers[StandardChannel::Rpm] = makeChannelHandler(m_rpm, &DataBroker::rpmChanged);
    m_channelHandlers[StandardChannel::ThrottlePosition] =
        makeChannelHandler(m_throttlePosition, &DataBroker::throttlePositionChanged);
    m_channelHandlers[StandardChannel::ManifoldPressure] =
        makeChannelHandler(m_manifoldPressure, &DataBroker::manifoldPressureChanged);
    m_channelHandlers[StandardChannel::CoolantTemperature] =
        makeChannelHandler(m_coolantTemperature, &DataBroker::coolantTemperatureChanged);
    m_channelHandlers[StandardChannel::OilTemperature] =
        makeChannelHandler(m_oilTemperature, &DataBroker::oilTemperatureChanged);
    m_channelHandlers[StandardChannel::IntakeAirTemperature] =
        makeChannelHandler(m_intakeAirTemperature, &DataBroker::intakeAirTemperatureChanged);
    m_channelHandlers[StandardChannel::OilPressure] =
        makeChannelHandler(m_oilPressure, &DataBroker::oilPressureChanged);
    m_channelHandlers[StandardChannel::FuelPressure] =
        makeChannelHandler(m_fuelPressure, &DataBroker::fuelPressureChanged);
    m_channelHandlers[StandardChannel::FuelLevel] =
        makeChannelHandler(m_fuelLevel, &DataBroker::fuelLevelChanged);
    m_channelHandlers[StandardChannel::AirFuelRatio] =
        makeChannelHandler(m_airFuelRatio, &DataBroker::airFuelRatioChanged);
    m_channelHandlers[StandardChannel::BatteryVoltage] =
        makeChannelHandler(m_batteryVoltage, &DataBroker::batteryVoltageChanged);
    m_channelHandlers[StandardChannel::VehicleSpeed] =
        makeChannelHandler(m_vehicleSpeed, &DataBroker::vehicleSpeedChanged);

    // Gear needs custom handling to convert double to QString using profile mapping
    m_channelHandlers[StandardChannel::Gear] = [this](double value) {
        auto gearInt = static_cast<int>(value);

        // Look up gear label from profile mapping
        QString gearStr = m_gearMapping.value(gearInt, "N");  // Default to "N" if not found

        if (m_gear != gearStr) {
            m_gear = gearStr;
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

    const auto mappingsValue = profile.value("channelMappings");
    if (mappingsValue.isUndefined() || mappingsValue.isNull()) {
        qWarning() << "DataBroker: Profile has no channelMappings - using empty mapping";
        return true; // Not an error, just no mappings
    }

    if (!mappingsValue.isObject()) {
        qCritical() << "DataBroker: channelMappings must be an object";
        return false;
    }

    const auto mappings = mappingsValue.toObject();
    for (auto it = mappings.begin(); it != mappings.end(); ++it) {
        const QString& protocolChannelName = it.key();
        const QString propertyName = it.value().toString();

        if (propertyName.isEmpty()) {
            qWarning() << "DataBroker: Skipping invalid mapping for" << protocolChannelName
                       << "- value must be a string";
            continue;
        }

        auto channelIt = PROPERTY_NAME_TO_CHANNEL.find(propertyName);
        if (channelIt == PROPERTY_NAME_TO_CHANNEL.end()) {
            qWarning() << "DataBroker: Unknown property name:" << propertyName << "for channel"
                       << protocolChannelName;
            continue;
        }

        m_channelMappings[protocolChannelName] = channelIt.value();
        qDebug() << "DataBroker: Mapped" << protocolChannelName << "->" << propertyName;
    }

    qInfo() << "DataBroker: Loaded" << m_channelMappings.size() << "channel mappings";

    // Load gear mapping from profile
    m_gearMapping = loadGearMappingFromProfile(profile);

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

    // Start the 60Hz queue processing timer
    m_queueTimer.start();

    return m_adapter->start();
}

void DataBroker::stop() {
    // Stop queue processing
    m_queueTimer.stop();

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

void DataBroker::processQueue() {
    // Dequeue all pending updates in bulk (more efficient than one-by-one)
    std::vector<ChannelUpdate> updates;
    const std::size_t dequeued = m_updateQueue.dequeueBulk(updates);

    if (dequeued == 0) {
        return;  // No updates to process
    }

    qCDebug(logBroker) << "Processing" << dequeued << "updates from queue";

    // Process all dequeued updates
    for (const auto& update : updates) {
        if (!update.value.valid) {
            qCDebug(logBroker) << "Skipping invalid value for" << update.channelName;
            continue;  // Skip invalid values
        }

        // Map protocol channel name to standard channel
        auto standardChannel = mapToStandardChannel(update.channelName);
        if (!standardChannel.has_value()) {
            // LOUD FAILURE: Unmapped channel indicates configuration error
            // This is critical - it means protocol is sending data we can't use
            // (Would have caught the toCamelCase bug that turned "RPM" → "rPM")

            // Only warn once per unmapped channel to avoid log spam
            if (!m_warnedUnmappedChannels.contains(update.channelName)) {
                qCCritical(logBroker) << "UNMAPPED CHANNEL:" << update.channelName
                                      << "- Check profile channelMappings!";
                qCCritical(logBroker) << "Available mappings:" << m_channelMappings.keys();
                qCCritical(logBroker)
                    << "This indicates a mismatch between protocol and profile.";
                qCCritical(logBroker) << "Expected one of:" << m_channelMappings.keys();

                m_warnedUnmappedChannels.insert(update.channelName);
            }
            continue;  // Unmapped channel
        }

        qCDebug(logBroker) << "Mapped" << update.channelName
                           << "to standard channel, calling handler";

        // Find and invoke the handler for this channel
        auto handlerIt = m_channelHandlers.find(standardChannel.value());
        if (handlerIt != m_channelHandlers.end()) {
            handlerIt.value()(update.value.value);
            qCDebug(logBroker) << "Handler executed for" << update.channelName;
        } else {
            qCWarning(logBroker) << "No handler found for standard channel";
        }
    }
}

void DataBroker::onChannelUpdated(const QString& channelName, const ChannelValue& value) {
    qDebug() << "DataBroker: onChannelUpdated:" << channelName << "=" << value.value
             << value.unit << "(valid:" << value.valid << ")";
    // Enqueue update for batch processing by the 60Hz timer
    if (!m_updateQueue.enqueue(channelName, value)) {
        qWarning() << "DataBroker: Failed to enqueue update for channel:" << channelName;
    } else {
        qDebug() << "DataBroker: Enqueued" << channelName;
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

QString DataBroker::gear() const {
    return m_gear;
}

bool DataBroker::isConnected() const {
    return m_isConnected;
}

} // namespace devdash