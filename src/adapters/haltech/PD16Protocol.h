#pragma once

#include "core/IProtocolAdapter.h"

#include <QCanBusFrame>
#include <QHash>
#include <QString>

#include <array>
#include <functional>
#include <vector>

namespace devdash {

/**
 * @brief Haltech PD16 power distribution module CAN protocol decoder.
 *
 * The PD16 uses multiplexed CAN frames where byte 0 contains:
 * - Bits 7-5: IO Type (0=25A, 1=8A, 2=HBO, 3=SPI, 4=AVI)
 * - Bits 3-0: IO Index (0-15)
 *
 * Supports up to 4 PD16 devices (A, B, C, D) on a single CAN bus,
 * each with a unique base CAN ID.
 *
 * ## Design Pattern
 *
 * Uses lookup tables instead of switch statements for:
 * - Frame offset → decoder function mapping
 * - IO type → handler function mapping
 * - Device ID → base CAN ID mapping
 *
 * This follows the Open/Closed Principle - new frame types or IO types
 * can be added by extending the tables, not modifying decode logic.
 *
 * ## Usage
 *
 * @code
 * PD16Protocol pd16;
 * pd16.setDeviceId(PD16Protocol::DeviceId::A);
 *
 * // In frame handler:
 * auto channels = pd16.decode(canFrame);
 * for (const auto& [name, value] : channels) {
 *     emit channelUpdated(name, value);
 * }
 * @endcode
 *
 * @note Single Responsibility: Protocol parsing only, no I/O operations.
 */
class PD16Protocol {
public:
    /**
     * @brief PD16 device ID (determines base CAN address).
     *
     * Each PD16 on the bus must have a unique device ID.
     * The base CAN ID is calculated as: 0x6D0 + (deviceId * 8)
     */
    enum class DeviceId {
        A = 0, ///< Base ID 0x6D0
        B = 1, ///< Base ID 0x6D8 (offset +8)
        C = 2, ///< Base ID 0x6E0 (offset +16)
        D = 3  ///< Base ID 0x6E8 (offset +24)
    };

    /**
     * @brief IO type extracted from multiplexer byte.
     *
     * Determines which type of PD16 channel the frame data refers to.
     */
    enum class IOType {
        Output25A = 0,    ///< 25 Amp high current outputs (channels 1-4)
        Output8A = 1,     ///< 8 Amp high side outputs (channels 1-8)
        HalfBridge = 2,   ///< Half bridge outputs for DC motors
        SpeedPulse = 3,   ///< Speed/Pulse inputs
        AnalogVoltage = 4 ///< Analog voltage inputs
    };

    /// Decoder function signature for frame handlers
    using FrameDecoder = std::function<std::vector<std::pair<QString, ChannelValue>>(
        const QByteArray& payload, const QString& devicePrefix)>;

    /// IO type handler function signature
    using IOTypeHandler = std::function<std::vector<std::pair<QString, ChannelValue>>(
        const QByteArray& payload, const QString& channelPrefix)>;

    PD16Protocol();

    /**
     * @brief Set which PD16 device this decoder handles.
     *
     * Updates the base CAN ID and channel name prefix accordingly.
     *
     * @param id Device ID (A, B, C, or D)
     */
    void setDeviceId(DeviceId id);

    /**
     * @brief Get current device ID.
     * @return Currently configured device ID
     */
    [[nodiscard]] DeviceId deviceId() const { return m_deviceId; }

    /**
     * @brief Get base CAN ID for current device.
     * @return Base frame ID (e.g., 0x6D0 for device A)
     */
    [[nodiscard]] uint32_t baseId() const { return m_baseId; }

    /**
     * @brief Get device name prefix for channel naming.
     * @return Prefix string like "pd16_A"
     */
    [[nodiscard]] QString devicePrefix() const { return m_devicePrefix; }

    /**
     * @brief Load protocol definition from JSON file.
     *
     * Parses the JSON protocol definition and builds internal lookup
     * tables for efficient frame decoding. The JSON defines:
     * - Frame offsets and their meanings
     * - IO types and their channel layouts
     * - Byte positions, scaling factors, and units
     *
     * @param path Path to PD16 protocol JSON
     * @return true if loaded successfully
     *
     * @note Clears any previously loaded definitions
     * @note Must be called before decode() for full functionality
     */
    [[nodiscard]] bool loadDefinition(const QString& path);

    /**
     * @brief Check if protocol definition is loaded.
     * @return true if a protocol definition has been loaded
     */
    [[nodiscard]] bool isLoaded() const { return m_loaded; }

