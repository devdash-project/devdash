#include "PD16Protocol.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace devdash {

namespace {
// Base CAN IDs for each PD16 device
constexpr uint32_t BASE_ID_A = 0x6D0;
constexpr uint32_t BASE_ID_B = 0x6D8;
constexpr uint32_t BASE_ID_C = 0x6E0;
constexpr uint32_t BASE_ID_D = 0x6E8;

// Frame offsets from base ID
// constexpr int OFFSET_OUTPUT_CONTROL = 0;    // RX: Output duty cycle/freq (high priority) - Reserved for future use
// constexpr int OFFSET_OUTPUT_CONFIG = 1;     // RX: Output/input configuration (low priority) - Reserved for future use
// constexpr int OFFSET_SYSTEM_CONFIG = 2;     // RX: System config flags - Reserved for future use
constexpr int OFFSET_INPUT_STATUS = 3;      // TX: Input state/voltage/freq (high priority)
constexpr int OFFSET_OUTPUT_STATUS = 4;     // TX: Output voltage/current/state
constexpr int OFFSET_DEVICE_STATUS = 5;     // TX: Firmware version/status
// constexpr int OFFSET_DIAGNOSTICS = 6;       // TX: Diagnostics/serial/temps - Reserved for future use
} // namespace

void PD16Protocol::setDeviceId(DeviceId id) {
    m_deviceId = id;
    switch (id) {
    case DeviceId::A:
        m_baseId = BASE_ID_A;
        break;
    case DeviceId::B:
        m_baseId = BASE_ID_B;
        break;
    case DeviceId::C:
        m_baseId = BASE_ID_C;
        break;
    case DeviceId::D:
        m_baseId = BASE_ID_D;
        break;
    }
}

bool PD16Protocol::loadDefinition(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open PD16 protocol definition:" << path;
        return false;
    }

    // For now, just validate the JSON exists
    // Full JSON parsing can be added later if needed
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse PD16 protocol definition:" << parseError.errorString();
        return false;
    }

    m_loaded = true;
    qDebug() << "Loaded PD16 protocol definition from" << path;
    return true;
}

bool PD16Protocol::isDeviceFrame(uint32_t frameId) const {
    return frameId >= m_baseId && frameId < m_baseId + 8;
}

