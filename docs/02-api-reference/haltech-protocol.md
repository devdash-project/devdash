# Haltech Protocol API

API reference for `HaltechProtocol` and `PD16Protocol` classes.

## Overview

The Haltech protocol decoder provides JSON-driven decoding of Haltech Elite ECU CAN frames and PD16 power distribution module frames.

**Key classes:**
- `HaltechProtocol` - Decodes ECU broadcast frames
- `PD16Protocol` - Decodes PD16 multiplexed frames

**Protocols supported:**
- Haltech CAN Broadcast Protocol v2.35 (1 Mbit/s, 11-bit IDs)
- Haltech PD16 Power Distribution Protocol

## HaltechProtocol Class

**Header:** `src/adapters/haltech/HaltechProtocol.h`

### Purpose

Stateless decoder for Haltech Elite ECU CAN broadcast frames. Uses JSON protocol definitions for flexibility across ECU models and firmware versions.

### Constructor

```cpp
HaltechProtocol();
```

Creates an empty protocol decoder. Must call `loadDefinition()` before use.

### loadDefinition()

```cpp
bool loadDefinition(const QString& jsonPath);
```

**Purpose:** Load protocol definition from JSON file.

**Parameters:**
- `jsonPath` - Path to JSON protocol definition (e.g., `"protocols/haltech-can-protocol-v2.35.json"`)

**Returns:** `true` if loaded successfully, `false` on error.

**Example:**

```cpp
HaltechProtocol protocol;
if (!protocol.loadDefinition("protocols/haltech-can-protocol-v2.35.json")) {
    qCritical() << "Failed to load protocol:" << protocol.errorString();
    return false;
}
```

### decode()

```cpp
QMap<QString, ChannelValue> decode(const QCanBusFrame& frame) const;
QMap<QString, ChannelValue> decode(quint32 frameId, const QByteArray& payload) const;
```

**Purpose:** Decode a CAN frame into channel values.

**Parameters:**
- `frame` - Complete CAN frame (ID + payload)
- `frameId` - Frame ID (e.g., `0x360`)
- `payload` - Frame payload (8 bytes)

**Returns:** Map of channel names to `ChannelValue` structs. Empty map if frame ID is unknown.

**Example:**

```cpp
QCanBusFrame frame(0x360, QByteArray::fromHex("0DAC03F501F40000"));
auto channels = protocol.decode(frame);

for (auto it = channels.begin(); it != channels.end(); ++it) {
    qDebug() << it.key() << "=" << it.value().value << it.value().unit;
}
// Output:
// "rpm" = 3500.0 "RPM"
// "manifoldPressure" = 101.3 "kPa"
// "throttlePosition" = 50.0 "%"
```

### Supported Frame IDs

| Frame ID | Name | Rate | Channels |
|----------|------|------|----------|
| `0x360` | Engine Core 1 | 50 Hz | RPM, MAP, TPS, Coolant Pressure |
| `0x361` | Pressures | 50 Hz | Fuel Pressure, Oil Pressure, Engine Demand, Wastegate |
| `0x362` | Injection & Ignition | 50 Hz | Duty cycles, timing |
| `0x370` | Vehicle Speed & Cam | 50 Hz | Speed, Gear, Cam angles |
| `0x372` | Battery & Boost | 50 Hz | Battery Voltage, Boost Target, Barometric |
| `0x3E0` | Temperatures 1 | 10 Hz | Coolant, Air, Fuel, Oil temperatures |
| `0x3E2` | Fuel Level | 10 Hz | Fuel level percentage |

## PD16Protocol Class

**Header:** `src/adapters/haltech/PD16Protocol.h`

### Purpose

Stateless decoder for Haltech PD16 power distribution module frames. Handles multiplexed frame format.

### Constructor

```cpp
PD16Protocol();
```

Creates an empty protocol decoder. Must call `loadDefinition()` before use.

### loadDefinition()

```cpp
bool loadDefinition(const QString& jsonPath);
```

**Purpose:** Load PD16 protocol definition from JSON file.

