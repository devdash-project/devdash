# Protocol Adapters

## The Adapter Pattern

The Protocol Adapter pattern is the cornerstone of /dev/dash's extensibility. It allows the system to work with **any vehicle ECU** or data source by implementing a single interface.

## Why Adapters?

Different ECUs speak different languages:
- **Haltech**: CAN bus, 1 Mbit/s, custom frame format
- **OBD-II**: CAN/K-Line, standard PIDs
- **MegaSquirt**: CAN or serial, MS-specific protocol
- **Custom ECUs**: Anything you can imagine

Without adapters, the UI would need to know about every protocol. **That doesn't scale.**

## The Solution: IProtocolAdapter

All protocol adapters implement a single interface:

```cpp
class IProtocolAdapter : public QObject {
    Q_OBJECT
public:
    // Lifecycle
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    // Data access
    virtual std::optional<ChannelValue> getChannel(const QString& name) const = 0;
    virtual QStringList availableChannels() const = 0;
    virtual QString adapterName() const = 0;

signals:
    // Data updates
    void channelUpdated(const QString& channelName, const ChannelValue& value);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& message);
};
```

##  Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         QML UI Layer                                 │
│         (binds to DataBroker.rpm, .coolantTemp, etc.)                │
└─────────────────────────┬───────────────────────────────────────────┘
                          │ Q_PROPERTY bindings
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        DataBroker                                    │
│     - Only knows about IProtocolAdapter interface                    │
│     - Receives channelUpdated("rpm", value) signals                  │
│     - Exposes standard properties to QML                             │
└─────────────────────────┬───────────────────────────────────────────┘
                          │ setAdapter(IProtocolAdapter*)
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│               IProtocolAdapter (interface)                           │
│     - start(), stop(), getChannel()                                  │
│     - Signals: channelUpdated, connectionStateChanged                │
│     - Standard channel names: "rpm", "coolantTemperature", etc.      │
└────────────┬─────────────────────────────┬──────────────────────────┘
             │                             │
             ▼                             ▼
┌────────────────────────┐    ┌────────────────────────┐
│    HaltechAdapter      │    │   SimulatorAdapter     │
│  - CAN bus, ECU proto  │    │  - Mock data for dev   │
│  - HaltechProtocol     │    │  - Random/scripted     │
│  - PD16Protocol        │    │                        │
└────────────────────────┘    └────────────────────────┘
                                        │
                                        ▼ (future)
                              ┌────────────────────────┐
                              │     OBD2Adapter        │
                              │  - ELM327, ISO-TP      │
                              └────────────────────────┘
```

## Dependency Inversion Principle

This is the **"D" in SOLID**:

> High-level modules should not depend on low-level modules.
> Both should depend on abstractions (interfaces).

**In practice:**

- `DataBroker` (high-level) doesn't know about `HaltechAdapter` (low-level)
- Both depend on `IProtocolAdapter` (abstraction)
- You can swap adapters without changing `DataBroker` or QML

## Channel Names: The Shared Language

Adapters translate protocol-specific data into **standard channel names**:

| ECU-Specific | Standard Channel Name |
|--------------|----------------------|
| Haltech "RPM" (CAN frame 0x360) | `"rpm"` |
| OBD-II PID 0x0C | `"rpm"` |
| MegaSquirt "engine rpm" | `"rpm"` |

**Result:** QML always binds to `DataBroker.rpm` regardless of ECU.

### Standard Channel Names

| Channel | Type | Units | Example |
|---------|------|-------|---------|
| `rpm` | double | RPM | 3500.0 |
| `coolantTemperature` | double | °C | 92.5 |
| `oilPressure` | double | kPa | 350.0 |
| `throttlePosition` | double | % | 75.0 |
| `vehicleSpeed` | double | km/h | 65.0 |
| `gear` | int | - | 3 |
| `batteryVoltage` | double | V | 14.2 |

Adapters are responsible for:
- ✅ Emitting correct channel names
- ✅ Providing values in correct units
- ✅ Marking invalid data with `valid=false`

## How Adapters Are Created

The `ProtocolAdapterFactory` creates adapters based on the vehicle profile:

```cpp
// From vehicle profile JSON:
{
  "protocol": {
    "adapter": "haltech",  // or "obd2", "simulator", etc.
    "config": {
      "interface": "can0",
      "bitrate": 1000000
    }
  }
}

