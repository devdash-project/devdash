# Adding Protocol Adapters

This guide walks you through adding support for a new ECU or data source to /dev/dash.

## Prerequisites

Before you start, make sure you understand:

- **[Protocol Adapters Architecture](../01-architecture/protocol-adapters.md)** - The adapter pattern and `IProtocolAdapter` interface
- **[Core Interfaces](../02-api-reference/core-interfaces.md)** - `IProtocolAdapter` API details
- **C++20** and **Qt 6** basics - Signals, slots, Q_OBJECT macro

## Overview: What You'll Build

You'll create a new adapter that:

1. **Implements `IProtocolAdapter`** - The common interface all adapters share
2. **Connects to your data source** - CAN bus, serial port, network, etc.
3. **Emits standard channel names** - `"rpm"`, `"coolantTemperature"`, etc.
4. **Registers with the factory** - So it can be loaded from vehicle profiles

**Time estimate:** 2-4 hours for a basic adapter, more for complex protocols.

## Step 1: Create Adapter Directory

```bash
# From project root
mkdir -p src/adapters/yourecuname
mkdir -p tests/adapters/yourecuname
```

**Example:** For an OBD-II adapter, you'd create:
```bash
mkdir -p src/adapters/obd2
mkdir -p tests/adapters/obd2
```

## Step 2: Implement the Adapter Class

Create `src/adapters/yourecuname/YourEcuAdapter.h`:

```cpp
#pragma once

#include "core/IProtocolAdapter.h"
#include <QObject>
#include <QString>
#include <QHash>
#include <optional>

namespace devdash::adapters {

/**
 * @brief Protocol adapter for YourECU.
 *
 * Connects to YourECU via [CAN bus / serial / network] and translates
 * ECU-specific data into standard channel names.
 */
class YourEcuAdapter : public IProtocolAdapter {
    Q_OBJECT

public:
    /**
     * @brief Construct adapter with configuration.
     * @param config JSON object with adapter-specific settings
     * @param parent Qt parent object
     */
    explicit YourEcuAdapter(const QJsonObject& config, QObject* parent = nullptr);
    ~YourEcuAdapter() override;

    // IProtocolAdapter interface
    [[nodiscard]] bool start() override;
    void stop() override;
    [[nodiscard]] bool isRunning() const override;
    [[nodiscard]] std::optional<ChannelValue> getChannel(const QString& channelName) const override;
    [[nodiscard]] QStringList availableChannels() const override;
    [[nodiscard]] QString adapterName() const override;

private slots:
    /**
     * @brief Handle incoming data from ECU.
     *
     * Called when new data arrives from your ECU connection.
     * Decode the data and emit channelUpdated signals.
     */
    void onDataReceived();

private:
    /**
     * @brief Parse raw data and update channels.
     * @param data Raw data from ECU
     */
    void processData(const QByteArray& data);

    bool m_running = false;
    QHash<QString, ChannelValue> m_channels;

    // Your ECU-specific connection object
    // Examples:
    // - QCanBusDevice* m_canDevice = nullptr;
    // - QSerialPort* m_serialPort = nullptr;
    // - QTcpSocket* m_socket = nullptr;
};

} // namespace devdash::adapters
```

Create `src/adapters/yourecuname/YourEcuAdapter.cpp`:

