#include "HaltechProtocol.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace devdash {

namespace {

//=============================================================================
// Physical Constants
//=============================================================================

/// Offset to convert Kelvin to Celsius
constexpr double KELVIN_TO_CELSIUS_OFFSET = 273.15;

/// Standard atmospheric pressure in kPa (for gauge pressure conversion)
constexpr double ATMOSPHERIC_PRESSURE_KPA = 101.325;

//=============================================================================
// Scaling Factors
//=============================================================================

/// Common scale factor for 0.1 resolution values (temps, pressures)
constexpr double SCALE_DIVIDE_BY_10 = 10.0;

/// Scale factor for 0.001 resolution values
constexpr double SCALE_DIVIDE_BY_1000 = 1000.0;

//=============================================================================
// Payload Layout Constants
//=============================================================================

/// Minimum payload size to contain any valid data
constexpr int MIN_PAYLOAD_SIZE = 2;

/// Size of a 16-bit value in bytes
constexpr int UINT16_SIZE = 2;

/// Size of an 8-bit value in bytes
constexpr int UINT8_SIZE = 1;

//=============================================================================
// Output Unit Strings
//=============================================================================

/// Celsius temperature unit string
const QString UNIT_CELSIUS = QString::fromUtf8("°C");

} // anonymous namespace

//=============================================================================
// Construction
//=============================================================================

HaltechProtocol::HaltechProtocol() = default;

//=============================================================================
// Protocol Loading
//=============================================================================

bool HaltechProtocol::loadDefinition(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "HaltechProtocol: Failed to open protocol definition:" << path;
        return false;
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "HaltechProtocol: Failed to parse protocol definition:"
                   << parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    QJsonObject frames = root["frames"].toObject();

    m_frameDefinitions.clear();
    m_decoders.clear();

    for (auto it = frames.begin(); it != frames.end(); ++it) {
        QString frameIdStr = it.key();
        QJsonObject frameObj = it.value().toObject();

        // Parse frame ID from hex string (e.g., "0x360")
        bool parseOk = false;
        uint32_t frameId = frameIdStr.toUInt(&parseOk, 16);
        if (!parseOk) {
            qWarning() << "HaltechProtocol: Invalid frame ID:" << frameIdStr;
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

            // Determine conversion type based on units and formula
            QString conversionStr = channelObj["conversion"].toString();
            if (channelDef.units == "K") {
                // Kelvin units require Kelvin-to-Celsius conversion
                channelDef.conversion = ConversionType::KelvinToCelsius;
            } else {
                channelDef.conversion = parseConversion(conversionStr);
            }

            frameDef.channels.push_back(std::move(channelDef));
        }

        m_frameDefinitions[frameId] = std::move(frameDef);
    }

    // Build decoder lookup table from loaded definitions
    buildDecoderTable();

    qDebug() << "HaltechProtocol: Loaded" << m_frameDefinitions.size()
             << "frame definitions from" << path;
    return !m_frameDefinitions.isEmpty();
}

void HaltechProtocol::buildDecoderTable() {
    m_decoders.clear();

    for (auto it = m_frameDefinitions.begin(); it != m_frameDefinitions.end(); ++it) {
        m_decoders[it.key()] = createFrameDecoder(it.value());
    }
}

HaltechProtocol::FrameDecoder
HaltechProtocol::createFrameDecoder(const FrameDefinition& frameDef) const {
    // Capture frame definition by value to ensure it outlives the lambda
    return [this, frameDef](const QByteArray& payload)
               -> std::vector<std::pair<QString, ChannelValue>> {
        std::vector<std::pair<QString, ChannelValue>> results;

        for (const auto& channelDef : frameDef.channels) {
            auto decoded = decodeChannel(channelDef, payload);
            if (decoded.has_value()) {
                QString channelName = toCamelCase(channelDef.name);
                results.emplace_back(std::move(channelName), std::move(decoded.value()));
            }
        }

        return results;
    };
}

//=============================================================================
// Frame Decoding
//=============================================================================

std::vector<std::pair<QString, ChannelValue>>
HaltechProtocol::decode(const QCanBusFrame& frame) const {
    if (!frame.isValid() || frame.payload().size() < MIN_PAYLOAD_SIZE) {
        return {};
    }

    // Lookup decoder for this frame ID
    auto it = m_decoders.find(frame.frameId());
    if (it != m_decoders.end()) {
        return it.value()(frame.payload());
    }

    // Unknown frame ID - return empty (not an error)
    return {};
}

