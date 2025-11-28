#pragma once

#include "core/IProtocolAdapter.h"

#include <QCanBusFrame>
#include <QHash>
#include <QString>

#include <vector>

namespace devdash {

/**
 * @brief Conversion types for CAN channel values
 *
 * Pattern-matched from JSON conversion strings at load time.
 */
enum class ConversionType {
    Identity,       ///< "x" - raw value unchanged
    DivideBy10,     ///< "x / 10" - divide by 10
    DivideBy1000,   ///< "x / 1000" - divide by 1000
    GaugePressure,  ///< "(x / 10) - 101.3" - gauge pressure from absolute
    KelvinToCelsius ///< Kelvin*10 to Celsius: (x / 10) - 273.15
};

/**
 * @brief Definition of a single channel within a CAN frame
 */
struct ChannelDefinition {
    QString name;                ///< Channel name (e.g., "RPM", "Coolant Temperature")
    std::vector<int> byteIndices; ///< Byte positions in frame (e.g., [0,1] for bytes 0-1)
    bool isSigned = false;       ///< Whether to interpret as signed integer
    QString units;               ///< Unit string (e.g., "RPM", "kPa", "K")
    ConversionType conversion = ConversionType::Identity; ///< Conversion to apply
};

/**
 * @brief Definition of a CAN frame and its channels
 */
struct FrameDefinition {
    uint32_t frameId = 0;        ///< CAN frame ID (e.g., 0x360)
    QString name;                ///< Human-readable name (e.g., "Engine Core 1")
    int rateHz = 0;              ///< Expected update rate in Hz
    std::vector<ChannelDefinition> channels; ///< Channels in this frame
};

/**
 * @brief Haltech CAN protocol decoder
 *
 * Decodes CAN frames according to Haltech protocol specification.
 * Supports loading protocol definitions from JSON files.
 * Single Responsibility: Protocol parsing only, no I/O.
 */
class HaltechProtocol {
  public:
    HaltechProtocol() = default;

    /**
     * @brief Load protocol definition from JSON file
     * @param path Path to protocol definition JSON
     * @return true if loaded successfully
     */
    [[nodiscard]] bool loadDefinition(const QString& path);

    /**
     * @brief Check if protocol definition is loaded
     */
    [[nodiscard]] bool isLoaded() const { return !m_frameDefinitions.isEmpty(); }

    /**
     * @brief Get list of known frame IDs
     */
    [[nodiscard]] QList<uint32_t> frameIds() const { return m_frameDefinitions.keys(); }

    /**
     * @brief Decode a CAN frame into channel values
     * @param frame The CAN frame to decode
     * @return Vector of (channel name, value) pairs decoded from the frame
     */
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decode(const QCanBusFrame& frame) const;

    /**
     * @brief Decode a uint16 value from payload at specified offset
     * @param payload The frame payload
     * @param offset Byte offset into payload
     * @return Decoded value, or 0 if offset is out of range
     */
    [[nodiscard]] static uint16_t decodeUint16(const QByteArray& payload, int offset);

    /**
     * @brief Decode a signed int16 value from payload
     */
    [[nodiscard]] static int16_t decodeInt16(const QByteArray& payload, int offset);

    /**
     * @brief Decode RPM from standard Haltech RPM frame
     */
    [[nodiscard]] static double decodeRpm(const QByteArray& payload);

    /**
     * @brief Decode temperature (Kelvin to Celsius conversion)
     */
    [[nodiscard]] static double decodeTemperature(const QByteArray& payload, int offset);

    /**
     * @brief Decode pressure (kPa)
     */
    [[nodiscard]] static double decodePressure(const QByteArray& payload, int offset);

    /**
     * @brief Parse conversion string to ConversionType enum
     */
    [[nodiscard]] static ConversionType parseConversion(const QString& formula);

    /**
     * @brief Apply conversion to raw value
     */
    [[nodiscard]] static double applyConversion(ConversionType type, double rawValue);

  private:
    QHash<uint32_t, FrameDefinition> m_frameDefinitions;

    /**
     * @brief Decode using JSON-defined frames (if loaded)
     */
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decodeFromDefinition(const QCanBusFrame& frame) const;

    /**
     * @brief Fallback decode using hardcoded frame definitions
     */
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decodeHardcoded(const QCanBusFrame& frame) const;
};

} // namespace devdash