```cpp
#include "YourEcuAdapter.h"
#include <QDebug>

namespace devdash::adapters {

YourEcuAdapter::YourEcuAdapter(const QJsonObject& config, QObject* parent)
    : IProtocolAdapter(parent)
{
    // Parse configuration
    // Example:
    // QString interface = config["interface"].toString("can0");
    // int bitrate = config["bitrate"].toInt(500000);

    // Initialize your connection object
    // m_canDevice = QCanBus::instance()->createDevice("socketcan", interface);
}

YourEcuAdapter::~YourEcuAdapter()
{
    stop();
}

bool YourEcuAdapter::start()
{
    if (m_running) {
        return true;
    }

    // Connect to your data source
    // Example for CAN:
    // if (!m_canDevice->connectDevice()) {
    //     qWarning() << "Failed to connect to CAN device:" << m_canDevice->errorString();
    //     emit errorOccurred("Failed to connect to CAN device");
    //     return false;
    // }

    // Connect signals for incoming data
    // connect(m_canDevice, &QCanBusDevice::framesReceived,
    //         this, &YourEcuAdapter::onDataReceived);

    m_running = true;
    emit connectionStateChanged(true);
    qInfo() << "YourEcuAdapter started";

    return true;
}

void YourEcuAdapter::stop()
{
    if (!m_running) {
        return;
    }

    // Disconnect from your data source
    // if (m_canDevice) {
    //     m_canDevice->disconnectDevice();
    // }

    m_running = false;
    emit connectionStateChanged(false);
    qInfo() << "YourEcuAdapter stopped";
}

bool YourEcuAdapter::isRunning() const
{
    return m_running;
}

std::optional<ChannelValue> YourEcuAdapter::getChannel(const QString& channelName) const
{
    auto it = m_channels.find(channelName);
    if (it != m_channels.end()) {
        return it.value();
    }
    return std::nullopt;
}

QStringList YourEcuAdapter::availableChannels() const
{
    return m_channels.keys();
}

QString YourEcuAdapter::adapterName() const
{
    return QStringLiteral("YourECU Adapter");
}

void YourEcuAdapter::onDataReceived()
{
    // Read data from your connection
    // Example for CAN:
    // while (m_canDevice->framesAvailable()) {
    //     QCanBusFrame frame = m_canDevice->readFrame();
    //     processData(frame.payload());
    // }
}

void YourEcuAdapter::processData(const QByteArray& data)
{
    // CRITICAL: Decode your ECU's protocol and emit standard channel names

    // Example: Decode RPM (big-endian uint16 at offset 0)
    if (data.size() >= 2) {
        quint16 rpmRaw = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
        double rpm = static_cast<double>(rpmRaw);

        // Validate data (ALWAYS validate external input!)
        if (rpm <= 10000.0) {  // Reasonable max RPM
            ChannelValue rpmValue{rpm, "RPM", true};
            m_channels["rpm"] = rpmValue;
            emit channelUpdated("rpm", rpmValue);
        } else {
            qWarning() << "Invalid RPM value received:" << rpm;
        }
    }

    // Example: Decode coolant temperature (Celsius, float at offset 2)
    if (data.size() >= 6) {
        // ... decode temperature
        // emit channelUpdated("coolantTemperature", tempValue);
    }

    // Repeat for all channels your ECU provides
}

} // namespace devdash::adapters
```

## Step 3: Register in Factory

Edit `src/adapters/ProtocolAdapterFactory.cpp`:

```cpp
#include "yourecuname/YourEcuAdapter.h"

// In the create() function, add your adapter:
std::unique_ptr<IProtocolAdapter>
ProtocolAdapterFactory::create(const QString& adapterType, const QJsonObject& config)
{
    if (adapterType == "haltech") {
        return std::make_unique<HaltechAdapter>(config);
    }
    if (adapterType == "simulator") {
        return std::make_unique<SimulatorAdapter>(config);
    }
    // ADD THIS:
    if (adapterType == "yourecu") {
        return std::make_unique<YourEcuAdapter>(config);
    }

    qWarning() << "Unknown adapter type:" << adapterType;
    return nullptr;
}
```

## Step 4: Update CMake

Edit `src/adapters/CMakeLists.txt`:

```cmake
target_sources(devdash_adapters PRIVATE
    # ... existing files ...

    # Add your adapter
    yourecuname/YourEcuAdapter.h
    yourecuname/YourEcuAdapter.cpp
)
```

## Step 5: Write Tests

Create `tests/adapters/yourecuname/test_yourecu_adapter.cpp`:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "adapters/yourecuname/YourEcuAdapter.h"
#include <QJsonObject>

using namespace devdash::adapters;

