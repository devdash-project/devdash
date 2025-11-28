# Running Tests

This guide covers how to run, write, and debug tests in /dev/dash.

## Quick Start

```bash
# Run all tests
ctest --test-dir build/dev --output-on-failure

# Run specific test
ctest --test-dir build/dev -R test_haltech_protocol --output-on-failure

# Run with verbose output
ctest --test-dir build/dev -V

# Run tests matching a pattern
ctest --test-dir build/dev -R "haltech|simulator"
```

## Test Framework

/dev/dash uses **Catch2 v3** for unit tests. Qt Test is only used when Qt-specific features (signals, event loop) are required.

### Why Catch2?

- **Modern C++20** syntax with sections and generators
- **Expressive assertions** with natural syntax
- **Single binary** execution (no Qt dependency for most tests)
- **BDD-style** organization with `SECTION`
- **Rich output** with colors and context

## Running Tests

### Using CTest

CTest is CMake's test runner that discovers and runs all tests:

```bash
# Configure first (if not already done)
cmake --preset dev

# Build tests
cmake --build build/dev

# Run all tests
ctest --test-dir build/dev --output-on-failure
```

**Output:**

```
Test project /workspaces/devdash/build/dev
    Start 1: test_haltech_protocol
1/3 Test #1: test_haltech_protocol ........   Passed    0.12 sec
    Start 2: test_simulator_adapter
2/3 Test #2: test_simulator_adapter .......   Passed    0.08 sec
    Start 3: test_data_broker
3/3 Test #3: test_data_broker .............   Passed    0.15 sec

100% tests passed, 0 tests failed out of 3
```

### Running a Single Test Binary

You can also run test binaries directly:

```bash
# Run specific test binary
./build/dev/test_haltech_protocol

# Run specific test case
./build/dev/test_haltech_protocol "HaltechProtocol decodes RPM correctly"

# List all test cases
./build/dev/test_haltech_protocol --list-tests

# Run with tags
./build/dev/test_haltech_protocol "[haltech]"
```

### Filtering Tests

```bash
# Run only haltech-related tests
ctest --test-dir build/dev -R haltech

# Exclude slow tests
ctest --test-dir build/dev -E slow

# Run tests 1-5
ctest --test-dir build/dev -I 1,5
```

## Test Organization

```
tests/
├── CMakeLists.txt              # Test configuration
├── core/
│   └── test_data_broker.cpp    # DataBroker tests
├── adapters/
│   ├── haltech/
│   │   ├── test_haltech_protocol.cpp
│   │   └── test_haltech_adapter.cpp
│   └── simulator/
│       └── test_simulator_adapter.cpp
└── integration/
    └── test_end_to_end.cpp     # Full system tests
```

Test files mirror the source structure:
- `src/adapters/haltech/HaltechProtocol.cpp` → `tests/adapters/haltech/test_haltech_protocol.cpp`

## Writing Tests

### Basic Test Structure

```cpp
#include <catch2/catch_test_macros.hpp>
#include "adapters/haltech/HaltechProtocol.h"

using namespace devdash::adapters;

TEST_CASE("HaltechProtocol decodes RPM correctly", "[haltech][protocol]") {
    HaltechProtocol protocol("protocols/haltech-can-protocol-v2.35.json");

    SECTION("normal RPM value") {
        QByteArray payload = QByteArray::fromHex("0DAC03E8000003E8");
        auto channels = protocol.decode("0x360", payload);

        REQUIRE(channels.contains("RPM"));
        REQUIRE(channels["RPM"] == 3500.0);
    }

    SECTION("zero RPM") {
        QByteArray payload = QByteArray::fromHex("0000000000000000");
        auto channels = protocol.decode("0x360", payload);

        REQUIRE(channels["RPM"] == 0.0);
    }

    SECTION("invalid frame ID") {
        QByteArray payload = QByteArray::fromHex("0DAC03E8000003E8");
        auto channels = protocol.decode("0xFFF", payload);  // Unknown frame

        REQUIRE(channels.empty());
    }
}
```

### Using Sections

Sections allow you to share setup code while testing different scenarios:

```cpp
TEST_CASE("DataBroker handles adapter updates", "[databroker]") {
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

        adapter.emitChannelUpdate("rpm", ChannelValue{-100.0, "RPM", true});

        REQUIRE(spy.count() == 0);  // No signal emitted
        REQUIRE(broker.rpm() == 0.0);  // Value unchanged
    }
}
```

### Tags

Use tags to categorize tests:

```cpp
// Run with: ctest -R "[haltech]"
TEST_CASE("...", "[haltech]") { }

// Run with: ctest -R "[slow]"
TEST_CASE("...", "[slow]") { }

// Multiple tags
TEST_CASE("...", "[haltech][protocol][integration]") { }
```

