#include "PD16Protocol.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace devdash {

namespace {

//=============================================================================
// Device Configuration Constants
//=============================================================================

/// Base CAN ID for device A (others offset by multiples of 8)
constexpr uint32_t BASE_CAN_ID = 0x6D0;

/// Offset between device base IDs
constexpr uint32_t DEVICE_ID_OFFSET = 8;

/// Number of frame offsets per device
constexpr int FRAMES_PER_DEVICE = 8;

//=============================================================================
// Frame Offset Constants
//=============================================================================

/// Frame offset for input status (TX from PD16)
constexpr int FRAME_OFFSET_INPUT_STATUS = 3;

/// Frame offset for output status (TX from PD16)
constexpr int FRAME_OFFSET_OUTPUT_STATUS = 4;

/// Frame offset for device status (TX from PD16)
constexpr int FRAME_OFFSET_DEVICE_STATUS = 5;

//=============================================================================
// Multiplexer Byte Layout
//=============================================================================

/// Bit shift for IO type in mux byte (bits 7-5)
constexpr int MUX_TYPE_SHIFT = 5;

/// Mask for IO type after shift
constexpr uint8_t MUX_TYPE_MASK = 0x07;

/// Mask for IO index (bits 3-0)
constexpr uint8_t MUX_INDEX_MASK = 0x0F;

//=============================================================================
// Payload Size Requirements
//=============================================================================

/// Minimum payload for input status frame
constexpr int INPUT_STATUS_MIN_SIZE = 2;

/// Payload size for speed pulse input (includes duty/freq)
constexpr int SPEED_PULSE_FULL_SIZE = 8;

/// Minimum payload for output status frame
constexpr int OUTPUT_STATUS_MIN_SIZE = 2;

/// Minimum payload for device status frame
constexpr int DEVICE_STATUS_MIN_SIZE = 5;

//=============================================================================
// Scaling Factors
//=============================================================================

/// Millivolts to volts conversion
constexpr double MV_TO_V = 1000.0;

/// Milliamps to amps conversion
constexpr double MA_TO_A = 1000.0;

/// Duty cycle resolution (0.1%)
constexpr double DUTY_CYCLE_SCALE = 10.0;

/// Firmware version scale factors
constexpr double FW_MINOR_SCALE = 100.0;
constexpr double FW_BUGFIX_SCALE = 10000.0;

//=============================================================================
// Bit Masks for Status Bytes
//=============================================================================

/// State bit in status byte
constexpr uint8_t STATE_BIT_MASK = 0x01;

/// Firmware major version mask (bits 1-0 of byte 1)
constexpr uint8_t FW_MAJOR_MASK = 0x03;

/// Status nibble mask (bits 7-4)
constexpr uint8_t STATUS_NIBBLE_MASK = 0x0F;

/// 25A retry count mask (bits 7-4 of byte 7)
constexpr int RETRY_COUNT_25A_SHIFT = 4;
constexpr uint8_t RETRY_COUNT_25A_MASK = 0x0F;

/// 8A retry count mask (bits 7-3 of byte 1)
constexpr int RETRY_COUNT_8A_SHIFT = 3;
constexpr uint8_t RETRY_COUNT_8A_MASK = 0x1F;

/// 8A pin state mask (bits 2-0)
constexpr uint8_t PIN_STATE_8A_MASK = 0x07;

//=============================================================================
// IO Type Name Lookup Table
//=============================================================================

/// Static table mapping IOType enum to string names
const std::array<QString, 5> IO_TYPE_NAMES = {
    QStringLiteral("25A"),  // Output25A
    QStringLiteral("8A"),   // Output8A
    QStringLiteral("HBO"),  // HalfBridge
    QStringLiteral("SPI"),  // SpeedPulse
    QStringLiteral("AVI")   // AnalogVoltage
};

} // anonymous namespace

//=============================================================================
// Construction & Initialization
//=============================================================================

PD16Protocol::PD16Protocol() {
    setDeviceId(DeviceId::A);
    initializeFallbackDecoders();
}

void PD16Protocol::initializeFallbackDecoders() {
    // Frame offset → decoder function mapping (fallback when no JSON loaded)
    m_frameDecoders[FRAME_OFFSET_INPUT_STATUS] =
        [this](const QByteArray& payload, const QString& prefix) {
            return decodeInputStatus(payload, prefix);
        };

    m_frameDecoders[FRAME_OFFSET_OUTPUT_STATUS] =
        [this](const QByteArray& payload, const QString& prefix) {
            return decodeOutputStatus(payload, prefix);
        };

    m_frameDecoders[FRAME_OFFSET_DEVICE_STATUS] =
        [this](const QByteArray& payload, const QString& prefix) {
            return decodeDeviceStatus(payload, prefix);
        };

    // IO type → output status handler mapping
    m_outputStatusHandlers[IOType::Output25A] = &PD16Protocol::decodeOutput25AStatus;
    m_outputStatusHandlers[IOType::Output8A] = &PD16Protocol::decodeOutput8AStatus;

    // IO type → input status handler mapping
    m_inputStatusHandlers[IOType::SpeedPulse] = &PD16Protocol::decodeSpeedPulseStatus;
    m_inputStatusHandlers[IOType::AnalogVoltage] = &PD16Protocol::decodeAnalogVoltageStatus;
}