TEST_CASE("YourEcuAdapter basic functionality", "[adapters][yourecu]") {
    QJsonObject config;
    config["interface"] = "vcan0";

    YourEcuAdapter adapter(config);

    SECTION("adapter name is correct") {
        REQUIRE(adapter.adapterName() == "YourECU Adapter");
    }

    SECTION("starts and stops") {
        REQUIRE(adapter.start());
        REQUIRE(adapter.isRunning());

        adapter.stop();
        REQUIRE_FALSE(adapter.isRunning());
    }

    SECTION("getChannel returns nullopt for unknown channel") {
        auto value = adapter.getChannel("nonexistent");
        REQUIRE_FALSE(value.has_value());
    }
}

TEST_CASE("YourEcuAdapter decodes data correctly", "[adapters][yourecu]") {
    QJsonObject config;
    YourEcuAdapter adapter(config);

    SECTION("decodes RPM correctly") {
        // Create test data payload
        QByteArray testData;
        testData.append(0x0D);  // RPM high byte (3500 RPM)
        testData.append(0xAC);  // RPM low byte

        // Inject test data (you may need to add a test-only method)
        // adapter.processDataForTesting(testData);

        // Verify channel was updated
        // auto rpm = adapter.getChannel("rpm");
        // REQUIRE(rpm.has_value());
        // REQUIRE(rpm->value == 3500.0);
        // REQUIRE(rpm->unit == "RPM");
    }
}
```

Update `tests/CMakeLists.txt`:

```cmake
add_executable(test_yourecu_adapter
    adapters/yourecuname/test_yourecu_adapter.cpp
)
target_link_libraries(test_yourecu_adapter PRIVATE
    devdash_adapters
    Catch2::Catch2WithMain
    Qt6::Core
)
catch_discover_tests(test_yourecu_adapter)
```

## Step 6: Build and Test

```bash
# Configure
cmake --preset dev

# Build
cmake --build build/dev

# Run your tests
ctest --test-dir build/dev -R test_yourecu_adapter --output-on-failure

# Run all tests to ensure you didn't break anything
ctest --test-dir build/dev --output-on-failure
```

## Step 7: Create Example Vehicle Profile

Create `profiles/example-yourecu.json`:

```json
{
  "name": "Example YourECU Vehicle",
  "protocol": {
    "adapter": "yourecu",
    "config": {
      "interface": "can0",
      "bitrate": 500000
    }
  },
  "gauges": {
    "cluster": {
      "layout": "sport",
      "primary": "tachometer",
      "secondary": ["speed", "coolant", "oil"]
    }
  },
  "alerts": {
    "coolantTemperature": {
      "warning": 100,
      "critical": 110
    }
  }
}
```

## Step 8: Test with Real Hardware or Mock

```bash
# Set up virtual CAN (if using CAN bus)
./scripts/setup-vcan.sh

# Run with your profile
./build/dev/devdash --profile profiles/example-yourecu.json
```

## Channel Naming Conventions

**CRITICAL:** Always use standard channel names so QML doesn't need to know about your ECU.

### Standard Channels

| Channel Name | Type | Unit | Description |
|--------------|------|------|-------------|
| `rpm` | double | RPM | Engine RPM |
| `coolantTemperature` | double | °C | Coolant temperature |
| `oilPressure` | double | kPa | Oil pressure |
| `oilTemperature` | double | °C | Oil temperature |
| `throttlePosition` | double | % | Throttle position (0-100) |
| `vehicleSpeed` | double | km/h | Vehicle speed |
| `gear` | int | - | Current gear (0 = neutral) |
| `batteryVoltage` | double | V | Battery voltage |
| `fuelPressure` | double | kPa | Fuel pressure |
| `manifoldPressure` | double | kPa | Intake manifold pressure (MAP) |
| `airFuelRatio` | double | - | AFR (14.7 = stoichiometric) |
| `ignitionTiming` | double | ° | Ignition advance in degrees BTDC |

### ECU-Specific Channels

If your ECU has unique features, prefix them with your ECU name:

```cpp
// Good: Namespaced ECU-specific channels
emit channelUpdated("yourecu_boostTarget", ChannelValue{1.5, "bar", true});
emit channelUpdated("yourecu_lambdaTarget", ChannelValue{0.85, "", true});