**Parameters:**
- `jsonPath` - Path to JSON protocol definition (e.g., `"protocols/haltech-pd16-can-protocol.json"`)

**Returns:** `true` if loaded successfully, `false` on error.

### decode()

```cpp
QMap<QString, ChannelValue> decode(const QCanBusFrame& frame) const;
QMap<QString, ChannelValue> decode(quint32 frameId, const QByteArray& payload) const;
```

**Purpose:** Decode a PD16 multiplexed CAN frame.

**Parameters:**
- `frame` - Complete CAN frame
- `frameId` - Frame ID (base + offset)
- `payload` - Frame payload (8 bytes, byte 0 is multiplexer)

**Returns:** Map of channel names to values.

**Example:**

```cpp
// PD16_A output status frame (0x6D4)
QCanBusFrame frame(0x6D4, QByteArray::fromHex("0000C8000064..."));
auto channels = pd16.decode(frame);

// Output channels like:
// "pd16_A_25A_0_voltage" = 12.8 "V"
// "pd16_A_25A_0_current" = 10.0 "A"
```

### PD16 Multiplexing

**Multiplexer byte (byte 0):**
- Bits 7-5: IO Type (0=25A, 1=8A, 2=HBO, 3=SPI, 4=AVI)
- Bits 3-0: IO Index (0-15)

**Device addressing:**
- PD16_A: Base ID `0x6D0`
- PD16_B: Base ID `0x6D8` (offset +8)
- PD16_C: Base ID `0x6E0` (offset +16)
- PD16_D: Base ID `0x6E8` (offset +24)

**Frame offsets (TX from PD16):**
- `+3` Input Status (SPI/AVI readings)
- `+4` Output Status (25A/8A/HBO readings)
- `+5` Device Status (firmware, flags)
- `+6` Diagnostics (temps, voltages, serial)

## JSON Protocol Format

### ECU Protocol Definition

```json
{
  "protocol": {
    "name": "Haltech CAN Broadcast Protocol",
    "version": "2.35.0",
    "bus_speed_bps": 1000000
  },
  "frames": {
    "0x360": {
      "name": "Engine Core 1",
      "rate_hz": 50,
      "channels": [
        {
          "name": "RPM",
          "bytes": [0, 1],
          "signed": false,
          "units": "RPM",
          "conversion": "x"
        },
        {
          "name": "Manifold Pressure",
          "bytes": [2, 3],
          "signed": false,
          "units": "kPa",
          "conversion": "x / 10"
        }
      ]
    }
  }
}
```

### Channel Field Reference

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Human-readable channel name (converted to camelCase) |
| `bytes` | array | Byte positions in payload [start, end] |
| `signed` | boolean | true if value is signed integer |
| `units` | string | Unit of measurement ("RPM", "kPa", "°C", "%") |
| `conversion` | string | Conversion formula pattern |

### Conversion Patterns

| Pattern | Description | Formula |
|---------|-------------|---------|
| `"x"` | Identity | Raw value unchanged |
| `"x / 10"` | Divide by 10 | `raw / 10.0` |
| `"x / 1000"` | Divide by 1000 | `raw / 1000.0` |
| `"(x / 10) - 101.3"` | Gauge pressure | `(raw / 10.0) - 101.3` |
| units=`"K"` | Kelvin to Celsius | `(raw / 10.0) - 273.15` |

**Note:** Temperature conversion is determined by `units` field, not conversion string.

## Unit Conversions

### Temperature

Haltech transmits temperatures in Kelvin × 10:

- **Input:** `3632` (raw)
- **Conversion:** `(3632 / 10.0) - 273.15 = 90.05`
- **Output:** `90.05 °C`

### Pressure

Haltech transmits absolute pressure. Gauge pressure subtracts atmospheric:

**Absolute pressure:**
- **Input:** `1013` (raw)
- **Conversion:** `1013 / 10.0 = 101.3`
- **Output:** `101.3 kPa`

