# Standard Channel Namespace

## Overview

Canonical channel names that all data sources map to. Protocol-specific names (e.g., Haltech's "ECT") are mapped to standard names (e.g., "coolantTemp") in the profile configuration.

## Channel Definitions

### Engine

| Channel | Type | Unit | Range | Description |
|---------|------|------|-------|-------------|
| `rpm` | double | RPM | 0-20000 | Engine speed |
| `coolantTemp` | double | °C | -40-200 | Coolant temperature |
| `oilPressure` | double | kPa | 0-1000 | Oil pressure |
| `oilTemp` | double | °C | -40-200 | Oil temperature |
| `throttlePosition` | double | % | 0-100 | Throttle position |
| `manifoldPressure` | double | kPa | 0-500 | MAP sensor |

### Drivetrain

| Channel | Type | Unit | Range | Description |
|---------|------|------|-------|-------------|
| `gear` | int | - | -1-9 | Current gear (0=N, -1=R) |
| `vehicleSpeed` | double | km/h | 0-400 | Vehicle speed |
| `frontDiffTemp` | double | °C | -40-200 | Front diff temperature |
| `rearDiffTemp` | double | °C | -40-200 | Rear diff temperature |
| `transferCaseMode` | string | - | - | 2H, 4H, 4L |

### Lockers

| Channel | Type | Unit | Range | Description |
|---------|------|------|-------|-------------|
| `frontLockerEngaged` | bool | - | - | Front locker status |
| `rearLockerEngaged` | bool | - | - | Rear locker status |

### Orientation

| Channel | Type | Unit | Range | Description |
|---------|------|------|-------|-------------|
| `pitchAngle` | double | ° | -90-90 | Vehicle pitch |
| `rollAngle` | double | ° | -180-180 | Vehicle roll |
| `heading` | double | ° | 0-360 | Compass heading |

## Profile Mappings

Profiles map protocol-specific names to standard channels:

```json
{
  "channelMappings": {
    "ECT": "coolantTemp",
    "TPS": "throttlePosition",
    "RPM": "rpm",
    "Oil Pressure": "oilPressure"
  }
}
```

## Adding Custom Channels

Rock crawler example:

```json
{
  "channels": {
    "suspensionTravel_FL": {
      "unitType": "distance",
      "defaultUnit": "mm",
      "description": "Front left suspension travel"
    }
  }
}
```
