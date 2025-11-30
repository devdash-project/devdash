# DataBroker

The **DataBroker** is the central data hub in /dev/dash. It sits between protocol adapters and the QML UI, aggregating vehicle data and exposing it to the user interface.

## Purpose

**Single Responsibility:** Data aggregation and QML exposure only.

The DataBroker:
- ✅ Receives channel updates from `IProtocolAdapter`
- ✅ Caches the latest values
- ✅ Exposes values to QML via `Q_PROPERTY`
- ✅ Emits signals when values change
- ✅ Handles unit conversion (future: user preferences)
- ❌ Does **not** know about specific adapters (depends only on `IProtocolAdapter` interface)
- ❌ Does **not** handle protocol parsing or I/O

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      QML UI Layer                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ Tachometer  │  │  Coolant    │  │   Speed     │         │
│  │ Text {      │  │  Gauge {    │  │   Text {    │         │
│  │   rpm       │  │   coolant   │  │   speed     │         │
│  │ }           │  │  }          │  │   }         │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└──────────┬─────────────┬───────────────┬────────────────────┘
           │ binds to    │ binds to      │ binds to
           │ DataBroker  │ DataBroker    │ DataBroker
           │ .rpm        │ .coolant...   │ .vehicleSpeed
           ▼             ▼               ▼
┌─────────────────────────────────────────────────────────────┐
│                       DataBroker                             │
│                                                              │
│  Q_PROPERTY(double rpm ...)                                  │
│  Q_PROPERTY(double coolantTemperature ...)                   │
│  Q_PROPERTY(double vehicleSpeed ...)                         │
│                                                              │
│  private slots:                                              │
│    onChannelUpdated(channelName, value) {                    │
│      if (channelName == "rpm") {                             │
│        m_rpm = value.value;                                  │
│        emit rpmChanged();                                    │
│      }                                                       │
│    }                                                         │
└──────────────────┬───────────────────────────────────────────┘
                   │ connects to signals
                   ▼
┌─────────────────────────────────────────────────────────────┐
│              IProtocolAdapter (interface)                    │
│                                                              │
│  signals:                                                    │
│    channelUpdated(QString channelName, ChannelValue value)   │
│    connectionStateChanged(bool connected)                    │
└──────────────────┬───────────────────────────────────────────┘
                   │ implemented by
                   ▼
┌─────────────────────────────────────────────────────────────┐
│   HaltechAdapter / SimulatorAdapter / OBD2Adapter            │
│                                                              │
│   emit channelUpdated("rpm", ChannelValue{3500.0, ...});     │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow

```
1. Adapter receives CAN frame
   ↓
2. Adapter decodes frame → standard channel names ("rpm", "coolantTemperature")
   ↓
3. Adapter emits: channelUpdated("rpm", ChannelValue{3500.0, "RPM", true})
   ↓
4. DataBroker::onChannelUpdated() receives signal
   ↓
5. DataBroker updates cached value: m_rpm = 3500.0
   ↓
6. DataBroker emits: rpmChanged()
   ↓
7. QML properties automatically update
   ↓
8. Gauges re-render with new value
```

## Q_PROPERTY Exposure

DataBroker exposes data to QML using Qt's property system:

```cpp
class DataBroker : public QObject {
    Q_OBJECT

    // Engine
    Q_PROPERTY(double rpm READ rpm NOTIFY rpmChanged)
    Q_PROPERTY(double throttlePosition READ throttlePosition NOTIFY throttlePositionChanged)
    Q_PROPERTY(double manifoldPressure READ manifoldPressure NOTIFY manifoldPressureChanged)

    // Temperatures
    Q_PROPERTY(double coolantTemperature READ coolantTemperature NOTIFY coolantTemperatureChanged)
    Q_PROPERTY(double oilTemperature READ oilTemperature NOTIFY oilTemperatureChanged)

    // ... more properties ...

public:
    // Getter methods
    [[nodiscard]] double rpm() const { return m_rpm; }
    [[nodiscard]] double coolantTemperature() const { return m_coolantTemperature; }

signals:
    // Change notification signals
    void rpmChanged();
    void coolantTemperatureChanged();

private:
    double m_rpm{0.0};
    double m_coolantTemperature{0.0};
};
```

### How QML Uses Properties

```qml
import QtQuick 2.15

// In QML
Text {
    // Automatically binds to DataBroker.rpm property
    text: DataBroker.rpm.toFixed(0)

    // Updates whenever rpmChanged() signal is emitted
}

Gauge {
    value: DataBroker.coolantTemperature
    minValue: 0
    maxValue: 120

    // Property binding is live - updates automatically
}
```

**The magic:** QML property bindings automatically re-evaluate when the `NOTIFY` signal is emitted. You don't need to manually refresh the UI.

## Connecting to Adapters

The DataBroker connects to the adapter's signals in `setAdapter()`:

