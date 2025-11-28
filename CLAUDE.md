# /dev/dash - CLAUDE.md

## Project Overview

A modular automotive dashboard framework built with Qt 6/QML and C++20. Targets NVIDIA Jetson but runs anywhere with Qt support.

## Build & Run

```bash
# Configure (from project root)
cmake --preset dev

# Build
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# Run with mock data
./scripts/setup-vcan.sh
haltech-mock --interface vcan0 --scenario idle &
./build/devdash --profile profiles/example-simulator.json

# Lint (clang-tidy)
cmake --build build --target clang-tidy

# Format (check)
cmake --build build --target format-check

# Format (fix)
cmake --build build --target format-fix

# Generate documentation
cmake --build build --target docs
# Output: build/docs/html/index.html
```

## Architecture

**Single app, dual display.** One Qt application manages both instrument cluster and head unit windows.

```
IProtocolAdapter (interface)
    └── HaltechAdapter, Obd2Adapter, SimulatorAdapter
DataBroker (central data hub, exposes Q_PROPERTY for QML)
    └── Receives data from adapters, handles unit conversion
ClusterWindow / HeadUnitWindow (QML-based UIs)
```

Vehicle-specific config lives in JSON profiles (`profiles/*.json`), not code.

## Code Style

### C++ Standards

- **C++20** minimum (`-std=c++20`)
- Use `std::format`, `std::span`, concepts, ranges where appropriate
- Prefer `std::optional`, `std::expected` over exceptions for recoverable errors
- Use `[[nodiscard]]` on functions returning values that shouldn't be ignored

### SOLID Principles

- **S**ingle Responsibility: One class = one job. `HaltechAdapter` parses CAN, nothing else.
- **O**pen/Closed: New protocols via new adapters, not modifying existing code.
- **L**iskov Substitution: Any `IProtocolAdapter*` works identically.
- **I**nterface Segregation: Small, focused interfaces. Don't force adapters to implement unused methods.
- **D**ependency Inversion: Depend on `IProtocolAdapter`, not concrete `HaltechAdapter`.

### Defensive Programming

```cpp
// Validate CAN data - never trust external input
auto rpm = decodeUint16(frame.payload(), 0);
if (rpm > MAX_VALID_RPM) {
    qWarning() << "Invalid RPM value:" << rpm;
    return;  // Don't propagate garbage
}

// Use std::optional for fallible operations
std::optional<ChannelValue> getChannel(const QString& name) const;

// Check preconditions explicitly
void setThreshold(int value) {
    Q_ASSERT(value >= 0 && value <= 100);  // Debug builds
    m_threshold = std::clamp(value, 0, 100);  // Release safety
}
```

### Qt/QML Conventions

- All QML-exposed properties use `Q_PROPERTY` with `NOTIFY` signal
- Property naming: `camelCase` (e.g., `coolantTemperature`, not `coolant_temperature`)
- Signals: past tense for events (`dataUpdated`), present for state (`isConnected`)
- Use `Q_ENUM`/`Q_FLAG` for enums exposed to QML
- QML components: `PascalCase.qml` (e.g., `Tachometer.qml`)

### Naming

```cpp
// Classes: PascalCase
class DataBroker;
class HaltechAdapter;

// Functions/methods: camelCase
void processFrame(const QCanBusFrame& frame);
double convertToFahrenheit(double celsius);

// Member variables: m_ prefix
QString m_currentGear;
double m_oilPressure;

// Constants: SCREAMING_SNAKE_CASE or k prefix
constexpr int MAX_RPM = 8000;
constexpr double kDefaultPressure = 101.3;

// Namespaces: lowercase
namespace devdash::protocol { }
```

### Documentation (Doxygen)

All public APIs must have Doxygen documentation. Use `@` prefix style:

```cpp
/**
 * @brief Decode a CAN frame into channel values.
 *
 * Parses raw CAN payload according to the Haltech V2.35 protocol
 * and returns decoded values with unit conversions applied.
 *
 * @param frameId Hex frame ID (e.g., "0x360")
 * @param payload Raw 8-byte CAN payload
 * @return Map of channel names to decoded values
 * @throws std::invalid_argument If frameId is not recognized
 *
 * @note Temperatures are converted from Kelvin to Celsius.
 * @see HaltechProtocol::encodeFrame()
 *
 * @example
 * @code
 * QByteArray data = QByteArray::fromHex("0DAC03E8...");
 * auto values = protocol.decodeFrame("0x360", data);
 * double rpm = values["RPM"];  // 3500.0
 * @endcode
 */
std::map<QString, double> decodeFrame(const QString& frameId, const QByteArray& payload);
```

**Required tags:**
- `@brief` - One-line summary (required for all public functions/classes)
- `@param` - Document each parameter
- `@return` - Document return value (if non-void)
- `@throws` - Document exceptions that may be thrown

**Optional tags:**
- `@note` - Important usage notes
- `@warning` - Gotchas or dangerous behavior
- `@see` - Cross-references to related functions
- `@example` / `@code` - Usage examples
- `@pre` / `@post` - Preconditions and postconditions

**Class documentation:**