**Gauge pressure:**
- **Input:** `1013` (raw)
- **Conversion:** `(1013 / 10.0) - 101.3 = 0.0`
- **Output:** `0.0 kPa` (atmospheric)

### Percentages

- **Input:** `500` (raw, 50.0%)
- **Conversion:** `500 / 10.0 = 50.0`
- **Output:** `50.0 %`

## Channel Naming

Channel names from JSON are automatically converted to `camelCase`:

```
"RPM" → "rpm"
"Coolant Temperature" → "coolantTemperature"
"Fuel Pressure" → "fuelPressure"
"Throttle Position" → "throttlePosition"
```

**Conversion rules:**
1. Split on spaces
2. PascalCase each word
3. Lowercase first letter

## Usage in Adapters

### Complete Example

```cpp
#include "adapters/haltech/HaltechProtocol.h"
#include "adapters/haltech/PD16Protocol.h"

class HaltechAdapter : public IProtocolAdapter {
    HaltechProtocol m_ecuProtocol;
    PD16Protocol m_pd16Protocol;
    QCanBusDevice* m_canDevice = nullptr;

public:
    bool start() override {
        // Load protocols
        if (!m_ecuProtocol.loadDefinition("protocols/haltech-can-protocol-v2.35.json")) {
            emit errorOccurred("Failed to load ECU protocol");
            return false;
        }

        if (!m_pd16Protocol.loadDefinition("protocols/haltech-pd16-can-protocol.json")) {
            emit errorOccurred("Failed to load PD16 protocol");
            return false;
        }

        // Connect to CAN
        m_canDevice = QCanBus::instance()->createDevice("socketcan", "can0");
        if (!m_canDevice->connectDevice()) {
            emit errorOccurred("Failed to connect to CAN");
            return false;
        }

        connect(m_canDevice, &QCanBusDevice::framesReceived,
                this, &HaltechAdapter::onFramesReceived);

        return true;
    }

private slots:
    void onFramesReceived() {
        while (m_canDevice->framesAvailable()) {
            QCanBusFrame frame = m_canDevice->readFrame();

            QMap<QString, ChannelValue> channels;

            // Determine protocol based on frame ID
            if (frame.frameId() >= 0x360 && frame.frameId() <= 0x3FF) {
                channels = m_ecuProtocol.decode(frame);
            } else if (frame.frameId() >= 0x6D0 && frame.frameId() <= 0x6FF) {
                channels = m_pd16Protocol.decode(frame);
            }

            // Emit channel updates
            for (auto it = channels.begin(); it != channels.end(); ++it) {
                emit channelUpdated(it.key(), it.value());
            }
        }
    }
};
```

## Testing

### Unit Test Example

```cpp
#include <catch2/catch_test_macros.hpp>
#include "adapters/haltech/HaltechProtocol.h"

TEST_CASE("HaltechProtocol decodes RPM", "[haltech][protocol]") {
    HaltechProtocol protocol;
    REQUIRE(protocol.loadDefinition("protocols/haltech-can-protocol-v2.35.json"));

    // Frame 0x360 with RPM = 3500 (0x0DAC)
    QByteArray payload = QByteArray::fromHex("0DAC03F501F40000");

    auto channels = protocol.decode(0x360, payload);

    REQUIRE(channels.contains("rpm"));
    REQUIRE(channels["rpm"].value == 3500.0);
    REQUIRE(channels["rpm"].unit == "RPM");
    REQUIRE(channels["rpm"].valid == true);
}
```

## References

- **Protocol specification:** `Haltech-CAN-Broadcast-Protocol-V2.35.0-1.pdf`
- **PD16 specification:** `haltech-can-pd-16-protocol.pdf`
- **JSON definitions:** `protocols/haltech/`

## Next Steps

- **Implement Haltech adapter**: [Adding Protocol Adapters](../03-contributing/adding-adapters.md)
- **See HaltechProtocol source**: `src/adapters/haltech/HaltechProtocol.h`
- **Understand adapters**: [Protocol Adapters](../01-architecture/protocol-adapters.md)