Common tags:
- `[haltech]`, `[obd2]`, `[simulator]` - Adapter-specific
- `[protocol]` - Protocol parsing tests
- `[integration]` - Integration tests
- `[slow]` - Long-running tests
- `[hardware]` - Requires real hardware (skip in CI)

### Assertions

Catch2 provides expressive assertions:

```cpp
// Basic assertions
REQUIRE(value == 3500.0);
REQUIRE(adapter.isRunning());
REQUIRE_FALSE(channels.empty());

// Floating point comparison
REQUIRE(value == Catch::Approx(3500.0).margin(0.1));

// String matching
REQUIRE_THAT(error, Catch::Matchers::ContainsSubstring("failed"));

// Exception checking
REQUIRE_THROWS_AS(protocol.decode("invalid", data), std::invalid_argument);
REQUIRE_NOTHROW(adapter.start());
```

## Testing Qt Signals

Use `QSignalSpy` to test signal emissions:

```cpp
#include <QSignalSpy>

TEST_CASE("Adapter emits connectionStateChanged", "[adapter]") {
    HaltechAdapter adapter(config);
    QSignalSpy spy(&adapter, &IProtocolAdapter::connectionStateChanged);

    SECTION("emits true on successful start") {
        adapter.start();

        REQUIRE(spy.count() == 1);
        auto arguments = spy.takeFirst();
        REQUIRE(arguments.at(0).toBool() == true);
    }

    SECTION("emits false on stop") {
        adapter.start();
        spy.clear();

        adapter.stop();

        REQUIRE(spy.count() == 1);
        auto arguments = spy.takeFirst();
        REQUIRE(arguments.at(0).toBool() == false);
    }
}
```

## Mock Objects

### Mock Adapters

Create mock adapters for testing higher-level components:

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

    // Test helper: inject data
    void emitTestData(const QString& channel, const ChannelValue& value) {
        emit channelUpdated(channel, value);
    }

private:
    bool m_running = false;
};
```

Use in tests:

```cpp
TEST_CASE("DataBroker processes mock adapter data", "[databroker]") {
    DataBroker broker;
    MockAdapter mock;
    broker.setAdapter(&mock);

    mock.emitTestData("rpm", ChannelValue{5000.0, "RPM", true});

    REQUIRE(broker.rpm() == 5000.0);
}
```

### Test Data Generators

Use Catch2 generators for data-driven tests:

```cpp
TEST_CASE("Protocol handles various RPM values", "[protocol]") {
    HaltechProtocol protocol("protocols/haltech-can-protocol-v2.35.json");

    auto rpm = GENERATE(0, 1000, 3500, 7000, 10000);

    INFO("Testing RPM: " << rpm);

    // Encode RPM value
    quint16 rpmRaw = static_cast<quint16>(rpm);
    QByteArray payload;
    payload.append(static_cast<char>(rpmRaw >> 8));
    payload.append(static_cast<char>(rpmRaw & 0xFF));
    payload.append(QByteArray(6, 0));  // Padding

    auto channels = protocol.decode("0x360", payload);

    REQUIRE(channels["RPM"] == rpm);
}
```

## Integration Tests

Integration tests verify the whole system works together:

```cpp
TEST_CASE("End-to-end data flow", "[integration]") {
    // Create real components
    DataBroker broker;
    SimulatorAdapter simulator(config);
    broker.setAdapter(&simulator);

    // Start adapter
    REQUIRE(simulator.start());

    // Wait for data (Qt event loop)
    QSignalSpy spy(&broker, &DataBroker::rpmChanged);
    REQUIRE(spy.wait(1000));  // Wait up to 1 second

    // Verify data arrived
    REQUIRE(broker.rpm() > 0.0);

    simulator.stop();
}
```

## Testing with Virtual CAN

For tests requiring CAN bus interaction:

```cpp
TEST_CASE("HaltechAdapter reads from vcan0", "[haltech][hardware]") {
    QJsonObject config;
    config["interface"] = "vcan0";

    HaltechAdapter adapter(config);
    REQUIRE(adapter.start());

    // Send test frame via cansend
    system("cansend vcan0 360#0DAC03E8000003E8");

    // Wait for data
    QSignalSpy spy(&adapter, &IProtocolAdapter::channelUpdated);
    REQUIRE(spy.wait(500));

    // Verify frame was decoded
    auto rpm = adapter.getChannel("rpm");
    REQUIRE(rpm.has_value());
    REQUIRE(rpm->value == 3500.0);

    adapter.stop();
}
```

## Debugging Tests

### Running Under GDB

```bash
# Build with debug symbols
cmake --preset dev
cmake --build build/dev