```cpp
/**
 * @brief Central data hub for vehicle telemetry.
 *
 * DataBroker aggregates data from protocol adapters, handles unit
 * conversions based on user preferences, and exposes values to QML
 * via Q_PROPERTY bindings.
 *
 * @note This class is thread-safe. Data updates from adapter threads
 *       are marshalled to the main thread via queued connections.
 *
 * @see IProtocolAdapter
 */
class DataBroker : public QObject {
    Q_OBJECT
    
    /**
     * @brief Current engine RPM.
     * @note Updates at 50Hz from Haltech adapter.
     */
    Q_PROPERTY(double rpm READ rpm NOTIFY rpmChanged)
    // ...
};
```

**Don't document:**
- Private implementation details (unless complex)
- Obvious getters/setters (the `@brief` is enough)
- Self-explanatory one-liners

### File Organization

- Headers: `.h` extension
- Sources: `.cpp` extension
- One class per file (exceptions: small related classes)
- Include guards: `#pragma once`

### Error Handling

- Use `qWarning()`, `qCritical()` for logging (not `std::cerr`)
- CAN parse failures: log and skip frame, don't crash
- Config errors: fail fast at startup with clear message
- Never `abort()` or `exit()` from library code

## Testing

We use **Catch2 v3** for unit tests and **Qt Test** only where Qt event loop is required.

```cpp
// tests/test_haltech_protocol.cpp
#include <catch2/catch_test_macros.hpp>
#include "adapters/haltech/HaltechProtocol.h"

TEST_CASE("HaltechProtocol decodes RPM correctly", "[haltech]") {
    QByteArray payload = QByteArray::fromHex("0DAC03E8...");
    
    SECTION("normal RPM value") {
        auto rpm = HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == 3500);
    }
    
    SECTION("zero RPM") {
        payload[0] = 0x00;
        payload[1] = 0x00;
        auto rpm = HaltechProtocol::decodeRpm(payload);
        REQUIRE(rpm == 0);
    }
}
```

### Test Guidelines

- Test file mirrors source: `src/adapters/haltech/HaltechProtocol.cpp` → `tests/adapters/haltech/test_haltech_protocol.cpp`
- Use descriptive test names: `"decodes temperature in Kelvin and converts to Celsius"`
- Test edge cases: zero values, max values, invalid input
- Mock CAN interface for adapter tests (use `SimulatorAdapter` or vcan)

## Linting & Static Analysis

### clang-tidy

Configured in `.clang-tidy`. Key checks enabled:

- `cppcoreguidelines-*` (SOLID, modern C++)
- `performance-*` (avoid copies, unnecessary allocations)
- `bugprone-*` (common mistakes)
- `modernize-*` (use modern C++ features)
- `readability-*` (clear code)

Suppressions go in code with comments:

```cpp
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
constexpr double ATMOSPHERIC_PRESSURE_KPA = 101.325;
```

### clang-format

Configured in `.clang-format`. Based on LLVM style with modifications for Qt.

Format before committing. CI will reject unformatted code.

## CMake

### Adding New Source Files

```cmake
# In src/adapters/CMakeLists.txt
target_sources(devdash_adapters PRIVATE
    haltech/HaltechAdapter.cpp
    haltech/HaltechAdapter.h
    haltech/HaltechProtocol.cpp
    haltech/HaltechProtocol.h
    # Add new files here
)
```

### Adding New Tests

```cmake
# In tests/CMakeLists.txt
add_executable(test_haltech_protocol
    adapters/haltech/test_haltech_protocol.cpp
)
target_link_libraries(test_haltech_protocol PRIVATE
    devdash_adapters
    Catch2::Catch2WithMain
)
catch_discover_tests(test_haltech_protocol)
```

### Build Presets

- `dev`: Debug build, sanitizers enabled, all warnings
- `release`: Optimized, LTO enabled
- `ci`: For CI pipeline, warnings as errors

## Dependencies

Managed via CMake `FetchContent`:

- **Qt 6.5+**: Core, Quick, Multimedia, SerialBus
- **Catch2 v3**: Testing
- **nlohmann/json**: Config parsing

System dependencies (install separately):

- GStreamer 1.x with plugins
- SocketCAN (Linux)

## Common Tasks

### Add a New Protocol Adapter

1. Create `src/adapters/newprotocol/` directory
2. Implement `IProtocolAdapter` interface
3. Register in `ProtocolAdapterFactory`
4. Add protocol definition JSON in `protocols/`
5. Add tests in `tests/adapters/newprotocol/`

### Add a New Gauge Component

1. Create `src/cluster/qml/gauges/NewGauge.qml`
2. Bind to `DataBroker` properties
3. Add to gauge registry in `ClusterWindow`
4. Make selectable in vehicle profile

### Debug CAN Issues

```bash
# Monitor raw CAN traffic
candump vcan0

# Send test frame
cansend vcan0 360#0DAC03E8000003E8

# Check adapter connection
./build/devdash --profile profiles/debug.json --log-level debug
```

## Don't

- Don't use raw pointers for ownership (use `std::unique_ptr`, `QObject` parent)
- Don't block the main thread (CAN parsing, file I/O go in worker threads)
- Don't hardcode vehicle-specific values (use profiles)
- Don't use `QString::arg()` for many arguments (use `std::format` or `QStringLiteral`)
- Don't suppress warnings without comment explaining why
