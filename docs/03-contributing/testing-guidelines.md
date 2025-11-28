# Testing Guidelines

This document outlines testing patterns, best practices, and conventions for /dev/dash.

## Testing Philosophy

/dev/dash follows a **test-driven development** approach with these principles:

- **Test behavior, not implementation** - Test what the code does, not how it does it
- **Test at the right level** - Unit tests for logic, integration tests for systems
- **Make tests readable** - Tests are documentation
- **Fast feedback** - Tests should run quickly
- **Isolated tests** - Tests shouldn't depend on each other

## Test Framework

We use **Catch2 v3** for most tests:

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Description of what is being tested", "[tags]") {
    // Arrange
    HaltechProtocol protocol;

    // Act
    auto result = protocol.decode("0x360", payload);

    // Assert
    REQUIRE(result["rpm"] == 3500.0);
}
```

Use **Qt Test** only when Qt-specific features are required (signals, event loop):

```cpp
#include <QTest>
#include <QSignalSpy>

class DataBrokerTest : public QObject {
    Q_OBJECT
private slots:
    void testRpmUpdate() {
        DataBroker broker;
        QSignalSpy spy(&broker, &DataBroker::rpmChanged);
        // ...
    }
};
```

## Test Organization

### File Structure

Tests mirror the source structure:

```
src/adapters/haltech/HaltechProtocol.cpp
→ tests/adapters/haltech/test_haltech_protocol.cpp

src/core/DataBroker.cpp
→ tests/core/test_data_broker.cpp
```

### Test Naming

**File names:** `test_<source_file>.cpp`

```
test_haltech_protocol.cpp
test_data_broker.cpp
test_simulator_adapter.cpp
```

**Test case names:** Descriptive sentences

```cpp
// Good: Clear, descriptive
TEST_CASE("HaltechProtocol decodes RPM from frame 0x360", "[haltech][protocol]")
TEST_CASE("DataBroker rejects invalid channel values", "[databroker]")
TEST_CASE("SimulatorAdapter generates data at 20Hz", "[simulator]")

// Bad: Vague
TEST_CASE("Test protocol", "[haltech]")
TEST_CASE("Test1", "[databroker]")
```

## Test Structure: Arrange-Act-Assert

Use the AAA pattern for clarity:

```cpp
TEST_CASE("HaltechProtocol decodes coolant temperature", "[haltech]") {
    // Arrange: Set up test data and objects
    HaltechProtocol protocol("protocols/haltech-can-protocol-v2.35.json");
    QByteArray payload = QByteArray::fromHex("0DAC012C00000000");

    // Act: Execute the code being tested
    auto channels = protocol.decode("0x360", payload);

    // Assert: Verify the results
    REQUIRE(channels.contains("coolantTemperature"));
    REQUIRE(channels["coolantTemperature"] == Catch::Approx(92.5).margin(0.1));
}
```

## Sections for Related Tests

Use `SECTION` to group related tests that share setup:

```cpp
TEST_CASE("DataBroker handles adapter updates", "[databroker]") {
    // Shared setup
    DataBroker broker;
    SimulatorAdapter adapter;
    broker.setAdapter(&adapter);

    SECTION("RPM updates trigger signal") {
        QSignalSpy spy(&broker, &DataBroker::rpmChanged);

        adapter.emitChannelUpdate("rpm", ChannelValue{3500.0, "RPM", true});

        REQUIRE(spy.count() == 1);
        REQUIRE(broker.rpm() == 3500.0);
    }

    SECTION("invalid data is rejected") {
        QSignalSpy spy(&broker, &DataBroker::rpmChanged);

        adapter.emitChannelUpdate("rpm", ChannelValue{-100.0, "RPM", false});

        REQUIRE(spy.count() == 0);  // No signal emitted
        REQUIRE(broker.rpm() == 0.0);  // Value unchanged
    }

    // Each SECTION runs independently with fresh setup
}
```

## Test Tags

Use tags to categorize tests:

```cpp
TEST_CASE("...", "[haltech]")           // Haltech-specific
TEST_CASE("...", "[obd2]")              // OBD-II-specific
TEST_CASE("...", "[protocol]")          // Protocol parsing
TEST_CASE("...", "[integration]")       // Integration tests
TEST_CASE("...", "[slow]")              // Long-running tests
TEST_CASE("...", "[hardware]")          // Requires real hardware

// Multiple tags
TEST_CASE("...", "[haltech][protocol][integration]")
```

**Running specific tags:**

```bash
# Run only haltech tests
ctest --test-dir build/dev -R haltech

# Run all protocol tests
./build/dev/test_haltech_protocol "[protocol]"