int PD16Protocol::getFrameOffset(uint32_t frameId) const {
    if (!isDeviceFrame(frameId)) {
        return -1;
    }
    return static_cast<int>(frameId - m_baseId);
}

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decode(const QCanBusFrame& frame) const {
    std::vector<std::pair<QString, ChannelValue>> results;

    if (!frame.isValid() || frame.payload().size() < 1) {
        return results;
    }

    // Check if frame belongs to this device
    if (!isDeviceFrame(frame.frameId())) {
        return results;
    }

    const auto& payload = frame.payload();
    int offset = getFrameOffset(frame.frameId());

    // Decode based on frame offset
    switch (offset) {
    case OFFSET_INPUT_STATUS: {
        // Byte 0: Mux ID (type + index)
        uint8_t muxByte = static_cast<uint8_t>(payload[0]);
        IOType ioType = getMuxType(muxByte);
        uint8_t ioIndex = getMuxIndex(muxByte);

        QString baseName = QString("pd16_%1_%2_%3")
                               .arg(static_cast<char>('A' + static_cast<int>(m_deviceId)))
                               .arg(getIOTypeName(ioType))
                               .arg(ioIndex);

        if (ioType == IOType::SpeedPulse || ioType == IOType::AnalogVoltage) {
            // Byte 1 bit 0: State (ON/OFF)
            if (payload.size() >= 2) {
                bool state = (payload[1] & 0x01) != 0;
                results.emplace_back(baseName + "_state",
                                     ChannelValue{state ? 1.0 : 0.0, "", true});
            }

            // Bytes 2-3: Voltage (mV)
            if (payload.size() >= 4) {
                uint16_t voltageRaw = decodeUint16(payload, 2);
                double voltage = voltageRaw / 1000.0; // mV to V
                results.emplace_back(baseName + "_voltage", ChannelValue{voltage, "V", true});
            }

            // For SPI: Bytes 4-5: Duty cycle, Bytes 6-7: Frequency
            if (ioType == IOType::SpeedPulse && payload.size() >= 8) {
                uint16_t dutyRaw = decodeUint16(payload, 4);
                double duty = dutyRaw / 10.0; // 0.1% resolution
                results.emplace_back(baseName + "_dutyCycle", ChannelValue{duty, "%", true});

                uint16_t freq = decodeUint16(payload, 6);
                results.emplace_back(baseName + "_frequency", ChannelValue{static_cast<double>(freq), "Hz", true});
            }
        }
        break;
    }

    case OFFSET_OUTPUT_STATUS: {
        // Byte 0: Mux ID
        if (payload.size() < 2) {
            break;
        }

        uint8_t muxByte = static_cast<uint8_t>(payload[0]);
        IOType ioType = getMuxType(muxByte);
        uint8_t ioIndex = getMuxIndex(muxByte);

        QString baseName = QString("pd16_%1_%2_%3")
                               .arg(static_cast<char>('A' + static_cast<int>(m_deviceId)))
                               .arg(getIOTypeName(ioType))
                               .arg(ioIndex);

        if (ioType == IOType::Output25A) {
            // Byte 1: Load %
            if (payload.size() >= 2) {
                uint8_t load = static_cast<uint8_t>(payload[1]);
                results.emplace_back(baseName + "_load",
                                     ChannelValue{static_cast<double>(load), "%", true});
            }

            // Bytes 2-3: Voltage (mV)
            if (payload.size() >= 4) {
                uint16_t voltageRaw = decodeUint16(payload, 2);
                double voltage = voltageRaw / 1000.0;
                results.emplace_back(baseName + "_voltage", ChannelValue{voltage, "V", true});
            }

            // Bytes 4-5: Low side current (mA)
            if (payload.size() >= 6) {
                uint16_t currentRaw = decodeUint16(payload, 4);
                double current = currentRaw / 1000.0;
                results.emplace_back(baseName + "_currentLow", ChannelValue{current, "A", true});
            }

            // Byte 6: High side current (mA)
            if (payload.size() >= 7) {
                uint8_t currentRaw = static_cast<uint8_t>(payload[6]);
                double current = currentRaw / 1000.0;
                results.emplace_back(baseName + "_currentHigh",
                                     ChannelValue{current, "A", true});
            }

            // Byte 7: Retry count (bits 7-4) and Pin state (bits 3-0)
            if (payload.size() >= 8) {
                uint8_t statusByte = static_cast<uint8_t>(payload[7]);
                uint8_t retryCount = (statusByte >> 4) & 0x0F;
                uint8_t pinState = statusByte & 0x0F;

                results.emplace_back(baseName + "_retries",
                                     ChannelValue{static_cast<double>(retryCount), "", true});
                results.emplace_back(baseName + "_pinState",
                                     ChannelValue{static_cast<double>(pinState), "", true});
            }
        } else if (ioType == IOType::Output8A) {
            // Byte 1: Retry count (bits 7-3) and Pin state (bits 2-0)
            if (payload.size() >= 2) {
                uint8_t statusByte = static_cast<uint8_t>(payload[1]);
                uint8_t retryCount = (statusByte >> 3) & 0x1F;
                uint8_t pinState = statusByte & 0x07;

                results.emplace_back(baseName + "_retries",
                                     ChannelValue{static_cast<double>(retryCount), "", true});
                results.emplace_back(baseName + "_pinState",
                                     ChannelValue{static_cast<double>(pinState), "", true});
            }

            // Bytes 2-3: Voltage (mV)
            if (payload.size() >= 4) {
                uint16_t voltageRaw = decodeUint16(payload, 2);
                double voltage = voltageRaw / 1000.0;
                results.emplace_back(baseName + "_voltage", ChannelValue{voltage, "V", true});
            }

            // Bytes 4-5: Current (mA)
            if (payload.size() >= 6) {
                uint16_t currentRaw = decodeUint16(payload, 4);
                double current = currentRaw / 1000.0;
                results.emplace_back(baseName + "_current", ChannelValue{current, "A", true});
            }

            // Byte 6: Load %
            if (payload.size() >= 7) {
                uint8_t load = static_cast<uint8_t>(payload[6]);
                results.emplace_back(baseName + "_load",
                                     ChannelValue{static_cast<double>(load), "%", true});
            }
        }
        break;
    }

    case OFFSET_DEVICE_STATUS: {
        // Device-level status (not multiplexed)
        if (payload.size() < 5) {
            break;
        }

        QString deviceName =
            QString("pd16_%1").arg(static_cast<char>('A' + static_cast<int>(m_deviceId)));

        // Byte 0 bits 7-4: Status
        uint8_t status = (static_cast<uint8_t>(payload[0]) >> 4) & 0x0F;
        results.emplace_back(deviceName + "_status",
                             ChannelValue{static_cast<double>(status), "", true});

        // Firmware version from bytes 1-4
        uint8_t fwMajor = static_cast<uint8_t>(payload[1]) & 0x03;
        uint8_t fwMinor = static_cast<uint8_t>(payload[2]);
        uint8_t fwBugfix = static_cast<uint8_t>(payload[3]);

        double fwVersion = fwMajor + (fwMinor / 100.0) + (fwBugfix / 10000.0);
        results.emplace_back(deviceName + "_firmwareVersion",
                             ChannelValue{fwVersion, "", true});
        break;
    }

    default:
        // Other frame types not yet implemented
        break;
    }

    return results;
}

QString PD16Protocol::getIOTypeName(IOType type) {
    switch (type) {
    case IOType::Output25A:
        return "25A";
    case IOType::Output8A:
        return "8A";
    case IOType::HalfBridge:
        return "HBO";
    case IOType::SpeedPulse:
        return "SPI";
    case IOType::AnalogVoltage:
        return "AVI";
    }
    return "Unknown";
}

uint16_t PD16Protocol::decodeUint16(const QByteArray& payload, int offset) {
    if (offset + 2 > payload.size()) {
        return 0;
    }
    // Big-endian
    return static_cast<uint16_t>((static_cast<uint8_t>(payload[offset]) << 8) |
                                 static_cast<uint8_t>(payload[offset + 1]));
}

} // namespace devdash