// Factory creates the adapter:
auto adapter = ProtocolAdapterFactory::create("haltech", config);

// DataBroker receives it:
dataBroker->setAdapter(std::move(adapter));
```

## Example: HaltechAdapter

```cpp
class HaltechAdapter : public IProtocolAdapter {
    Q_OBJECT
public:
    bool start() override {
        // Connect to CAN bus
        m_canDevice = QCanBus::instance()->createDevice("socketcan", "can0");
        if (!m_canDevice->connectDevice()) {
            return false;
        }
        connect(m_canDevice, &QCanBusDevice::framesReceived,
                this, &HaltechAdapter::onFramesReceived);
        return true;
    }

private slots:
    void onFramesReceived() {
        while (m_canDevice->framesAvailable()) {
            auto frame = m_canDevice->readFrame();

            // Decode using HaltechProtocol
            auto channels = m_protocol.decode(frame);

            // Emit standard channel names
            for (const auto& [name, value] : channels) {
                emit channelUpdated(name, value);
            }
        }
    }

private:
    QCanBusDevice* m_canDevice = nullptr;
    HaltechProtocol m_protocol;
};
```

**Key points:**
- ✅ Implements `IProtocolAdapter`
- ✅ Handles CAN bus I/O
- ✅ Uses `HaltechProtocol` to decode frames
- ✅ Emits `channelUpdated` with standard names

## Example: SimulatorAdapter

```cpp
class SimulatorAdapter : public IProtocolAdapter {
    Q_OBJECT
public:
    bool start() override {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &SimulatorAdapter::generateData);
        m_timer->start(50); // 20 Hz
        return true;
    }

private slots:
    void generateData() {
        // Generate fake data
        emit channelUpdated("rpm", ChannelValue{2500.0 + qrand() % 1000, "RPM", true});
        emit channelUpdated("coolantTemperature", ChannelValue{90.0, "°C", true});
        emit channelUpdated("vehicleSpeed", ChannelValue{65.0, "km/h", true});
    }

private:
    QTimer* m_timer = nullptr;
};
```

**No CAN bus, no hardware - just a timer emitting fake data.**

Perfect for:
- UI development
- Testing layouts
- Demos

## Benefits of This Design

### 1. Easy to Add New Adapters

Want to add OBD-II support?

1. Create `OBD2Adapter` implementing `IProtocolAdapter`
2. Register it in `ProtocolAdapterFactory`
3. Done!

No changes needed to:
- ❌ DataBroker
- ❌ QML UI
- ❌ Existing adapters

### 2. Easy to Test

```cpp
TEST_CASE("DataBroker receives RPM updates") {
    DataBroker broker;

    // Use mock adapter
    auto mockAdapter = std::make_unique<MockAdapter>();
    broker.setAdapter(std::move(mockAdapter));

    // Simulate channel update
    mockAdapter->emitChannelUpdate("rpm", ChannelValue{3500.0, "RPM", true});

    // Verify DataBroker updated
    REQUIRE(broker.rpm() == 3500.0);
}
```

### 3. Easy to Swap

Want to switch from Haltech to OBD-II? **Change one line:**

```diff
 {
   "protocol": {
-    "adapter": "haltech",
+    "adapter": "obd2",
     "config": {
       "interface": "can0"
     }
   }
 }
```

Everything else keeps working.

## Thread Safety

Adapters typically read from I/O (CAN bus, serial) on a background thread, but Qt's signal/slot mechanism automatically marshals `channelUpdated` signals to the main thread.

**You don't need to worry about threading** - Qt handles it.

## What About Adapter-Specific Features?

Some adapters have unique features (e.g., Haltech's PD16 power distribution). These are still exposed through standard channel names:

```cpp
// PD16 output channels
emit channelUpdated("pd16_A_25A_0_voltage", ChannelValue{12.5, "V", true});
emit channelUpdated("pd16_A_25A_0_current", ChannelValue{15.2, "A", true});
```

The channel name encodes the specificity, but the mechanism is the same.

## Next Steps

- **Implement your own adapter:** [Adding Protocol Adapters](../03-contributing/adding-adapters.md)
- **Understand DataBroker:** [DataBroker](data-broker.md)
- **See full API:** [Core Interfaces API](../02-api-reference/core-interfaces.md)