# Exclude slow tests
./build/dev/test_haltech_protocol "~[slow]"
```

## Assertions

### Basic Assertions

```cpp
REQUIRE(value == 3500);        // Must be true (test stops if fails)
CHECK(value == 3500);          // Should be true (test continues if fails)
REQUIRE_FALSE(adapter.isRunning());

// Use REQUIRE for critical assertions
// Use CHECK for multiple non-critical assertions
```

### Floating Point Comparisons

```cpp
// Good: Use Approx for floating point
REQUIRE(temperature == Catch::Approx(92.5).margin(0.1));
REQUIRE(pressure == Catch::Approx(350.0).epsilon(0.01));  // 1% tolerance

// Bad: Direct comparison (fails due to rounding)
REQUIRE(temperature == 92.5);  // ❌ May fail
```

### String Matching

```cpp
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;
using Catch::Matchers::EndsWith;

REQUIRE_THAT(errorMsg, ContainsSubstring("Failed to connect"));
REQUIRE_THAT(adapterName, StartsWith("Haltech"));
REQUIRE_THAT(fileName, EndsWith(".json"));
```

### Exception Testing

```cpp
// Require exception to be thrown
REQUIRE_THROWS_AS(protocol.decode("invalid", data), std::invalid_argument);

// Require specific exception message
REQUIRE_THROWS_WITH(protocol.decode("0xFFF", data), "Unknown frame ID");

// Require no exception
REQUIRE_NOTHROW(adapter.start());
```

## Testing Qt Signals

Use `QSignalSpy` to test signal emissions:

```cpp
#include <QSignalSpy>

TEST_CASE("Adapter emits connectionStateChanged on start", "[adapter]") {
    HaltechAdapter adapter(config);
    QSignalSpy spy(&adapter, &IProtocolAdapter::connectionStateChanged);

    SECTION("emits true on successful start") {
        REQUIRE(adapter.start());

        REQUIRE(spy.count() == 1);
        QList<QVariant> arguments = spy.takeFirst();
        REQUIRE(arguments.at(0).toBool() == true);
    }

    SECTION("emits false on stop") {
        adapter.start();
        spy.clear();  // Clear previous signals

        adapter.stop();

        REQUIRE(spy.count() == 1);
        REQUIRE(spy.takeFirst().at(0).toBool() == false);
    }
}
```

### Waiting for Asynchronous Signals

```cpp
TEST_CASE("Adapter receives data within 1 second", "[adapter][integration]") {
    HaltechAdapter adapter(config);
    adapter.start();

    QSignalSpy spy(&adapter, &IProtocolAdapter::channelUpdated);

    // Wait up to 1000ms for signal
    REQUIRE(spy.wait(1000));

    // Verify signal was emitted
    REQUIRE(spy.count() >= 1);
}
```

## Mock Objects

### Creating Mock Adapters

```cpp
class MockAdapter : public IProtocolAdapter {
    Q_OBJECT
public:
    bool start() override {
        m_running = true;
        emit connectionStateChanged(true);
        return true;
    }

    void stop() override {
        m_running = false;
        emit connectionStateChanged(false);
    }

    bool isRunning() const override { return m_running; }

    QString adapterName() const override { return "Mock"; }

    QStringList availableChannels() const override {
        return {"rpm", "coolantTemperature"};
    }

    std::optional<ChannelValue> getChannel(const QString& name) const override {
        auto it = m_channels.find(name);
        return (it != m_channels.end()) ? std::optional{it.value()} : std::nullopt;
    }