    /**
     * @brief Check if using hardcoded fallback decoders.
     * @return true if no JSON loaded and using built-in decoders
     */
    [[nodiscard]] bool usingFallback() const { return !m_loaded; }

    /**
     * @brief Decode a CAN frame into channel values.
     *
     * Only decodes frames matching the current device's base ID range
     * (baseId to baseId + 7).
     *
     * @param frame The CAN frame to decode
     * @return Vector of (channel name, value) pairs
     *
     * @note Returns empty vector for frames outside this device's ID range
     */
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decode(const QCanBusFrame& frame) const;

    /**
     * @brief Extract IO type from multiplexer byte.
     *
     * @param muxByte Byte 0 of CAN payload
     * @return IO type (bits 7-5)
     */
    [[nodiscard]] static IOType getMuxType(uint8_t muxByte);

    /**
     * @brief Extract IO index from multiplexer byte.
     *
     * @param muxByte Byte 0 of CAN payload
     * @return IO index 0-15 (bits 3-0)
     */
    [[nodiscard]] static uint8_t getMuxIndex(uint8_t muxByte);

    /**
     * @brief Get human-readable name for IO type.
     *
     * @param type The IO type enum value
     * @return Short string like "25A", "8A", "HBO", "SPI", "AVI"
     */
    [[nodiscard]] static QString getIOTypeName(IOType type);

    /**
     * @brief Decode a uint16 value from payload (big-endian).
     *
     * @param payload The frame payload
     * @param offset Byte offset into payload
     * @return Decoded value, or 0 if offset out of range
     */
    [[nodiscard]] static uint16_t decodeUint16(const QByteArray& payload, int offset);

private:
    DeviceId m_deviceId = DeviceId::A;
    uint32_t m_baseId = 0;
    QString m_devicePrefix;
    bool m_loaded = false;

    /// Frame offset → decoder function lookup table
    QHash<int, FrameDecoder> m_frameDecoders;

    /// IO type → status handler lookup table
    QHash<IOType, IOTypeHandler> m_outputStatusHandlers;

    /// IO type → input handler lookup table
    QHash<IOType, IOTypeHandler> m_inputStatusHandlers;

    /**
     * @brief Initialize decoder lookup tables.
     *
     * Called during construction to set up fallback decoders.
     * These are overwritten if loadDefinition() is called.
     */
    void initializeFallbackDecoders();

    /**
     * @brief Build decoder tables from loaded JSON definition.
     *
     * Called automatically after successful loadDefinition().
     */
    void buildDecoderTablesFromJson();

    /**
     * @brief Check if frame belongs to this device.
     *
     * @param frameId CAN frame ID to check
     * @return true if frameId is in range [baseId, baseId + 7]
     */
    [[nodiscard]] bool isDeviceFrame(uint32_t frameId) const;

    /**
     * @brief Get frame offset from base (0-7).
     *
     * @param frameId CAN frame ID
     * @return Offset 0-7, or -1 if not a device frame
     */
    [[nodiscard]] int getFrameOffset(uint32_t frameId) const;

    /**
     * @brief Build channel name prefix for a specific IO.
     *
     * @param ioType The IO type (25A, 8A, etc.)
     * @param ioIndex The IO index (0-15)
     * @return Full prefix like "pd16_A_25A_1"
     */
    [[nodiscard]] QString buildChannelPrefix(IOType ioType, uint8_t ioIndex) const;

    // Frame decoders (one per frame offset)
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decodeInputStatus(const QByteArray& payload, const QString& devicePrefix) const;

    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decodeOutputStatus(const QByteArray& payload, const QString& devicePrefix) const;

    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decodeDeviceStatus(const QByteArray& payload, const QString& devicePrefix) const;

    // IO type handlers for output status
    [[nodiscard]] static std::vector<std::pair<QString, ChannelValue>>
    decodeOutput25AStatus(const QByteArray& payload, const QString& channelPrefix);

    [[nodiscard]] static std::vector<std::pair<QString, ChannelValue>>
    decodeOutput8AStatus(const QByteArray& payload, const QString& channelPrefix);

    // IO type handlers for input status
    [[nodiscard]] static std::vector<std::pair<QString, ChannelValue>>
    decodeSpeedPulseStatus(const QByteArray& payload, const QString& channelPrefix);

    [[nodiscard]] static std::vector<std::pair<QString, ChannelValue>>
    decodeAnalogVoltageStatus(const QByteArray& payload, const QString& channelPrefix);
};

} // namespace devdash