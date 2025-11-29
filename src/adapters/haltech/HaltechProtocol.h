#pragma once

#include "core/IProtocolAdapter.h"

#include <QCanBusFrame>
#include <QHash>
#include <QString>

#include <functional>
#include <vector>

namespace devdash {

/**
 * @brief Conversion types for CAN channel values.
 *
 * Pattern-matched from JSON conversion strings at load time.
 * Each type represents a specific mathematical transformation
 * applied to raw CAN data.
 */
enum class ConversionType {
    Identity,       ///< "x" - raw value unchanged
    DivideBy10,     ///< "x / 10" - divide by 10
    DivideBy1000,   ///< "x / 1000" - divide by 1000
    GaugePressure,  ///< "(x / 10) - 101.3" - gauge pressure from absolute
    KelvinToCelsius ///< Kelvin*10 to Celsius: (x / 10) - 273.15
};

/**
 * @brief Definition of a single channel within a CAN frame.
 *
 * Represents one data field extracted from a CAN frame payload,
 * including its location, signedness, units, and conversion formula.
 */
struct ChannelDefinition {
    QString name;                 ///< Channel name (e.g., "RPM", "Coolant Temperature")
    std::vector<int> byteIndices; ///< Byte positions in frame (e.g., [0,1] for bytes 0-1)
    bool isSigned = false;        ///< Whether to interpret as signed integer
    QString units;                ///< Unit string (e.g., "RPM", "kPa", "K")
    ConversionType conversion = ConversionType::Identity; ///< Conversion to apply
};

/**
 * @brief Definition of a CAN frame and its channels.
 *
 * Contains all information needed to decode a specific CAN frame ID,
 * including metadata and the list of channels it contains.
 */
struct FrameDefinition {
    uint32_t frameId = 0;                    ///< CAN frame ID (e.g., 0x360)
    QString name;                            ///< Human-readable name (e.g., "Engine Core 1")
    int rateHz = 0;                          ///< Expected update rate in Hz
    std::vector<ChannelDefinition> channels; ///< Channels in this frame
};

/**
 * @brief Haltech CAN protocol decoder.
 *
 * Decodes CAN frames according to Haltech protocol specification.
 * Protocol definitions are loaded from JSON files, making this class
 * fully data-driven and extensible without code changes.
 *
 * ## Design Pattern
 *
 * Uses a lookup table pattern instead of switch statements to map
 * frame IDs to decoder functions. This follows the Open/Closed Principle -
 * new frame types can be added via JSON without modifying this code.
 *
 * ## Usage
 *
 * @code
 * HaltechProtocol protocol;
 * if (!protocol.loadDefinition("protocols/haltech-v2.35.json")) {
 *     qCritical() << "Failed to load protocol";
 *     return;
 * }
 *
 * // In frame handler:
 * auto channels = protocol.decode(canFrame);
 * for (const auto& [name, value] : channels) {
 *     emit channelUpdated(name, value);
 * }
 * @endcode
 *
 * @note Single Responsibility: Protocol parsing only, no I/O operations.
 * @see HaltechAdapter for CAN bus I/O
 */
class HaltechProtocol {
public:
    /// Decoder function signature for frame handlers
    using FrameDecoder = std::function<std::vector<std::pair<QString, ChannelValue>>(
        const QByteArray& payload)>;

    HaltechProtocol();

    /**
     * @brief Load protocol definition from JSON file.
     *
     * Parses the JSON protocol definition and builds internal lookup
     * tables for efficient frame decoding. Must be called before decode().
     *
     * @param path Path to protocol definition JSON file
     * @return true if loaded successfully, false on parse error or missing file
     *
     * @note Clears any previously loaded definitions
     */
    [[nodiscard]] bool loadDefinition(const QString& path);

    /**
     * @brief Check if protocol definition is loaded.
     * @return true if a protocol definition has been successfully loaded
     */
    [[nodiscard]] bool isLoaded() const { return !m_frameDefinitions.isEmpty(); }

    /**
     * @brief Get list of known frame IDs.
     * @return List of frame IDs that can be decoded
     */
    [[nodiscard]] QList<uint32_t> frameIds() const { return m_frameDefinitions.keys(); }

    /**
     * @brief Decode a CAN frame into channel values.
     *
     * Looks up the frame ID in the decoder table and applies the
     * appropriate decoding logic. Unknown frame IDs return an empty vector.
     *
     * @param frame The CAN frame to decode
     * @return Vector of (channel name, value) pairs decoded from the frame
     *
     * @pre frame.isValid() should be true
     * @note Returns empty vector for unknown frame IDs (not an error)
     */
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decode(const QCanBusFrame& frame) const;

