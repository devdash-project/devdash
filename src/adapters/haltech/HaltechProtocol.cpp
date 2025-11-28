#include "HaltechProtocol.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstring>

namespace devdash {

namespace {
// Haltech CAN frame IDs (standard base addresses) - used for hardcoded fallback
constexpr uint32_t FRAME_ID_RPM_TPS = 0x360;
constexpr uint32_t FRAME_ID_FUEL_IGN = 0x361;
constexpr uint32_t FRAME_ID_TEMPS = 0x362;
constexpr uint32_t FRAME_ID_PRESSURES = 0x363;
constexpr uint32_t FRAME_ID_SPEED_GEAR = 0x370;

// Validation limits
constexpr double MAX_VALID_RPM = 20000.0;
constexpr double MIN_VALID_TEMP_C = -40.0;
constexpr double MAX_VALID_TEMP_C = 200.0;

// Physical constants
constexpr double KELVIN_OFFSET = 273.15;
constexpr double ATMOSPHERIC_PRESSURE_KPA = 101.3;
} // namespace

ConversionType HaltechProtocol::parseConversion(const QString& formula) {
    // Pattern match common conversion formulas
    QString normalized = formula.simplified().toLower();

    if (normalized == "x" || normalized.isEmpty()) {
        return ConversionType::Identity;
    }
    if (normalized == "x / 10") {
        return ConversionType::DivideBy10;
    }
    if (normalized == "x / 1000") {
        return ConversionType::DivideBy1000;
    }
    if (normalized.contains("101.3") || normalized.contains("101.325")) {
        return ConversionType::GaugePressure;
    }
    // Temperature conversion is determined by units, not formula
    // The JSON uses "x / 10" for temps but units are "K"

    // Default to DivideBy10 for unknown formulas with division
    if (normalized.contains("/ 10")) {
        return ConversionType::DivideBy10;
    }

    return ConversionType::Identity;
}

double HaltechProtocol::applyConversion(ConversionType type, double rawValue) {
    switch (type) {
    case ConversionType::Identity:
        return rawValue;
    case ConversionType::DivideBy10:
        return rawValue / 10.0;
    case ConversionType::DivideBy1000:
        return rawValue / 1000.0;
    case ConversionType::GaugePressure:
        return (rawValue / 10.0) - ATMOSPHERIC_PRESSURE_KPA;
    case ConversionType::KelvinToCelsius:
        return (rawValue / 10.0) - KELVIN_OFFSET;
    }
    return rawValue;
}

bool HaltechProtocol::loadDefinition(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open protocol definition:" << path;
        return false;
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse protocol definition:" << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    QJsonObject frames = root["frames"].toObject();

    m_frameDefinitions.clear();

    for (auto it = frames.begin(); it != frames.end(); ++it) {
        QString frameIdStr = it.key();
        QJsonObject frameObj = it.value().toObject();

        // Parse frame ID from hex string (e.g., "0x360")
        bool parseOk = false;
        uint32_t frameId = frameIdStr.toUInt(&parseOk, 16);
        if (!parseOk) {
            qWarning() << "Invalid frame ID:" << frameIdStr;
            continue;
        }

        FrameDefinition frameDef;
        frameDef.frameId = frameId;
        frameDef.name = frameObj["name"].toString();
        frameDef.rateHz = frameObj["rate_hz"].toInt();

        QJsonArray channelsArray = frameObj["channels"].toArray();
        for (const auto& channelVal : channelsArray) {
            QJsonObject channelObj = channelVal.toObject();

            ChannelDefinition channelDef;
            channelDef.name = channelObj["name"].toString();
            channelDef.isSigned = channelObj["signed"].toBool(false);
            channelDef.units = channelObj["units"].toString();

            // Parse byte indices
            QJsonArray bytesArray = channelObj["bytes"].toArray();
            for (const auto& byteVal : bytesArray) {
                channelDef.byteIndices.push_back(byteVal.toInt());
            }

            // Parse conversion - check for temperature units
            QString conversionStr = channelObj["conversion"].toString();
            if (channelDef.units == "K") {
                // Kelvin units means we need Kelvin-to-Celsius conversion
                channelDef.conversion = ConversionType::KelvinToCelsius;
            } else {
                channelDef.conversion = parseConversion(conversionStr);
            }

            frameDef.channels.push_back(std::move(channelDef));
        }

        m_frameDefinitions[frameId] = std::move(frameDef);
    }

    qDebug() << "Loaded" << m_frameDefinitions.size() << "frame definitions from" << path;
    return !m_frameDefinitions.isEmpty();
}

std::vector<std::pair<QString, ChannelValue>>
HaltechProtocol::decode(const QCanBusFrame& frame) const {
    if (!frame.isValid() || frame.payload().size() < 2) {
        return {};
    }

    // Use JSON-defined decoding if available, otherwise fall back to hardcoded
    if (m_frameDefinitions.contains(frame.frameId())) {
        return decodeFromDefinition(frame);
    }
    return decodeHardcoded(frame);
}

std::vector<std::pair<QString, ChannelValue>>
HaltechProtocol::decodeFromDefinition(const QCanBusFrame& frame) const {
    std::vector<std::pair<QString, ChannelValue>> results;

    const auto& frameDef = m_frameDefinitions[frame.frameId()];
    const auto& payload = frame.payload();

    for (const auto& channelDef : frameDef.channels) {
        // Check we have enough bytes
        if (channelDef.byteIndices.empty()) {
            continue;
        }

        int maxByte =
            *std::max_element(channelDef.byteIndices.begin(), channelDef.byteIndices.end());
        if (maxByte >= payload.size()) {
            continue;
        }

        // Extract raw value based on byte indices
        double rawValue = 0.0;
        if (channelDef.byteIndices.size() == 2) {
            // 16-bit value (big-endian)
            int offset = channelDef.byteIndices[0];
            if (channelDef.isSigned) {
                rawValue = static_cast<double>(decodeInt16(payload, offset));
            } else {
                rawValue = static_cast<double>(decodeUint16(payload, offset));
            }
        } else if (channelDef.byteIndices.size() == 1) {
            // 8-bit value
            int offset = channelDef.byteIndices[0];
            if (channelDef.isSigned) {
                rawValue = static_cast<double>(static_cast<int8_t>(payload[offset]));
            } else {
                rawValue = static_cast<double>(static_cast<uint8_t>(payload[offset]));
            }
        }

        // Apply conversion
        double convertedValue = applyConversion(channelDef.conversion, rawValue);

        // Convert channel name to camelCase for consistency
        QString channelName = channelDef.name;
        channelName = channelName.replace(" ", "");
        if (!channelName.isEmpty()) {
            channelName[0] = channelName[0].toLower();
        }

        // Determine output unit (convert K to °C)
        QString outputUnit = channelDef.units;
        if (channelDef.conversion == ConversionType::KelvinToCelsius) {
            outputUnit = QString::fromUtf8("°C");
        }

        results.emplace_back(channelName, ChannelValue{convertedValue, outputUnit, true});
    }

    return results;
}

std::vector<std::pair<QString, ChannelValue>>
HaltechProtocol::decodeHardcoded(const QCanBusFrame& frame) const {
    std::vector<std::pair<QString, ChannelValue>> results;

    const auto frameId = frame.frameId();
    const auto& payload = frame.payload();

    switch (frameId) {
    case FRAME_ID_RPM_TPS: {
        // Byte 0-1: RPM
        // Byte 2-3: TPS (throttle position %)
        auto rpm = decodeRpm(payload);
        if (rpm >= 0 && rpm <= MAX_VALID_RPM) {
            results.emplace_back("rpm", ChannelValue{rpm, "RPM", true});
        }

        if (payload.size() >= 4) {
            auto tps = decodeUint16(payload, 2) / 10.0; // 0.1% resolution
            if (tps >= 0 && tps <= 100) {
                results.emplace_back("throttlePosition", ChannelValue{tps, "%", true});
            }
        }
        break;
    }

    case FRAME_ID_TEMPS: {
        // Byte 0-1: Coolant temp (Kelvin * 10)
        // Byte 2-3: Intake air temp (Kelvin * 10)
        if (payload.size() >= 2) {
            auto coolant = decodeTemperature(payload, 0);
            if (coolant >= MIN_VALID_TEMP_C && coolant <= MAX_VALID_TEMP_C) {
                results.emplace_back("coolantTemperature", ChannelValue{coolant, "°C", true});
            }
        }
        if (payload.size() >= 4) {
            auto iat = decodeTemperature(payload, 2);
            if (iat >= MIN_VALID_TEMP_C && iat <= MAX_VALID_TEMP_C) {
                results.emplace_back("intakeAirTemperature", ChannelValue{iat, "°C", true});
            }
        }
        break;
    }

    case FRAME_ID_PRESSURES: {
        // Byte 0-1: Manifold pressure (kPa * 10)
        // Byte 2-3: Oil pressure (kPa * 10)
        if (payload.size() >= 2) {
            auto map = decodePressure(payload, 0);
            if (map >= 0 && map <= 400) { // 0-400 kPa reasonable range
                results.emplace_back("manifoldPressure", ChannelValue{map, "kPa", true});
            }
        }
        if (payload.size() >= 4) {
            auto oilPressure = decodePressure(payload, 2);
            if (oilPressure >= 0 && oilPressure <= 1000) {
                results.emplace_back("oilPressure", ChannelValue{oilPressure, "kPa", true});
            }
        }
        break;
    }

    case FRAME_ID_SPEED_GEAR: {
        // Byte 0-1: Vehicle speed (km/h * 10)
        // Byte 2: Gear (0=N, 1-6=gear)
        if (payload.size() >= 2) {
            auto speed = decodeUint16(payload, 0) / 10.0;
            if (speed >= 0 && speed <= 400) {
                results.emplace_back("vehicleSpeed", ChannelValue{speed, "km/h", true});
            }
        }
        if (payload.size() >= 3) {
            auto gear = static_cast<int>(payload[2]);
            if (gear >= 0 && gear <= 8) {
                results.emplace_back("gear", ChannelValue{static_cast<double>(gear), "", true});
            }
        }
        break;
    }

    default:
        // Unknown frame ID - ignore
        break;
    }

    return results;
}

uint16_t HaltechProtocol::decodeUint16(const QByteArray& payload, int offset) {
    if (offset + 2 > payload.size()) {
        return 0;
    }
    // Big-endian (Haltech standard)
    return static_cast<uint16_t>((static_cast<uint8_t>(payload[offset]) << 8) |
                                 static_cast<uint8_t>(payload[offset + 1]));
}

int16_t HaltechProtocol::decodeInt16(const QByteArray& payload, int offset) {
    return static_cast<int16_t>(decodeUint16(payload, offset));
}

double HaltechProtocol::decodeRpm(const QByteArray& payload) {
    // RPM is typically at offset 0, 1 RPM per bit
    return static_cast<double>(decodeUint16(payload, 0));
}

double HaltechProtocol::decodeTemperature(const QByteArray& payload, int offset) {
    // Temperature in Kelvin * 10, convert to Celsius
    auto kelvinTimes10 = decodeUint16(payload, offset);
    return (kelvinTimes10 / 10.0) - 273.15;
}

double HaltechProtocol::decodePressure(const QByteArray& payload, int offset) {
    // Pressure in kPa * 10
    return decodeUint16(payload, offset) / 10.0;
}

} // namespace devdash
