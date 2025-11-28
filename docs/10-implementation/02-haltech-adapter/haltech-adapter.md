# Haltech Adapter

## Overview

The Haltech adapter provides CAN bus protocol decoding for Haltech Elite ECUs and PD16 power distribution modules. It implements the `IProtocolAdapter` interface and uses JSON-driven protocol definitions for flexibility.

## Architecture

```
HaltechAdapter (CAN I/O)
    ├── HaltechProtocol (ECU broadcast decoding)
    │   └── Loads: haltech-can-protocol-v2.35.json
    └── PD16Protocol (Power distribution decoding)
        └── Loads: haltech-pd16-can-protocol.json
```

### Separation of Concerns

- **HaltechAdapter**: Manages CAN bus connection, receives frames
- **HaltechProtocol**: Stateless decoder for ECU broadcast frames
- **PD16Protocol**: Stateless decoder for PD16 multiplexed frames

## Supported Protocols

### Haltech ECU Broadcast Protocol v2.35

**Supported frames:**
- `0x360` - Engine Core 1 (RPM, MAP, TPS, Coolant Pressure)
- `0x361` - Pressures (Fuel, Oil, Engine Demand, Wastegate)
- `0x362` - Injection & Ignition (Duty cycles, timing)
- `0x370` - Vehicle Speed & Cam (Speed, Gear, Cam angles)
- `0x372` - Battery & Boost Target (Voltage, target boost, barometric)
- `0x3E0` - Temperatures 1 (Coolant, Air, Fuel, Oil)
- `0x3E2` - Fuel Level

**Bus speed:** 1 Mbit/s
**Frame format:** 11-bit CAN ID, big-endian byte order

### Haltech PD16 Protocol

The PD16 power distribution module uses **multiplexed frames** where byte 0 contains:
- Bits 7-5: IO Type (0=25A, 1=8A, 2=HBO, 3=SPI, 4=AVI)
- Bits 3-0: IO Index (0-15)

**Device addressing:**
- PD16_A: Base ID `0x6D0`
- PD16_B: Base ID `0x6D8` (offset +8)
- PD16_C: Base ID `0x6E0` (offset +16)
- PD16_D: Base ID `0x6E8` (offset +24)

**Frame types (TX from PD16):**
- `+3` Input Status (SPI/AVI voltage, frequency, duty cycle)
- `+4` Output Status (25A/8A/HBO voltage, current, pin state)
- `+5` Device Status (firmware version, status flags)
- `+6` Diagnostics (temperatures, voltages, serial number)

## JSON Protocol Definitions

Protocol definitions live in `protocols/haltech/`:
- `haltech-can-protocol-v2.35.json` - ECU broadcast protocol
- `haltech-pd16-can-protocol.json` - PD16 power distribution

### JSON Format

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
          "name": "Coolant Temperature",
          "bytes": [0, 1],
          "signed": false,
          "units": "K",
          "conversion": "x / 10"
        }
      ]
    }
  }
}
```

### Conversion Patterns

The decoder uses pattern matching on the `conversion` string:

| Pattern | ConversionType | Formula |
|---------|---------------|---------|
| `"x"` | Identity | Raw value unchanged |
| `"x / 10"` | DivideBy10 | `raw / 10.0` |
| `"x / 1000"` | DivideBy1000 | `raw / 1000.0` |
| `"(x / 10) - 101.3"` | GaugePressure | `(raw / 10.0) - 101.3` |
| units="K" | KelvinToCelsius | `(raw / 10.0) - 273.15` |

**Note:** Temperature conversion is determined by `units` field, not conversion string.

## Channel Naming

Channels are automatically converted to `camelCase`:
- `"RPM"` → `"rPM"` → `"rpm"`
- `"Coolant Temperature"` → `"CoolantTemperature"` → `"coolantTemperature"`
- `"Fuel Pressure"` → `"FuelPressure"` → `"fuelPressure"`

## Unit Conversions

### Temperature
- **Input:** Kelvin × 10 (e.g., `3632` = 363.2 K)
- **Output:** Celsius (e.g., `90.05°C`)
- **Conversion:** `(raw / 10.0) - 273.15`

### Pressure
Haltech transmits absolute pressure. Gauge pressure channels subtract atmospheric:
- **Gauge:** `(raw / 10.0) - 101.3` kPa
- **Absolute:** `raw / 10.0` kPa

### Percentages
- **Input:** Percent × 10 (e.g., `500` = 50.0%)
- **Output:** Percent (e.g., `50.0`)
- **Conversion:** `raw / 10.0`

## Usage

### Loading a Protocol

```cpp
#include "adapters/haltech/HaltechProtocol.h"

devdash::HaltechProtocol protocol;
if (!protocol.loadDefinition("protocols/haltech/haltech-can-protocol-v2.35.json")) {
    qCritical() << "Failed to load protocol definition";
    return;
}
```

### Decoding a CAN Frame

```cpp
QCanBusFrame frame(0x360, QByteArray::fromHex("0DAC03F501F40000"));
auto channels = protocol.decode(frame);

for (const auto& [name, value] : channels) {
    qDebug() << name << "=" << value.value << value.unit;
}
// Output:
// "rpm" = 3500 "RPM"
// "manifoldPressure" = 101.3 "kPa"
// "throttlePosition" = 50.0 "%"
```

### Using in HaltechAdapter

```cpp
class HaltechAdapter : public IProtocolAdapter {
    HaltechProtocol m_ecuProtocol;
    PD16Protocol m_pd16Protocol;

    void processFrame(const QCanBusFrame& frame) {
        // Determine which protocol based on frame ID
        if (frame.frameId() >= 0x360 && frame.frameId() <= 0x3FF) {
            // ECU broadcast
            auto channels = m_ecuProtocol.decode(frame);
            // Emit channel updates...
        } else if (frame.frameId() >= 0x6D0 && frame.frameId() <= 0x6FF) {
            // PD16
            auto channels = m_pd16Protocol.decode(frame);
            // Emit channel updates...
        }
    }
};
```

## Vehicle Profile Configuration

```json
{
  "name": "Moon Patrol",
  "protocol": {
    "adapter": "haltech",
    "config": {
      "interface": "can0",
      "bitrate": 1000000,
      "ecu_protocol": "protocols/haltech/haltech-can-protocol-v2.35.json",
      "pd16_enabled": true,
      "pd16_devices": ["A"]
    }
  }
}
```

## Testing

Run Haltech protocol tests:
```bash
ctest --test-dir build/dev -R haltech --output-on-failure
```

Test with mock CAN data:
```bash
./scripts/setup-vcan.sh
haltech-mock --interface vcan0 --scenario idle &
./build/devdash --profile profiles/example-haltech.json
```

## References

- [Haltech CAN Protocol v2.35 PDF](../../Haltech-CAN-Broadcast-Protocol-V2.35.0-1.pdf)
- [Haltech PD16 Protocol PDF](../../haltech-can-pd-16-protocol.pdf)
- Protocol JSON definitions: `protocols/haltech/`
