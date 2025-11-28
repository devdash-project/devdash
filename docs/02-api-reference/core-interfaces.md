# Core Interfaces API

This document describes the core C++ interfaces in /dev/dash.

## IProtocolAdapter

**Header:** `src/core/IProtocolAdapter.h`

The `IProtocolAdapter` interface is the cornerstone of /dev/dash's extensibility. All protocol adapters (Haltech, OBD-II, Simulator, etc.) implement this interface.

### Purpose

Enables the **Dependency Inversion Principle**:
- High-level modules (`DataBroker`, QML) depend on `IProtocolAdapter` interface
- Low-level modules (`HaltechAdapter`, `OBD2Adapter`) implement the interface
- Adapters are **swappable** without changing high-level code

### Class Declaration

```cpp
namespace devdash {

class IProtocolAdapter : public QObject {
    Q_OBJECT

public:
    explicit IProtocolAdapter(QObject* parent = nullptr);
    virtual ~IProtocolAdapter() = default;

    // Lifecycle methods
    [[nodiscard]] virtual bool start() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool isRunning() const = 0;

    // Data access methods
    [[nodiscard]] virtual std::optional<ChannelValue>
        getChannel(const QString& channelName) const = 0;
    [[nodiscard]] virtual QStringList availableChannels() const = 0;
    [[nodiscard]] virtual QString adapterName() const = 0;

signals:
    void channelUpdated(const QString& channelName, const ChannelValue& value);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& message);
};

} // namespace devdash
```

## Methods

### start()

```cpp
[[nodiscard]] virtual bool start() = 0;
```

**Purpose:** Initialize and start the adapter's data source (CAN bus, serial port, network, etc.).

**Returns:** `true` if started successfully, `false` on failure.

**Example implementation:**

```cpp
bool HaltechAdapter::start() {
    if (m_running) {
        return true;  // Already running
    }

    // Connect to CAN bus
    m_canDevice = QCanBus::instance()->createDevice("socketcan", "can0");
    if (!m_canDevice || !m_canDevice->connectDevice()) {
        emit errorOccurred("Failed to connect to CAN device");
        return false;
    }

    // Connect signals for incoming data
    connect(m_canDevice, &QCanBusDevice::framesReceived,
            this, &HaltechAdapter::onFramesReceived);

    m_running = true;
    emit connectionStateChanged(true);
    return true;
}
```

**Best practices:**
- ✅ Return `false` and emit `errorOccurred()` on failure
- ✅ Emit `connectionStateChanged(true)` on success
- ✅ Make idempotent (safe to call multiple times)
- ✅ Don't throw exceptions - return `false` instead

---

### stop()

```cpp
virtual void stop() = 0;
```

**Purpose:** Stop the adapter and release resources (disconnect CAN bus, close files, etc.).

**Example implementation:**

```cpp
void HaltechAdapter::stop() {
    if (!m_running) {
        return;  // Already stopped
    }

    if (m_canDevice) {
        m_canDevice->disconnectDevice();
        m_canDevice->deleteLater();
        m_canDevice = nullptr;
    }

    m_running = false;
    emit connectionStateChanged(false);
}
```

**Best practices:**
- ✅ Make idempotent (safe to call multiple times)
- ✅ Emit `connectionStateChanged(false)`
- ✅ Delete Qt objects with `deleteLater()` (not `delete`)
- ✅ Don't throw exceptions

---

### isRunning()

```cpp
[[nodiscard]] virtual bool isRunning() const = 0;
```

**Purpose:** Check if the adapter is currently running.

**Returns:** `true` if running, `false` otherwise.

**Example implementation:**

```cpp
bool HaltechAdapter::isRunning() const {
    return m_running;
}
```

---

### getChannel()

```cpp
[[nodiscard]] virtual std::optional<ChannelValue>
getChannel(const QString& channelName) const = 0;
```

**Purpose:** Get the current value of a channel by name.

**Parameters:**
- `channelName` - Channel name (e.g., `"rpm"`, `"coolantTemperature"`)