    /**
     * @brief Decode a uint16 value from payload at specified offset.
     *
     * Interprets two bytes as big-endian unsigned 16-bit integer
     * (Haltech standard byte order).
     *
     * @param payload The frame payload
     * @param offset Byte offset into payload
     * @return Decoded value, or 0 if offset is out of range
     */
    [[nodiscard]] static uint16_t decodeUint16(const QByteArray& payload, int offset);

    /**
     * @brief Decode a signed int16 value from payload.
     *
     * @param payload The frame payload
     * @param offset Byte offset into payload
     * @return Decoded signed value, or 0 if offset is out of range
     */
    [[nodiscard]] static int16_t decodeInt16(const QByteArray& payload, int offset);

    /**
     * @brief Decode RPM from payload (uint16 at offset 0, 1 RPM per bit).
     *
     * Convenience method for direct RPM extraction. Equivalent to
     * decodeUint16(payload, 0) but with semantic meaning.
     *
     * @param payload The frame payload (must be at least 2 bytes)
     * @return RPM value, or 0 if payload too short
     */
    [[nodiscard]] static double decodeRpm(const QByteArray& payload);

    /**
     * @brief Decode temperature from payload (Kelvin * 10 to Celsius).
     *
     * Haltech stores temperatures as Kelvin * 10. This method decodes
     * and converts to Celsius.
     *
     * @param payload The frame payload
     * @param offset Byte offset to the temperature value
     * @return Temperature in Celsius, or 0 if payload too short
     */
    [[nodiscard]] static double decodeTemperature(const QByteArray& payload, int offset);

    /**
     * @brief Decode pressure from payload (kPa * 10 to kPa).
     *
     * @param payload The frame payload
     * @param offset Byte offset to the pressure value
     * @return Pressure in kPa, or 0 if payload too short
     */
    [[nodiscard]] static double decodePressure(const QByteArray& payload, int offset);

    /**
     * @brief Parse conversion formula string to ConversionType enum.
     *
     * Recognizes common Haltech conversion patterns:
     * - "x" or empty → Identity
     * - "x / 10" → DivideBy10
     * - "x / 1000" → DivideBy1000
     * - Contains "101.3" → GaugePressure
     *
     * @param formula Conversion formula from JSON
     * @return Matching ConversionType enum value
     */
    [[nodiscard]] static ConversionType parseConversion(const QString& formula);

    /**
     * @brief Apply conversion to raw value.
     *
     * @param type The conversion type to apply
     * @param rawValue Raw integer value from CAN payload
     * @return Converted floating-point value in engineering units
     */
    [[nodiscard]] static double applyConversion(ConversionType type, double rawValue);

private:
    /// Frame definitions loaded from JSON, keyed by frame ID
    QHash<uint32_t, FrameDefinition> m_frameDefinitions;

    /// Decoder lookup table: frame ID → decoder function
    QHash<uint32_t, FrameDecoder> m_decoders;

    /**
     * @brief Build decoder lookup table from loaded frame definitions.
     *
     * Creates a FrameDecoder lambda for each frame definition that
     * knows how to extract and convert all channels in that frame.
     * Called automatically after loading definitions.
     */
    void buildDecoderTable();

    /**
     * @brief Create a decoder function for a specific frame definition.
     *
     * @param frameDef The frame definition to create a decoder for
     * @return Lambda that decodes frames matching this definition
     */
    [[nodiscard]] FrameDecoder createFrameDecoder(const FrameDefinition& frameDef) const;

    /**
     * @brief Decode a single channel from payload.
     *
     * @param channelDef Channel definition with byte positions and conversion
     * @param payload Raw CAN payload bytes
     * @return Decoded channel value, or nullopt if payload too short
     */
    [[nodiscard]] std::optional<ChannelValue> decodeChannel(
        const ChannelDefinition& channelDef,
        const QByteArray& payload) const;

    /**
     * @brief Convert channel name to camelCase.
     *
     * Transforms "Coolant Temperature" → "coolantTemperature"
     * for consistent property naming.
     *
     * @param name Original channel name from JSON
     * @return camelCase version of the name
     */
    [[nodiscard]] static QString toCamelCase(const QString& name);
};

} // namespace devdash