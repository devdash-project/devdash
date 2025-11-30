# Dash Protocol - Unified Data Abstraction

## Overview

The **Dash Protocol** is the unified data abstraction layer that allows multiple data sources (CAN bus, I2C sensors, GPIO, serial) to feed telemetry data into the dashboard in a consistent way.

## Problem Statement

Not all vehicle data comes from the CAN bus:
- **CAN bus**: ECU data (RPM, coolant temp, throttle)
- **I2C/SPI**: Inclinometer (pitch/roll angles)
- **Analog/I2C**: Differential temperature sensors
- **GPIO**: Locker engagement status
- **Serial**: Transfer case mode, custom controllers

Each source has different:
- Update rates (1Hz to 100Hz)
- Data formats
- Connection methods
- Unit conventions

## Solution: IDataSource Interface

All data sources implement `IDataSource`:

```cpp
class IDataSource : public QObject {
    Q_OBJECT
public:
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

signals:
    void channelUpdated(const QString& channel, const ChannelValue& value);
};
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       Data Sources                           │
├─────────────┬─────────────┬─────────────┬─────────────┬─────┤
│  Haltech    │  OBD-II     │Inclinometer │  Diff Temp  │GPIO │
│  (CAN)      │  (CAN)      │  (I2C/SPI)  │  (Analog)   │     │
└──────┬──────┴──────┬──────┴──────┬──────┴──────┬──────┴──┬──┘
       │             │             │             │         │
       │   IDataSource Interface   │             │         │
       ▼             ▼             ▼             ▼         ▼
┌─────────────────────────────────────────────────────────────┐
│              Thread-Safe ChannelUpdateQueue                  │
│               (moodycamel::ConcurrentQueue)                 │
└─────────────────────────────────┬───────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────┐
│                      DataBroker                              │
│          (60Hz timer processes queue)                       │
│          (applies unit conversion)                          │
│          (exposes Q_PROPERTY to QML)                        │
└─────────────────────────────────┬───────────────────────────┘
                                  │
                                  ▼
                              QML Gauges
```

## Benefits

1. **Open/Closed Principle**: Add new data sources without modifying existing code
2. **Thread Safety**: Lock-free queue, no race conditions
3. **Decoupled Update Rates**: Sources update at their native rate, UI at 60Hz
4. **Testability**: Easy to mock data sources for testing

## Usage

### Profile Configuration

```json
{
  "dataSources": [
    {
      "type": "haltech",
      "interface": "vcan0",
      "protocol": "protocols/haltech-can-protocol-v2.35.json"
    },
    {
      "type": "inclinometer",
      "interface": "/dev/i2c-1",
      "address": "0x68"
    }
  ]
}
```

### Adding a New Data Source

```cpp
class InclinometerAdapter : public IDataSource {
    Q_OBJECT
public:
    bool start() override {
        // Initialize I2C connection
        // Start reading thread
        return true;
    }

    void readSensor() {
        double pitch = readPitchAngle();
        double roll = readRollAngle();

        emit channelUpdated("pitchAngle", {pitch, "deg", true});
        emit channelUpdated("rollAngle", {roll, "deg", true});
    }
};
```

## See Also

- [Unit Conversion](unit-conversion.md) - How units are handled
- [DataBroker](data-broker.md) - Queue processing and QML exposure
- [Channel Namespace](../reference/channel-namespace.md) - Standard channel names