//=============================================================================
// Device Configuration
//=============================================================================

void PD16Protocol::setDeviceId(DeviceId id) {
    m_deviceId = id;
    m_baseId = BASE_CAN_ID + (static_cast<uint32_t>(id) * DEVICE_ID_OFFSET);
    m_devicePrefix = QString("pd16_%1").arg(static_cast<char>('A' + static_cast<int>(id)));
}

bool PD16Protocol::loadDefinition(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "PD16Protocol: Failed to open protocol definition:" << path;
        return false;
    }

    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "PD16Protocol: Failed to parse protocol definition:"
                   << parseError.errorString();
        return false;
    }

    // Parse and build decoder tables from JSON
    buildDecoderTablesFromJson();

    m_loaded = true;
    qDebug() << "PD16Protocol: Loaded protocol definition from" << path;
    return true;
}

void PD16Protocol::buildDecoderTablesFromJson() {
    // Clear existing tables
    m_frameDecoders.clear();
    m_outputStatusHandlers.clear();
    m_inputStatusHandlers.clear();

    // TODO: Parse JSON and build tables dynamically
    // For now, rebuild the same handlers but from a "loaded" state
    // This structure supports future JSON-driven configuration

    // Frame decoders - these would be built from JSON "frames" section
    m_frameDecoders[FRAME_OFFSET_INPUT_STATUS] =
        [this](const QByteArray& payload, const QString& prefix) {
            return decodeInputStatus(payload, prefix);
        };

    m_frameDecoders[FRAME_OFFSET_OUTPUT_STATUS] =
        [this](const QByteArray& payload, const QString& prefix) {
            return decodeOutputStatus(payload, prefix);
        };

    m_frameDecoders[FRAME_OFFSET_DEVICE_STATUS] =
        [this](const QByteArray& payload, const QString& prefix) {
            return decodeDeviceStatus(payload, prefix);
        };

    // IO type handlers - these would be built from JSON "io_types" section
    m_outputStatusHandlers[IOType::Output25A] = &PD16Protocol::decodeOutput25AStatus;
    m_outputStatusHandlers[IOType::Output8A] = &PD16Protocol::decodeOutput8AStatus;
    m_inputStatusHandlers[IOType::SpeedPulse] = &PD16Protocol::decodeSpeedPulseStatus;
    m_inputStatusHandlers[IOType::AnalogVoltage] = &PD16Protocol::decodeAnalogVoltageStatus;
}

//=============================================================================
// Frame Identification
//=============================================================================

bool PD16Protocol::isDeviceFrame(uint32_t frameId) const {
    return frameId >= m_baseId && frameId < m_baseId + FRAMES_PER_DEVICE;
}

int PD16Protocol::getFrameOffset(uint32_t frameId) const {
    if (!isDeviceFrame(frameId)) {
        return -1;
    }
    return static_cast<int>(frameId - m_baseId);
}

//=============================================================================
// Main Decode Entry Point
//=============================================================================

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decode(const QCanBusFrame& frame) const {
    if (!frame.isValid() || frame.payload().isEmpty()) {
        return {};
    }

    // Check if frame belongs to this device
    if (!isDeviceFrame(frame.frameId())) {
        return {};
    }

    int offset = getFrameOffset(frame.frameId());

    // Lookup decoder for this frame offset
    auto it = m_frameDecoders.find(offset);
    if (it != m_frameDecoders.end()) {
        return it.value()(frame.payload(), m_devicePrefix);
    }

    // Unhandled frame offset - return empty (not an error)
    return {};
}

//=============================================================================
// Channel Name Building
//=============================================================================

QString PD16Protocol::buildChannelPrefix(IOType ioType, uint8_t ioIndex) const {
    return QString("%1_%2_%3")
        .arg(m_devicePrefix)
        .arg(getIOTypeName(ioType))
        .arg(ioIndex);
}