**Returns:** `std::optional<ChannelValue>` containing the value if available, or `std::nullopt` if not.

**Example implementation:**

```cpp
std::optional<ChannelValue> HaltechAdapter::getChannel(const QString& channelName) const {
    auto it = m_channels.find(channelName);
    if (it != m_channels.end()) {
        return it.value();
    }
    return std::nullopt;
}
```

**Usage:**

```cpp
auto rpm = adapter->getChannel("rpm");
if (rpm.has_value()) {
    qInfo() << "RPM:" << rpm->value << rpm->unit;
} else {
    qWarning() << "RPM not available";
}
```

**Best practices:**
- ✅ Return `std::nullopt` for unknown channels (don't throw)
- ✅ Validate channel name (case-sensitive)
- ✅ Use standard channel names (see [Standard Channels](#standard-channel-names))

---

### availableChannels()

```cpp
[[nodiscard]] virtual QStringList availableChannels() const = 0;
```

**Purpose:** Get a list of all channel names the adapter can provide.

**Returns:** `QStringList` of channel names.

**Example implementation:**

```cpp
QStringList HaltechAdapter::availableChannels() const {
    return m_channels.keys();
}
```

**Example output:**

```cpp
QStringList channels = adapter->availableChannels();
// ["rpm", "coolantTemperature", "oilPressure", "throttlePosition", ...]
```

---

### adapterName()

```cpp
[[nodiscard]] virtual QString adapterName() const = 0;
```

**Purpose:** Get a human-readable name for the adapter.

**Returns:** Adapter name as `QString`.

**Example implementation:**

```cpp
QString HaltechAdapter::adapterName() const {
    return QStringLiteral("Haltech Elite Adapter");
}
```

**Usage:** Displayed in UI, logs, error messages.

## Signals

### channelUpdated

```cpp
void channelUpdated(const QString& channelName, const ChannelValue& value);
```

**Purpose:** Emitted when a channel's value changes.

**Parameters:**
- `channelName` - Name of the updated channel
- `value` - New `ChannelValue`

**Example emission:**

```cpp
void HaltechAdapter::onFramesReceived() {
    while (m_canDevice->framesAvailable()) {
        QCanBusFrame frame = m_canDevice->readFrame();

        // Decode frame
        auto channels = m_protocol.decode(frame.frameId(), frame.payload());

        // Emit update for each channel
        for (auto it = channels.begin(); it != channels.end(); ++it) {
            ChannelValue value{it.value(), "RPM", true};
            m_channels[it.key()] = value;
            emit channelUpdated(it.key(), value);
        }
    }
}
```

**Connected by:**

`DataBroker` connects to this signal to receive updates:

```cpp
connect(adapter.get(), &IProtocolAdapter::channelUpdated,
        this, &DataBroker::onChannelUpdated);
```

**Best practices:**
- ✅ Emit for **every** channel update
- ✅ Use **standard channel names** (e.g., `"rpm"` not `"RPM"` or `"engineRPM"`)
- ✅ Only emit if value is **valid** (`value.valid == true`)
- ✅ Validate data **before** emitting (reject invalid sensor readings)

---

### connectionStateChanged

```cpp
void connectionStateChanged(bool connected);
```

**Purpose:** Emitted when adapter connection state changes.

**Parameters:**
- `connected` - `true` if connected, `false` if disconnected

**Example emission:**

```cpp
bool HaltechAdapter::start() {
    // ... connect to CAN bus ...
    m_running = true;
    emit connectionStateChanged(true);  // Connected!
    return true;
}

void HaltechAdapter::stop() {
    // ... disconnect from CAN bus ...
    m_running = false;
    emit connectionStateChanged(false);  // Disconnected
}
```

**Connected by:**

UI can react to connection state:

```qml
Connections {
    target: adapter
    function onConnectionStateChanged(connected) {
        if (!connected) {
            statusText.text = "Disconnected"
            statusText.color = "red"
        }
    }
}
```

---

### errorOccurred

```cpp
void errorOccurred(const QString& message);
```

**Purpose:** Emitted when an error occurs.

**Parameters:**
- `message` - Human-readable error description

**Example emission:**

```cpp
bool HaltechAdapter::start() {
    m_canDevice = QCanBus::instance()->createDevice("socketcan", "can0");
    if (!m_canDevice) {
        emit errorOccurred("Failed to create CAN device: socketcan plugin not found");
        return false;
    }

    if (!m_canDevice->connectDevice()) {
        QString error = QString("Failed to connect to can0: %1")
                            .arg(m_canDevice->errorString());
        emit errorOccurred(error);
        return false;
    }

    return true;
}
```

**Connected by:**

Application can log or display errors:

```cpp
connect(adapter.get(), &IProtocolAdapter::errorOccurred,
        this, [](const QString& error) {
            qCritical() << "Adapter error:" << error;
        });
```

## ChannelValue Struct

**Header:** `src/core/IProtocolAdapter.h`

Represents a channel's current value with metadata.

```cpp
struct ChannelValue {
    double value{0.0};  // Numeric value
    QString unit;       // Unit (e.g., "RPM", "°C", "kPa")
    bool valid{false};  // true if data is valid
};
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `value` | `double` | Numeric value of the channel |
| `unit` | `QString` | Unit string (e.g., `"RPM"`, `"°C"`, `"kPa"`, `"%"`) |
| `valid` | `bool` | `true` if data is valid, `false` if sensor error or no data |

### Usage Example

```cpp
// Creating a ChannelValue
ChannelValue rpm{3500.0, "RPM", true};
ChannelValue temp{92.5, "°C", true};
ChannelValue invalid{0.0, "", false};  // Sensor error

// Emitting with ChannelValue
emit channelUpdated("rpm", rpm);
emit channelUpdated("coolantTemperature", temp);

// Checking validity
if (value.valid) {
    qInfo() << "RPM:" << value.value << value.unit;
} else {
    qWarning() << "Invalid RPM data";
}
```

### Best Practices

- ✅ **Always set `valid`** - Set to `false` if sensor error or no data
- ✅ **Use standard units** - See [Standard Channels](#standard-channel-names) for unit conventions
- ✅ **Validate data** - Reject out-of-range values (set `valid = false`)
- ❌ **Don't emit invalid data** - Check `valid` before emitting `channelUpdated`

## Standard Channel Names

To ensure QML compatibility across adapters, use these **standard channel names**:

| Channel Name | Type | Unit | Range | Description |
|--------------|------|------|-------|-------------|
| `rpm` | double | `RPM` | 0-15000 | Engine RPM |
| `coolantTemperature` | double | `°C` | -40-150 | Coolant temperature |
| `oilTemperature` | double | `°C` | -40-150 | Oil temperature |
| `intakeAirTemperature` | double | `°C` | -40-100 | Intake air temperature |
| `oilPressure` | double | `kPa` | 0-1000 | Oil pressure |
| `fuelPressure` | double | `kPa` | 0-1000 | Fuel pressure |
| `manifoldPressure` | double | `kPa` | 0-500 | Intake manifold pressure (MAP) |
| `throttlePosition` | double | `%` | 0-100 | Throttle position |
| `vehicleSpeed` | double | `km/h` | 0-400 | Vehicle speed |
| `gear` | int | `-` | -1-8 | Current gear (-1 = reverse, 0 = neutral) |
| `batteryVoltage` | double | `V` | 0-20 | Battery voltage |
| `fuelLevel` | double | `%` | 0-100 | Fuel level percentage |
| `airFuelRatio` | double | `-` | 10-20 | Air-fuel ratio (14.7 = stoichiometric) |
| `ignitionTiming` | double | `°` | -20-60 | Ignition advance (degrees BTDC) |

### ECU-Specific Channels

For ECU-specific channels, **prefix with adapter name**:

```cpp
// Good: Namespaced channel names
emit channelUpdated("haltech_pd16_A_25A_0_current", value);
emit channelUpdated("obd2_pid_01_03", value);  // DTC status

// Bad: Conflicts with other adapters
emit channelUpdated("currentDraw", value);  // Ambiguous
```

## Thread Safety

`IProtocolAdapter` is designed for **automatic thread safety** via Qt's signal/slot mechanism:

### How It Works

```
┌────────────────────────────────────┐
│  Adapter Thread (background)       │
│                                    │
│  while (canDevice->framesAvail())  │
│    emit channelUpdated(...)        │  ← Signal emitted
└────────────┬───────────────────────┘
             │ Qt queues to main thread
             ▼
┌────────────────────────────────────┐
│  Main Thread (QML event loop)      │
│                                    │
│  DataBroker::onChannelUpdated()    │  ← Slot called
│  emit rpmChanged()                 │  ← QML updates
└────────────────────────────────────┘
```

**Key point:** You don't need manual locking. Qt marshals signals across threads automatically if the sender and receiver are on different threads.

### Example: Threaded Adapter

```cpp
class HaltechAdapter : public IProtocolAdapter {
private slots:
    void onFramesReceived() {
        // This runs on background thread
        while (m_canDevice->framesAvailable()) {
            auto frame = m_canDevice->readFrame();
            auto value = decodeRPM(frame);

            // Safe: Qt queues this to main thread automatically
            emit channelUpdated("rpm", value);
        }
    }
};
```

`DataBroker` receives the signal on the main thread:

```cpp
void DataBroker::onChannelUpdated(const QString& name, const ChannelValue& value) {
    // This ALWAYS runs on main thread (safe for QML)
    m_rpm = value.value;
    emit rpmChanged();  // QML updates
}
```

## Example Implementation

See [HaltechAdapter](../../src/adapters/haltech/HaltechAdapter.h) for a complete reference implementation.

**Minimal example:**

```cpp
#include "core/IProtocolAdapter.h"

class MyAdapter : public IProtocolAdapter {
    Q_OBJECT
public:
    explicit MyAdapter(const QJsonObject& config, QObject* parent = nullptr)
        : IProtocolAdapter(parent) {
        // Parse config
    }

    bool start() override {
        // Connect to data source
        m_running = true;
        emit connectionStateChanged(true);
        return true;
    }

    void stop() override {
        // Disconnect
        m_running = false;
        emit connectionStateChanged(false);
    }

    bool isRunning() const override {
        return m_running;
    }

    std::optional<ChannelValue> getChannel(const QString& name) const override {
        auto it = m_channels.find(name);
        return (it != m_channels.end()) ? std::optional{it.value()} : std::nullopt;
    }

    QStringList availableChannels() const override {
        return m_channels.keys();
    }

    QString adapterName() const override {
        return "My Custom Adapter";
    }

private:
    bool m_running = false;
    QHash<QString, ChannelValue> m_channels;
};
```

## Testing

### Unit Test Example

```cpp
#include <catch2/catch_test_macros.hpp>
#include "adapters/myecu/MyAdapter.h"

TEST_CASE("MyAdapter lifecycle", "[adapter]") {
    QJsonObject config;
    MyAdapter adapter(config);

    SECTION("starts and stops") {
        REQUIRE(adapter.start());
        REQUIRE(adapter.isRunning());

        adapter.stop();
        REQUIRE_FALSE(adapter.isRunning());
    }

    SECTION("emits connectionStateChanged") {
        QSignalSpy spy(&adapter, &IProtocolAdapter::connectionStateChanged);

        adapter.start();

        REQUIRE(spy.count() == 1);
        REQUIRE(spy.takeFirst().at(0).toBool() == true);
    }
}
```

## Next Steps

- **Implement an adapter**: [Adding Protocol Adapters](../03-contributing/adding-adapters.md)
- **See Haltech API**: [Haltech Protocol API](haltech-protocol.md)
- **Understand DataBroker**: [DataBroker Architecture](../01-architecture/data-broker.md)