std::optional<ChannelValue> HaltechProtocol::decodeChannel(
    const ChannelDefinition& channelDef,
    const QByteArray& payload) const {

    if (channelDef.byteIndices.empty()) {
        return std::nullopt;
    }

    // Verify payload contains all required bytes
    int maxByteIndex =
        *std::max_element(channelDef.byteIndices.begin(), channelDef.byteIndices.end());
    if (maxByteIndex >= payload.size()) {
        return std::nullopt;
    }

    // Extract raw value based on byte count
    double rawValue = 0.0;
    const auto byteCount = channelDef.byteIndices.size();

    if (byteCount == UINT16_SIZE) {
        // 16-bit value (big-endian)
        int offset = channelDef.byteIndices[0];
        if (channelDef.isSigned) {
            rawValue = static_cast<double>(decodeInt16(payload, offset));
        } else {
            rawValue = static_cast<double>(decodeUint16(payload, offset));
        }
    } else if (byteCount == UINT8_SIZE) {
        // 8-bit value
        int offset = channelDef.byteIndices[0];
        if (channelDef.isSigned) {
            rawValue = static_cast<double>(static_cast<int8_t>(payload[offset]));
        } else {
            rawValue = static_cast<double>(static_cast<uint8_t>(payload[offset]));
        }
    } else {
        // Unsupported byte count
        return std::nullopt;
    }

    // Apply conversion formula
    double convertedValue = applyConversion(channelDef.conversion, rawValue);

    // Determine output unit (convert K to °C for display)
    QString outputUnit = channelDef.units;
    if (channelDef.conversion == ConversionType::KelvinToCelsius) {
        outputUnit = UNIT_CELSIUS;
    }

    return ChannelValue{convertedValue, outputUnit, true};
}

//=============================================================================
// Conversion Utilities
//=============================================================================

ConversionType HaltechProtocol::parseConversion(const QString& formula) {
    QString normalized = formula.simplified().toLower();

    if (normalized.isEmpty() || normalized == "x") {
        return ConversionType::Identity;
    }

    if (normalized == "x / 10") {
        return ConversionType::DivideBy10;
    }

    if (normalized == "x / 1000") {
        return ConversionType::DivideBy1000;
    }

    // Check for gauge pressure pattern (subtracts atmospheric)
    if (normalized.contains("101.3") || normalized.contains("101.325")) {
        return ConversionType::GaugePressure;
    }

    // Default to DivideBy10 for unknown formulas containing division
    if (normalized.contains("/ 10")) {
        return ConversionType::DivideBy10;
    }

    return ConversionType::Identity;
}

double HaltechProtocol::applyConversion(ConversionType type, double rawValue) {
    // Using a lookup table instead of switch for consistency with our pattern
    // Note: For simple enums with direct value mapping, a static table is efficient
    
    struct ConversionOp {
        std::function<double(double)> apply;
    };

    // Static table initialized once - maps ConversionType to operation
    static const std::array<ConversionOp, 5> operations = {{
        // Identity
        {[](double v) { return v; }},
        // DivideBy10
        {[](double v) { return v / SCALE_DIVIDE_BY_10; }},
        // DivideBy1000
        {[](double v) { return v / SCALE_DIVIDE_BY_1000; }},
        // GaugePressure
        {[](double v) { return (v / SCALE_DIVIDE_BY_10) - ATMOSPHERIC_PRESSURE_KPA; }},
        // KelvinToCelsius
        {[](double v) { return (v / SCALE_DIVIDE_BY_10) - KELVIN_TO_CELSIUS_OFFSET; }},
    }};

    auto index = static_cast<size_t>(type);
    if (index < operations.size()) {
        return operations[index].apply(rawValue);
    }

    return rawValue;
}

//=============================================================================
// Low-Level Decoding
//=============================================================================

uint16_t HaltechProtocol::decodeUint16(const QByteArray& payload, int offset) {
    if (offset + UINT16_SIZE > payload.size()) {
        return 0;
    }
    // Big-endian (Haltech standard)
    return static_cast<uint16_t>(
        (static_cast<uint8_t>(payload[offset]) << 8) |
        static_cast<uint8_t>(payload[offset + 1]));
}

int16_t HaltechProtocol::decodeInt16(const QByteArray& payload, int offset) {
    return static_cast<int16_t>(decodeUint16(payload, offset));
}

double HaltechProtocol::decodeRpm(const QByteArray& payload) {
    // RPM is uint16 at offset 0, 1 RPM per bit
    return static_cast<double>(decodeUint16(payload, 0));
}

double HaltechProtocol::decodeTemperature(const QByteArray& payload, int offset) {
    // Temperature stored as Kelvin * 10, convert to Celsius
    auto kelvinTimes10 = decodeUint16(payload, offset);
    return (kelvinTimes10 / SCALE_DIVIDE_BY_10) - KELVIN_TO_CELSIUS_OFFSET;
}

double HaltechProtocol::decodePressure(const QByteArray& payload, int offset) {
    // Pressure stored as kPa * 10
    return decodeUint16(payload, offset) / SCALE_DIVIDE_BY_10;
}

//=============================================================================
// String Utilities
//=============================================================================

QString HaltechProtocol::toCamelCase(const QString& name) {
    QString result = name;
    result = result.replace(" ", "");

    if (!result.isEmpty()) {
        result[0] = result[0].toLower();
    }

    return result;
}

} // namespace devdash