```cpp
void DataBroker::setAdapter(std::unique_ptr<IProtocolAdapter> adapter) {
    // Disconnect old adapter if any
    if (m_adapter) {
        disconnect(m_adapter.get(), nullptr, this, nullptr);
    }

    // Store new adapter
    m_adapter = std::move(adapter);

    // Connect signals
    connect(m_adapter.get(), &IProtocolAdapter::channelUpdated,
            this, &DataBroker::onChannelUpdated);

    connect(m_adapter.get(), &IProtocolAdapter::connectionStateChanged,
            this, &DataBroker::onConnectionStateChanged);

    connect(m_adapter.get(), &IProtocolAdapter::errorOccurred,
            this, [](const QString& error) {
                qWarning() << "Adapter error:" << error;
            });
}
```

## Channel Update Handling

When a channel updates, DataBroker routes it to the correct property:

```cpp
void DataBroker::onChannelUpdated(const QString& channelName, const ChannelValue& value) {
    // Validate data
    if (!value.valid) {
        qWarning() << "Invalid data for channel:" << channelName;
        return;
    }

    // Route to appropriate property
    if (channelName == "rpm") {
        if (value.value != m_rpm) {
            m_rpm = value.value;
            emit rpmChanged();
        }
    } else if (channelName == "coolantTemperature") {
        if (value.value != m_coolantTemperature) {
            m_coolantTemperature = value.value;
            emit coolantTemperatureChanged();
        }
    }
    // ... handle all other channels ...
}
```

**Key points:**
- ✅ Validates `value.valid` flag before using data
- ✅ Only emits signal if value actually changed (prevents unnecessary QML updates)
- ✅ Logs warnings for invalid data

## Supported Channels

DataBroker currently exposes these properties to QML:

| Property | Type | Unit | Description |
|----------|------|------|-------------|
| `rpm` | double | RPM | Engine RPM |
| `throttlePosition` | double | % | Throttle position (0-100) |
| `manifoldPressure` | double | kPa | Intake manifold pressure |
| `coolantTemperature` | double | °C | Coolant temperature |
| `oilTemperature` | double | °C | Oil temperature |
| `intakeAirTemperature` | double | °C | Intake air temperature |
| `oilPressure` | double | kPa | Oil pressure |
| `fuelPressure` | double | kPa | Fuel pressure |
| `fuelLevel` | double | % | Fuel level (0-100) |
| `airFuelRatio` | double | - | Air-fuel ratio (14.7 = stoich) |
| `batteryVoltage` | double | V | Battery voltage |
| `vehicleSpeed` | double | km/h | Vehicle speed |
| `gear` | int | - | Current gear (0 = neutral) |
| `isConnected` | bool | - | Adapter connection state |

## Usage Example

### Setting Up DataBroker

```cpp
// In main.cpp or application initialization
auto dataBroker = new DataBroker(&app);

// Create adapter from vehicle profile
auto adapter = ProtocolAdapterFactory::createFromProfile("profiles/my-vehicle.json");

// Inject adapter into broker
dataBroker->setAdapter(std::move(adapter));

// Start receiving data
dataBroker->start();

// Expose to QML
engine.rootContext()->setContextProperty("DataBroker", dataBroker);
```

### Using in QML

```qml
// Tachometer.qml
import QtQuick 2.15

Item {
    Text {
        id: rpmText
        text: Math.round(DataBroker.rpm)  // Automatically updates
        font.pixelSize: 48
    }

    Rectangle {
        id: redline
        visible: DataBroker.rpm > 7000  // Live property binding
        color: "red"
    }

    Connections {
        target: DataBroker
        function onRpmChanged() {
            // Optional: React to changes with custom logic
            if (DataBroker.rpm > 7000) {
                shiftLightAnimation.start()
            }
        }
    }
}
```

## Thread Safety

DataBroker is **thread-safe by design** thanks to Qt's signal/slot mechanism:

- **Adapter thread**: Reads CAN bus, emits `channelUpdated` signal
- **Qt meta-object system**: Automatically marshals signal to main thread (queued connection)
- **Main thread**: DataBroker receives signal, updates properties, emits QML-visible signals

**You don't need to manually lock** - Qt handles cross-thread communication automatically.

### How Qt Marshals Signals

```
┌───────────────────────────┐
│  Adapter Thread           │
│  (background)             │
│                           │
│  emit channelUpdated(...) │  ← CAN bus read happens here
└──────────┬────────────────┘
           │ Qt queues signal to main thread
           │ (automatic if threads differ)
           ▼
┌───────────────────────────┐
│  Main Thread              │
│  (QML event loop)         │
│                           │
│  DataBroker receives      │  ← Property update happens here
│  onChannelUpdated()       │
│  emit rpmChanged()        │  ← QML updates happen here
└───────────────────────────┘
```

## Unit Conversion (Future Feature)

Currently, adapters are responsible for emitting data in the correct units (e.g., °C for temperature, kPa for pressure).

**Future enhancement:** DataBroker could handle unit conversion based on user preferences:

```cpp
// Future: User preference for temperature unit
enum class TemperatureUnit { Celsius, Fahrenheit };

double DataBroker::coolantTemperature() const {
    if (m_temperatureUnit == TemperatureUnit::Fahrenheit) {
        return m_coolantTemperature * 9.0 / 5.0 + 32.0;
    }
    return m_coolantTemperature;
}
```

