#pragma once

#include "core/IProtocolAdapter.h"

#include <QCanBusFrame>
#include <QHash>
#include <QString>

#include <vector>

namespace devdash {

/**
 * @brief Haltech PD16 power distribution module CAN protocol decoder
 *
 * The PD16 uses multiplexed CAN frames where byte 0 contains:
 * - Bits 7-5: IO Type (0=25A, 1=8A, 2=HBO, 3=SPI, 4=AVI)
 * - Bits 3-0: IO Index (0-15)
 *
 * Supports up to 4 PD16 devices (A, B, C, D) on a single CAN bus.
 */
class PD16Protocol {
  public:
    /**
     * @brief PD16 device ID (determines base CAN address)
     */
    enum class DeviceId {
        A, ///< Base ID 0x6D0
        B, ///< Base ID 0x6D8 (offset +8)
        C, ///< Base ID 0x6E0 (offset +16)
        D  ///< Base ID 0x6E8 (offset +24)
    };

    /**
     * @brief IO type extracted from mux byte
     */
    enum class IOType {
        Output25A = 0,    ///< 25 Amp high current outputs
        Output8A = 1,     ///< 8 Amp high side outputs
        HalfBridge = 2,   ///< Half bridge outputs for DC motors
        SpeedPulse = 3,   ///< Speed/Pulse inputs
        AnalogVoltage = 4 ///< Analog voltage inputs
    };

    PD16Protocol() = default;

    /**
     * @brief Set which PD16 device this decoder handles
     * @param id Device ID (A, B, C, or D)
     */
    void setDeviceId(DeviceId id);

    /**
     * @brief Get current device ID
     */
    [[nodiscard]] DeviceId deviceId() const { return m_deviceId; }

    /**
     * @brief Get base CAN ID for current device
     */
    [[nodiscard]] uint32_t baseId() const { return m_baseId; }

    /**
     * @brief Load protocol definition from JSON file
     * @param path Path to PD16 protocol JSON
     * @return true if loaded successfully
     */
    [[nodiscard]] bool loadDefinition(const QString& path);

    /**
     * @brief Check if protocol definition is loaded
     */
    [[nodiscard]] bool isLoaded() const { return m_loaded; }

    /**
     * @brief Decode a CAN frame into channel values
     * @param frame The CAN frame to decode
     * @return Vector of (channel name, value) pairs
     *
     * @note Only decodes frames matching the current device's base ID
     */
    [[nodiscard]] std::vector<std::pair<QString, ChannelValue>>
    decode(const QCanBusFrame& frame) const;

    /**
     * @brief Extract IO type from mux byte
     * @param muxByte Byte 0 of CAN payload
     * @return IO type (bits 7-5)
     */
    [[nodiscard]] static IOType getMuxType(uint8_t muxByte) {
        return static_cast<IOType>((muxByte >> 5) & 0x07);
    }

    /**
     * @brief Extract IO index from mux byte
     * @param muxByte Byte 0 of CAN payload
     * @return IO index (bits 3-0)
     */
    [[nodiscard]] static uint8_t getMuxIndex(uint8_t muxByte) { return muxByte & 0x0F; }

    /**
     * @brief Get human-readable name for IO type
     */
    [[nodiscard]] static QString getIOTypeName(IOType type);

    /**
     * @brief Decode a uint16 value from payload (big-endian)
     */
    [[nodiscard]] static uint16_t decodeUint16(const QByteArray& payload, int offset);

  private:
    DeviceId m_deviceId = DeviceId::A;
    uint32_t m_baseId = 0x6D0;
    bool m_loaded = false;

    /**
     * @brief Check if frame belongs to this device
     */
    [[nodiscard]] bool isDeviceFrame(uint32_t frameId) const;

    /**
     * @brief Get frame offset from base (0-7)
     */
    [[nodiscard]] int getFrameOffset(uint32_t frameId) const;
};

} // namespace devdash
