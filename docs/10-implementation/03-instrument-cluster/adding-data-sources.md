# Developer Guide: Adding Data Sources

## Overview

Add support for new sensors/protocols by implementing `IDataSource`.

## Quick Start

### 1. Implement IDataSource

```cpp
// src/adapters/inclinometer/InclinometerAdapter.h
#pragma once
#include "core/interfaces/IDataSource.h"
#include <QThread>

class InclinometerAdapter : public IDataSource {
    Q_OBJECT

public:
    explicit InclinometerAdapter(const QJsonObject& config,
                                 QObject* parent = nullptr);
    ~InclinometerAdapter() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;

private:
    void readSensor();

    QThread* m_workerThread{nullptr};
    bool m_running{false};
    QString m_i2cDevice;
    uint8_t m_address;
};
```

### 2. Implement Reading Logic

```cpp
// src/adapters/inclinometer/InclinometerAdapter.cpp
#include "InclinometerAdapter.h"

bool InclinometerAdapter::start() {
    m_workerThread = new QThread(this);
    moveToThread(m_workerThread);
    m_workerThread->start();

    // Initialize I2C, start timer for periodic reads
    connect(&m_timer, &QTimer::timeout, this, &InclinometerAdapter::readSensor);
    m_timer.start(10);  // 100Hz

    m_running = true;
    return true;
}

void InclinometerAdapter::readSensor() {
    double pitch = readPitchFromI2C();  // Your I2C code
    double roll = readRollFromI2C();

    // Report in native units (radians from sensor)
    emit channelUpdated("pitchAngle", {pitch, "rad", true});
    emit channelUpdated("rollAngle", {roll, "rad", true});
}
```

### 3. Register in Factory

```cpp
// src/adapters/ProtocolAdapterFactory.cpp
#include "inclinometer/InclinometerAdapter.h"

const QHash<QString, AdapterCreator> ADAPTER_CREATORS = {
    // ... existing adapters ...
    {"inclinometer", [](const QJsonObject& config) {
        return std::make_unique<InclinometerAdapter>(config);
    }},
};
```

### 4. Use in Profile

```json
{
  "dataSources": [
    {
      "type": "inclinometer",
      "interface": "/dev/i2c-1",
      "address": "0x68"
    }
  ]
}
```

## Best Practices

### 1. Report Native Units

```cpp
// ✅ CORRECT - report Kelvin (Haltech native)
emit channelUpdated("coolantTemp", {373.15, "K", true});

// ❌ WRONG - don't convert in adapter
emit channelUpdated("coolantTemp", {100.0, "C", true});
```

### 2. Use Worker Threads

```cpp
// ✅ CORRECT - read on worker thread
void start() {
    m_thread = new QThread(this);
    m_reader = new SensorReader();
    m_reader->moveToThread(m_thread);
    m_thread->start();
}

// ❌ WRONG - blocking main thread
void start() {
    while (true) {
        auto data = blockingRead();  // Freezes UI!
    }
}
```

### 3. Handle Errors Gracefully

```cpp
void readSensor() {
    try {
        auto value = readFromHardware();
        emit channelUpdated("rpm", {value, "RPM", true});
    } catch (const std::exception& e) {
        qWarning() << "Sensor read failed:" << e.what();
        emit channelUpdated("rpm", {0, "RPM", false});  // Mark invalid
        emit errorOccurred(e.what());
    }
}
```

## Custom Unit Converter (Optional)

If your sensor uses proprietary units:

```cpp
class MyCustomConverter : public IUnitConverter {
public:
    double convert(double value, const QString& from, const QString& to) const override {
        if (from == "proprietary" && to == "kPa") {
            return value * 6.89476;  // Your conversion
        }
        return value;
    }
};

class MyAdapter : public IDataSource {
public:
    std::unique_ptr<IUnitConverter> createUnitConverter() override {
        return std::make_unique<MyCustomConverter>();
    }
};
```

## See Also

- [Dash Protocol](../../01-architecture/dash-protocol.md) - Architecture overview
- [Unit Conversion](../../01-architecture/unit-conversion.md) - How units work
- [Channel Namespace](../../reference/channel-namespace.md) - Standard channels