This would allow adapters to emit in standard units (always °C) while users see their preferred units in the UI.

## Alert Threshold Evaluation (Future Feature)

DataBroker is a natural place to evaluate alert thresholds:

```cpp
void DataBroker::onChannelUpdated(const QString& channelName, const ChannelValue& value) {
    if (channelName == "coolantTemperature") {
        m_coolantTemperature = value.value;
        emit coolantTemperatureChanged();

        // Check alert thresholds
        if (m_coolantTemperature > m_coolantCriticalThreshold) {
            emit alertTriggered("coolant", AlertLevel::Critical);
        } else if (m_coolantTemperature > m_coolantWarningThreshold) {
            emit alertTriggered("coolant", AlertLevel::Warning);
        }
    }
}
```

This keeps alert logic centralized and testable.

## Testing DataBroker

### Unit Test Example

```cpp
#include <catch2/catch_test_macros.hpp>
#include <QSignalSpy>
#include "core/broker/DataBroker.h"
#include "adapters/simulator/SimulatorAdapter.h"

TEST_CASE("DataBroker receives adapter updates", "[databroker]") {
    DataBroker broker;
    auto adapter = std::make_unique<SimulatorAdapter>();
    auto* adapterPtr = adapter.get();  // Keep pointer before moving

    broker.setAdapter(std::move(adapter));
    broker.start();

    SECTION("RPM updates") {
        QSignalSpy spy(&broker, &DataBroker::rpmChanged);

        // Simulate adapter emitting data
        emit adapterPtr->channelUpdated("rpm", ChannelValue{3500.0, "RPM", true});

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.rpm() == 3500.0);
    }

    SECTION("invalid data is rejected") {
        QSignalSpy spy(&broker, &DataBroker::rpmChanged);

        // Emit invalid data
        emit adapterPtr->channelUpdated("rpm", ChannelValue{-100.0, "RPM", false});

        REQUIRE(spy.count() == 0);  // Signal not emitted
        REQUIRE(broker.rpm() == 0.0);  // Value unchanged
    }
}
```

### Testing with Mock Adapters

```cpp
class MockAdapter : public IProtocolAdapter {
    Q_OBJECT
public:
    void emitTestData(const QString& channel, double value) {
        emit channelUpdated(channel, ChannelValue{value, "", true});
    }

    bool start() override { return true; }
    void stop() override {}
    bool isRunning() const override { return true; }
    QString adapterName() const override { return "Mock"; }
    // ... implement other methods ...
};

TEST_CASE("DataBroker with mock adapter", "[databroker]") {
    DataBroker broker;
    auto mock = std::make_unique<MockAdapter>();
    auto* mockPtr = mock.get();

    broker.setAdapter(std::move(mock));

    mockPtr->emitTestData("rpm", 5000.0);
    REQUIRE(broker.rpm() == 5000.0);

    mockPtr->emitTestData("coolantTemperature", 95.0);
    REQUIRE(broker.coolantTemperature() == 95.0);
}
```

## Common Pitfalls

### 1. Forgetting to Start the Adapter

```cpp
// BAD: Adapter created but never started
dataBroker->setAdapter(std::move(adapter));
// No data will flow!

// GOOD: Start the adapter
dataBroker->setAdapter(std::move(adapter));
dataBroker->start();
```

### 2. Not Exposing to QML Context

```cpp
// BAD: DataBroker created but not exposed to QML
auto dataBroker = new DataBroker(&app);
engine.load(QUrl("qrc:/main.qml"));
// QML can't access DataBroker!

// GOOD: Expose to QML root context
engine.rootContext()->setContextProperty("DataBroker", dataBroker);
engine.load(QUrl("qrc:/main.qml"));
```

### 3. Breaking Property Bindings in QML

```qml
// BAD: Assignment breaks binding
Text {
    text: DataBroker.rpm.toFixed(0)
    Component.onCompleted: {
        text = "1000"  // Binding broken! Won't update anymore
    }
}

// GOOD: Preserve binding
Text {
    property int manualOverride: -1
    text: manualOverride >= 0 ? manualOverride : DataBroker.rpm.toFixed(0)
}
```

## Benefits of This Design

### 1. Decouples UI from Adapters

QML doesn't care if data comes from Haltech, OBD-II, or a simulator:

```qml
// Same QML works with any adapter
Text {
    text: DataBroker.rpm  // Works with Haltech, OBD-II, simulator, ...
}
```

### 2. Single Source of Truth

All vehicle data flows through DataBroker, making it easy to:
- Add logging
- Implement data recording
- Apply unit conversions
- Evaluate alert thresholds

### 3. Testable

DataBroker can be tested in isolation with mock adapters (no real CAN bus needed).

### 4. Thread-Safe

Qt's signal/slot mechanism handles cross-thread communication automatically.

## Next Steps

- **Understand how adapters work**: [Protocol Adapters](protocol-adapters.md)
- **See DataBroker API**: [DataBroker API Reference](../02-api-reference/data-broker-api.md)
- **Use DataBroker in QML**: [UI Layer](ui-layer.md)