# Run test under gdb
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
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Test",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/dev/test_haltech_protocol",
      "args": ["specific test case"],
      "cwd": "${workspaceFolder}",
      "MIMode": "gdb"
    }
  ]
}
```

### Verbose Output

```bash
# Show all test output (even passing tests)
ctest --test-dir build/dev -V

# Show only failures
ctest --test-dir build/dev --output-on-failure

# Repeat failed tests
ctest --test-dir build/dev --rerun-failed
```

## Continuous Integration

The CI pipeline runs tests on every pull request:

```yaml
# .github/workflows/ci.yml (excerpt)
- name: Configure
  run: cmake --preset ci

- name: Build
  run: cmake --build build/ci

- name: Test
  run: ctest --preset ci
```

The `ci` preset enables:
- Address Sanitizer (catches memory bugs)
- Undefined Behavior Sanitizer (catches UB)
- clang-tidy (static analysis)
- Warnings as errors

**Before pushing**, run locally with the CI preset:

```bash
cmake --preset ci
cmake --build build/ci
ctest --preset ci
```

## Test Coverage

(Optional) Generate coverage reports:

```bash
# Configure with coverage
cmake --preset dev -DCMAKE_CXX_FLAGS="--coverage"

# Build and run tests
cmake --build build/dev
ctest --test-dir build/dev

# Generate report
gcovr --root . --exclude tests/ --html --html-details -o coverage.html
```

## Best Practices

### ✅ Do

- **Test edge cases**: Zero, max, negative, invalid input
- **Use descriptive names**: `"decodes RPM from frame 0x360"` not `"test1"`
- **Test one thing per section**: Each `SECTION` should verify one behavior
- **Mock external dependencies**: Use `MockAdapter` instead of real CAN bus
- **Check return values**: Test both success and failure paths
- **Validate preconditions**: `REQUIRE(adapter.start())` before testing behavior

### ❌ Don't

- **Don't test implementation details**: Test public API, not private methods
- **Don't depend on test order**: Tests should be independent
- **Don't use sleep()**: Use `QSignalSpy::wait()` or event loop
- **Don't hardcode paths**: Use relative paths from project root
- **Don't skip cleanup**: Ensure `stop()` is called even if test fails

## Common Patterns

### Testing Data Validation

```cpp
TEST_CASE("Adapter validates input data", "[adapter]") {
    SimulatorAdapter adapter;

    SECTION("rejects negative RPM") {
        // Trigger internal validation
        adapter.processDataForTesting(-100.0);

        auto rpm = adapter.getChannel("rpm");
        REQUIRE_FALSE(rpm.has_value());  // Invalid data rejected
    }

    SECTION("accepts valid RPM range") {
        adapter.processDataForTesting(3500.0);

        auto rpm = adapter.getChannel("rpm");
        REQUIRE(rpm.has_value());
        REQUIRE(rpm->value == 3500.0);
    }
}
```

### Testing Error Handling

```cpp
TEST_CASE("Adapter handles connection failures", "[adapter]") {
    QJsonObject config;
    config["interface"] = "nonexistent_interface";

    HaltechAdapter adapter(config);

    SECTION("start() returns false on failure") {
        REQUIRE_FALSE(adapter.start());
    }

    SECTION("emits errorOccurred signal") {
        QSignalSpy spy(&adapter, &IProtocolAdapter::errorOccurred);

        adapter.start();

        REQUIRE(spy.count() == 1);
        QString errorMsg = spy.takeFirst().at(0).toString();
        REQUIRE_THAT(errorMsg, Catch::Matchers::ContainsSubstring("Failed"));
    }
}
```

## Troubleshooting

### Tests hang indefinitely

**Cause**: Waiting on a signal that never arrives.

**Solution**: Use timeout with `QSignalSpy::wait()`:

```cpp
REQUIRE(spy.wait(1000));  // Wait max 1 second
```

### "Unknown test" error

**Cause**: Test binary not rebuilt after changes.

**Solution**: Rebuild tests:

```bash
cmake --build build/dev
```

### Sanitizer reports false positives

**Cause**: Qt or system libraries may trigger sanitizer warnings.

**Solution**: Suppress known false positives:

```bash
export LSAN_OPTIONS=suppressions=lsan.supp
```

## Next Steps

- **Write your first test**: [Testing Guidelines](../03-contributing/testing-guidelines.md)
- **Understand test patterns**: [Adding Protocol Adapters](../03-contributing/adding-adapters.md)
- **Explore test examples**: See `tests/adapters/haltech/` for reference