//=============================================================================
// Frame Decoders
//=============================================================================

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeInputStatus(const QByteArray& payload,
                                 const QString& /*devicePrefix*/) const {
    if (payload.size() < INPUT_STATUS_MIN_SIZE) {
        return {};
    }

    uint8_t muxByte = static_cast<uint8_t>(payload[0]);
    IOType ioType = getMuxType(muxByte);
    uint8_t ioIndex = getMuxIndex(muxByte);

    QString channelPrefix = buildChannelPrefix(ioType, ioIndex);

    // Lookup handler for this IO type
    auto it = m_inputStatusHandlers.find(ioType);
    if (it != m_inputStatusHandlers.end()) {
        return it.value()(payload, channelPrefix);
    }

    return {};
}

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeOutputStatus(const QByteArray& payload,
                                  const QString& /*devicePrefix*/) const {
    if (payload.size() < OUTPUT_STATUS_MIN_SIZE) {
        return {};
    }

    uint8_t muxByte = static_cast<uint8_t>(payload[0]);
    IOType ioType = getMuxType(muxByte);
    uint8_t ioIndex = getMuxIndex(muxByte);

    QString channelPrefix = buildChannelPrefix(ioType, ioIndex);

    // Lookup handler for this IO type
    auto it = m_outputStatusHandlers.find(ioType);
    if (it != m_outputStatusHandlers.end()) {
        return it.value()(payload, channelPrefix);
    }

    return {};
}

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeDeviceStatus(const QByteArray& payload,
                                  const QString& devicePrefix) const {
    if (payload.size() < DEVICE_STATUS_MIN_SIZE) {
        return {};
    }

    std::vector<std::pair<QString, ChannelValue>> results;

    // Byte 0 bits 7-4: Status
    uint8_t status = (static_cast<uint8_t>(payload[0]) >> 4) & STATUS_NIBBLE_MASK;
    results.emplace_back(
        devicePrefix + "_status",
        ChannelValue{static_cast<double>(status), "", true});

    // Firmware version from bytes 1-3
    uint8_t fwMajor = static_cast<uint8_t>(payload[1]) & FW_MAJOR_MASK;
    uint8_t fwMinor = static_cast<uint8_t>(payload[2]);
    uint8_t fwBugfix = static_cast<uint8_t>(payload[3]);

    double fwVersion = fwMajor + (fwMinor / FW_MINOR_SCALE) + (fwBugfix / FW_BUGFIX_SCALE);
    results.emplace_back(
        devicePrefix + "_firmwareVersion",
        ChannelValue{fwVersion, "", true});

    return results;
}