    // Test helper: Inject data
    void emitTestData(const QString& channel, double value) {
        ChannelValue channelValue{value, "", true};
        m_channels[channel] = channelValue;
        emit channelUpdated(channel, channelValue);
    }

private:
    bool m_running = false;
    QHash<QString, ChannelValue> m_channels;
};
```

**Usage:**

```cpp
TEST_CASE("DataBroker processes mock adapter data", "[databroker]") {
    DataBroker broker;
    MockAdapter mock;
    broker.setAdapter(&mock);

    mock.emitTestData("rpm", 5000.0);

    REQUIRE(broker.rpm() == 5000.0);
}
```

## Data-Driven Tests with Generators

Use Catch2 generators for data-driven tests:

```cpp
TEST_CASE("Protocol handles various RPM values", "[protocol]") {
    HaltechProtocol protocol("protocols/haltech-can-protocol-v2.35.json");

    // Test multiple RPM values
    auto rpm = GENERATE(0, 1000, 3500, 7000, 10000);

    INFO("Testing RPM: " << rpm);  // Shown if test fails

    // Encode RPM value into CAN payload
    quint16 rpmRaw = static_cast<quint16>(rpm);
    QByteArray payload;
    payload.append(static_cast<char>(rpmRaw >> 8));
    payload.append(static_cast<char>(rpmRaw & 0xFF));
    payload.append(QByteArray(6, 0));  // Padding

    auto channels = protocol.decode("0x360", payload);

    REQUIRE(channels["rpm"] == rpm);
}
```

**Multiple generators:**

```cpp
TEST_CASE("Temperature conversion", "[protocol]") {
    auto celsius = GENERATE(0, 25, 50, 75, 100);
    auto expected = celsius * 9.0 / 5.0 + 32.0;

    double fahrenheit = convertToFahrenheit(celsius);

    REQUIRE(fahrenheit == Catch::Approx(expected));
}
```

## Testing Edge Cases

### Always Test

- ✅ **Zero values**
- ✅ **Maximum values**
- ✅ **Invalid input** (negative, out of range, malformed)
- ✅ **Boundary conditions** (min-1, min, min+1, max-1, max, max+1)
- ✅ **Null/empty** (nullptr, empty strings, empty containers)

```cpp
TEST_CASE("Protocol validates RPM range", "[protocol]") {
    HaltechProtocol protocol;

    SECTION("zero RPM is valid") {
        auto result = protocol.decodeRPM(createPayload(0));
        REQUIRE(result == 0);
    }

    SECTION("maximum RPM is valid") {
        auto result = protocol.decodeRPM(createPayload(15000));
        REQUIRE(result == 15000);
    }

    SECTION("negative RPM is rejected") {
        auto result = protocol.decodeRPM(createPayload(-100));
        REQUIRE_FALSE(result.has_value());
    }

    SECTION("excessive RPM is rejected") {
        auto result = protocol.decodeRPM(createPayload(20000));
        REQUIRE_FALSE(result.has_value());
    }
}
```

## Integration Tests

### Testing with Virtual CAN

```cpp
TEST_CASE("HaltechAdapter reads from vcan0", "[haltech][integration][hardware]") {
    QJsonObject config;
    config["interface"] = "vcan0";

    HaltechAdapter adapter(config);
    REQUIRE(adapter.start());

    // Send test frame via cansend
    system("cansend vcan0 360#0DAC03E8000003E8");

    // Wait for adapter to process frame
    QSignalSpy spy(&adapter, &IProtocolAdapter::channelUpdated);
    REQUIRE(spy.wait(500));

    // Verify frame was decoded
    auto rpm = adapter.getChannel("rpm");
    REQUIRE(rpm.has_value());
    REQUIRE(rpm->value == 3500.0);

    adapter.stop();
}
```

### End-to-End Tests

```cpp
TEST_CASE("End-to-end data flow from adapter to DataBroker", "[integration]") {
    // Create real components
    DataBroker broker;
    SimulatorAdapter simulator;
    broker.setAdapter(&simulator);

    // Start adapter
    REQUIRE(simulator.start());

    // Wait for data to flow
    QSignalSpy spy(&broker, &DataBroker::rpmChanged);
    REQUIRE(spy.wait(1000));

    // Verify data reached DataBroker
    REQUIRE(broker.rpm() > 0.0);
    REQUIRE(broker.isConnected());

    simulator.stop();
}
```

## Testing Patterns

### Testing SOLID Compliance

**Test that adapters are swappable (Liskov Substitution Principle):**

```cpp
TEST_CASE("DataBroker works with any IProtocolAdapter", "[databroker][solid]") {
    DataBroker broker;

    SECTION("works with HaltechAdapter") {
        auto adapter = std::make_unique<HaltechAdapter>(config);
        broker.setAdapter(std::move(adapter));
        REQUIRE(broker.start());
    }

    SECTION("works with SimulatorAdapter") {
        auto adapter = std::make_unique<SimulatorAdapter>(config);
        broker.setAdapter(std::move(adapter));
        REQUIRE(broker.start());
    }

    // Both work identically - Liskov Substitution Principle
}
```

### Testing Input Validation (Defensive Programming)

```cpp
TEST_CASE("Adapter validates external data", "[adapter][defensive]") {
    HaltechAdapter adapter(config);

    SECTION("rejects negative RPM") {
        QByteArray payload = createInvalidPayload(-100);
        auto result = adapter.processFrame(payload);
        REQUIRE_FALSE(result.has_value());  // Rejected
    }

    SECTION("rejects out-of-range temperature") {
        QByteArray payload = createPayloadWithTemp(300);  // 300°C
        auto result = adapter.processFrame(payload);
        REQUIRE_FALSE(result.has_value());  // Rejected
    }

    SECTION("accepts valid data") {
        QByteArray payload = createValidPayload();
        auto result = adapter.processFrame(payload);
        REQUIRE(result.has_value());  // Accepted
    }
}
```

## Performance Testing

### Benchmarking (Future)

```cpp
// Using Catch2 BENCHMARK (Catch2 v3)
TEST_CASE("Protocol decode performance", "[protocol][benchmark]") {
    HaltechProtocol protocol;
    QByteArray payload = createTestPayload();

    BENCHMARK("decode frame 0x360") {
        return protocol.decode("0x360", payload);
    };

    // Outputs: ~50μs/iteration (example)
}
```

## Best Practices

### ✅ Do

- **Test behavior, not implementation** - Test what the code does, not how
- **Use descriptive test names** - Test name should explain what is being tested
- **Test one thing per test case** - Each test should verify one behavior
- **Use SECTION for related tests** - Share setup code with sections
- **Test edge cases** - Zero, max, invalid, null
- **Mock external dependencies** - Use MockAdapter instead of real CAN
- **Keep tests fast** - Tests should run in milliseconds
- **Make tests independent** - Order shouldn't matter

### ❌ Don't

- **Don't test private methods** - Test through public API
- **Don't depend on test order** - Tests must be independent
- **Don't use sleep()** - Use `QSignalSpy::wait()` or event loop
- **Don't hardcode paths** - Use relative paths from project root
- **Don't skip cleanup** - Ensure `stop()` is called even if test fails
- **Don't test Qt internals** - Trust Qt's own tests

## Test Coverage

### Running with Coverage (Optional)

```bash
# Configure with coverage flags
cmake --preset dev -DCMAKE_CXX_FLAGS="--coverage"