// Bad: Conflicts with other ECUs
emit channelUpdated("boostTarget", ChannelValue{1.5, "bar", true});
```

## Optional: JSON Protocol Definitions

If your ECU has a complex, frame-based protocol (like Haltech), you can create a JSON protocol definition file.

See `protocols/haltech-can-protocol-v2.35.json` for an example.

This allows you to:
- Define frames and fields declaratively
- Reuse frame parsing logic
- Support multiple protocol versions

Create `protocols/yourecu-protocol.json` and a corresponding `YourEcuProtocol` class similar to `HaltechProtocol`.

## Common Pitfalls

### 1. Not Validating Data

```cpp
// BAD: Trust external data
double rpm = decodeRpm(data);
emit channelUpdated("rpm", ChannelValue{rpm, "RPM", true});

// GOOD: Validate before emitting
double rpm = decodeRpm(data);
if (rpm >= 0.0 && rpm <= 10000.0) {
    emit channelUpdated("rpm", ChannelValue{rpm, "RPM", true});
} else {
    qWarning() << "Invalid RPM:" << rpm;
}
```

### 2. Blocking the Main Thread

```cpp
// BAD: Blocking I/O on main thread
void YourEcuAdapter::start() {
    while (true) {
        auto data = m_socket->readAll();  // BLOCKS!
        processData(data);
    }
}

// GOOD: Use Qt signals for asynchronous I/O
void YourEcuAdapter::start() {
    connect(m_socket, &QTcpSocket::readyRead,
            this, &YourEcuAdapter::onDataReceived);
}
```

### 3. Forgetting to Emit `connectionStateChanged`

```cpp
bool YourEcuAdapter::start() {
    // ... connect to ECU ...
    m_running = true;
    emit connectionStateChanged(true);  // DON'T FORGET!
    return true;
}
```

### 4. Using ECU-Specific Channel Names

```cpp
// BAD: QML won't understand "EngineRPM"
emit channelUpdated("EngineRPM", rpmValue);

// GOOD: Use standard names
emit channelUpdated("rpm", rpmValue);
```

## Testing Checklist

Before submitting your adapter:

- [ ] All tests pass (`ctest --test-dir build/dev`)
- [ ] Adapter starts and stops without errors
- [ ] Channels emit standard names (`rpm`, not `RPM` or `engine_rpm`)
- [ ] Invalid data is rejected (doesn't crash or emit garbage)
- [ ] `connectionStateChanged` signal is emitted on connect/disconnect
- [ ] `errorOccurred` signal is emitted on failures
- [ ] Doxygen comments on all public methods
- [ ] Code passes clang-format (`cmake --build build/dev --target format-check`)
- [ ] Code passes clang-tidy (`cmake --build build/dev --target clang-tidy`)

## Next Steps

- **Test with real hardware** - Connect to your actual ECU
- **Profile for performance** - Ensure updates run at 20+ Hz
- **Add more channels** - Expand beyond the basic channels
- **Write integration tests** - Test with `DataBroker` and QML
- **Document your protocol** - Add comments explaining frame formats

## Resources

- **[IProtocolAdapter API](../02-api-reference/core-interfaces.md)** - Full interface documentation
- **[HaltechAdapter source](../../src/adapters/haltech/HaltechAdapter.cpp)** - Reference implementation
- **[SimulatorAdapter source](../../src/adapters/simulator/SimulatorAdapter.cpp)** - Minimal example
- **[Qt SerialBus](https://doc.qt.io/qt-6/qtserialbus-index.html)** - CAN bus and serial port APIs

## Getting Help

If you run into issues:

1. Check the [GitHub Issues](https://github.com/yourorg/devdash/issues)
2. Look at existing adapter implementations for reference
3. Ask in [GitHub Discussions](https://github.com/yourorg/devdash/discussions)