//=============================================================================
// IO Type Status Handlers
//=============================================================================

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeOutput25AStatus(const QByteArray& payload, const QString& prefix) {
    std::vector<std::pair<QString, ChannelValue>> results;

    // Byte 1: Load %
    if (payload.size() >= 2) {
        uint8_t load = static_cast<uint8_t>(payload[1]);
        results.emplace_back(prefix + "_load", ChannelValue{static_cast<double>(load), "%", true});
    }

    // Bytes 2-3: Voltage (mV)
    if (payload.size() >= 4) {
        uint16_t voltageRaw = decodeUint16(payload, 2);
        double voltage = voltageRaw / MV_TO_V;
        results.emplace_back(prefix + "_voltage", ChannelValue{voltage, "V", true});
    }

    // Bytes 4-5: Low side current (mA)
    if (payload.size() >= 6) {
        uint16_t currentRaw = decodeUint16(payload, 4);
        double current = currentRaw / MA_TO_A;
        results.emplace_back(prefix + "_currentLow", ChannelValue{current, "A", true});
    }

    // Byte 6: High side current (mA as uint8)
    if (payload.size() >= 7) {
        uint8_t currentRaw = static_cast<uint8_t>(payload[6]);
        double current = currentRaw / MA_TO_A;
        results.emplace_back(prefix + "_currentHigh", ChannelValue{current, "A", true});
    }

    // Byte 7: Retry count (bits 7-4) and Pin state (bits 3-0)
    if (payload.size() >= 8) {
        uint8_t statusByte = static_cast<uint8_t>(payload[7]);
        uint8_t retryCount = (statusByte >> RETRY_COUNT_25A_SHIFT) & RETRY_COUNT_25A_MASK;
        uint8_t pinState = statusByte & STATUS_NIBBLE_MASK;

        results.emplace_back(prefix + "_retries",
                             ChannelValue{static_cast<double>(retryCount), "", true});
        results.emplace_back(prefix + "_pinState",
                             ChannelValue{static_cast<double>(pinState), "", true});
    }

    return results;
}

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeOutput8AStatus(const QByteArray& payload, const QString& prefix) {
    std::vector<std::pair<QString, ChannelValue>> results;

    // Byte 1: Retry count (bits 7-3) and Pin state (bits 2-0)
    if (payload.size() >= 2) {
        uint8_t statusByte = static_cast<uint8_t>(payload[1]);
        uint8_t retryCount = (statusByte >> RETRY_COUNT_8A_SHIFT) & RETRY_COUNT_8A_MASK;
        uint8_t pinState = statusByte & PIN_STATE_8A_MASK;

        results.emplace_back(prefix + "_retries",
                             ChannelValue{static_cast<double>(retryCount), "", true});
        results.emplace_back(prefix + "_pinState",
                             ChannelValue{static_cast<double>(pinState), "", true});
    }

    // Bytes 2-3: Voltage (mV)
    if (payload.size() >= 4) {
        uint16_t voltageRaw = decodeUint16(payload, 2);
        double voltage = voltageRaw / MV_TO_V;
        results.emplace_back(prefix + "_voltage", ChannelValue{voltage, "V", true});
    }

    // Bytes 4-5: Current (mA)
    if (payload.size() >= 6) {
        uint16_t currentRaw = decodeUint16(payload, 4);
        double current = currentRaw / MA_TO_A;
        results.emplace_back(prefix + "_current", ChannelValue{current, "A", true});
    }

    // Byte 6: Load %
    if (payload.size() >= 7) {
        uint8_t load = static_cast<uint8_t>(payload[6]);
        results.emplace_back(prefix + "_load", ChannelValue{static_cast<double>(load), "%", true});
    }

    return results;
}

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeSpeedPulseStatus(const QByteArray& payload, const QString& prefix) {
    std::vector<std::pair<QString, ChannelValue>> results;

    // Byte 1 bit 0: State (ON/OFF)
    if (payload.size() >= 2) {
        bool state = (payload[1] & STATE_BIT_MASK) != 0;
        results.emplace_back(prefix + "_state", ChannelValue{state ? 1.0 : 0.0, "", true});
    }

    // Bytes 2-3: Voltage (mV)
    if (payload.size() >= 4) {
        uint16_t voltageRaw = decodeUint16(payload, 2);
        double voltage = voltageRaw / MV_TO_V;
        results.emplace_back(prefix + "_voltage", ChannelValue{voltage, "V", true});
    }

    // Bytes 4-5: Duty cycle (0.1% resolution)
    if (payload.size() >= 6) {
        uint16_t dutyRaw = decodeUint16(payload, 4);
        double duty = dutyRaw / DUTY_CYCLE_SCALE;
        results.emplace_back(prefix + "_dutyCycle", ChannelValue{duty, "%", true});
    }

    // Bytes 6-7: Frequency (Hz)
    if (payload.size() >= SPEED_PULSE_FULL_SIZE) {
        uint16_t freq = decodeUint16(payload, 6);
        results.emplace_back(prefix + "_frequency",
                             ChannelValue{static_cast<double>(freq), "Hz", true});
    }

    return results;
}

std::vector<std::pair<QString, ChannelValue>>
PD16Protocol::decodeAnalogVoltageStatus(const QByteArray& payload, const QString& prefix) {
    std::vector<std::pair<QString, ChannelValue>> results;

    // Byte 1 bit 0: State (ON/OFF)
    if (payload.size() >= 2) {
        bool state = (payload[1] & STATE_BIT_MASK) != 0;
        results.emplace_back(prefix + "_state", ChannelValue{state ? 1.0 : 0.0, "", true});
    }

    // Bytes 2-3: Voltage (mV)
    if (payload.size() >= 4) {
        uint16_t voltageRaw = decodeUint16(payload, 2);
        double voltage = voltageRaw / MV_TO_V;
        results.emplace_back(prefix + "_voltage", ChannelValue{voltage, "V", true});
    }

    return results;
}

//=============================================================================
// Static Utility Functions
//=============================================================================

PD16Protocol::IOType PD16Protocol::getMuxType(uint8_t muxByte) {
    return static_cast<IOType>((muxByte >> MUX_TYPE_SHIFT) & MUX_TYPE_MASK);
}

uint8_t PD16Protocol::getMuxIndex(uint8_t muxByte) {
    return muxByte & MUX_INDEX_MASK;
}

QString PD16Protocol::getIOTypeName(IOType type) {
    auto index = static_cast<size_t>(type);
    if (index < IO_TYPE_NAMES.size()) {
        return IO_TYPE_NAMES[index];
    }
    return QStringLiteral("Unknown");
}

uint16_t PD16Protocol::decodeUint16(const QByteArray& payload, int offset) {
    constexpr int UINT16_SIZE = 2;
    if (offset + UINT16_SIZE > payload.size()) {
        return 0;
    }
    // Big-endian
    return static_cast<uint16_t>(
        (static_cast<uint8_t>(payload[offset]) << 8) |
        static_cast<uint8_t>(payload[offset + 1]));
}

} // namespace devdash