# Build and run tests
cmake --build build/dev
ctest --test-dir build/dev

# Generate coverage report
gcovr --root . --exclude tests/ --html --html-details -o coverage.html
```

### Coverage Goals

- **Aim for 80%+ coverage** for core logic
- **100% coverage** for critical paths (safety, data validation)
- **Lower coverage OK** for UI code, Qt boilerplate

## CI Integration

Tests run automatically on every PR:

```yaml
# .github/workflows/ci.yml
- name: Test
  run: ctest --preset ci
```

**Local pre-push check:**

```bash
cmake --preset ci
cmake --build build/ci
ctest --preset ci --output-on-failure
```

## Debugging Tests

### Running Under GDB

```bash
gdb --args ./build/dev/test_haltech_protocol "specific test case"

# Inside gdb:
(gdb) break HaltechProtocol::decode
(gdb) run
(gdb) print payload
```

### VS Code Debugging

`.vscode/launch.json`:

```json
{
  "name": "Debug Test",
  "type": "cppdbg",
  "request": "launch",
  "program": "${workspaceFolder}/build/dev/test_haltech_protocol",
  "args": ["specific test case", "-d", "yes"],  // -d for debug output
  "cwd": "${workspaceFolder}",
  "MIMode": "gdb"
}
```

## Example Test File

**Complete example:** `tests/adapters/haltech/test_haltech_protocol.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "adapters/haltech/HaltechProtocol.h"

using namespace devdash::adapters;
using Catch::Matchers::WithinRel;

TEST_CASE("HaltechProtocol decodes CAN frames", "[haltech][protocol]") {
    HaltechProtocol protocol("protocols/haltech-can-protocol-v2.35.json");

    SECTION("decodes RPM from frame 0x360") {
        QByteArray payload = QByteArray::fromHex("0DAC03E8000003E8");

        auto channels = protocol.decode("0x360", payload);

        REQUIRE(channels.contains("rpm"));
        REQUIRE(channels["rpm"] == 3500.0);
    }

    SECTION("decodes coolant temperature") {
        QByteArray payload = QByteArray::fromHex("00000000012C0000");

        auto channels = protocol.decode("0x360", payload);

        REQUIRE(channels.contains("coolantTemperature"));
        REQUIRE_THAT(channels["coolantTemperature"],
                     WithinRel(92.5, 0.01));
    }

    SECTION("returns empty map for unknown frame ID") {
        QByteArray payload = QByteArray::fromHex("0DAC03E8000003E8");

        auto channels = protocol.decode("0xFFF", payload);

        REQUIRE(channels.empty());
    }
}
```

## Next Steps

- **Run tests**: [Running Tests](../00-getting-started/running-tests.md)
- **Add adapter tests**: [Adding Protocol Adapters](adding-adapters.md)
- **See test examples**: Browse `tests/adapters/haltech/`
