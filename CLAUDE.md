# /dev/dash - CLAUDE.md

## Project Overview

A modular automotive dashboard framework built with Qt 6/QML and C++20. Targets NVIDIA Jetson but runs anywhere with Qt support.

## Build & Run

```bash
# Configure (from project root)
cmake --preset dev

# Build
cmake --build build/dev

# Run tests
ctest --test-dir build/dev --output-on-failure

# Run with mock data
.devcontainer/setup-vcan.sh
haltech-mock --interface vcan0 --scenario idle &
./build/dev/devdash --profile profiles/example-simulator.json

# Lint (clang-tidy)
cmake --build build/dev --target clang-tidy

# Format (check)
cmake --build build/dev --target format-check

# Format (fix)
cmake --build build/dev --target format-fix

# Generate documentation
cmake --build build/dev --target docs
# Output: build/dev/docs/html/index.html
```

## Architecture

**Single app, dual display.** One Qt application manages both instrument cluster and head unit windows.

```
IProtocolAdapter (interface)
    └── HaltechAdapter, Obd2Adapter, SimulatorAdapter
DataBroker (central data hub, exposes Q_PROPERTY for QML)
    └── Receives data from adapters, handles unit conversion
    └── Channel mappings are DATA-DRIVEN from JSON profiles
ClusterWindow / HeadUnitWindow (QML-based UIs)
```

Vehicle-specific config lives in JSON profiles (`profiles/*.json`), not code.

## Critical Code Generation Rules

**These rules are mandatory. Violations will be rejected in code review.**

### 1. Every Public Function MUST Have Doxygen Documentation

No exceptions. If it's public, it gets documented:

```cpp
// ❌ WRONG - Missing documentation
void processFrame(const QCanBusFrame& frame);

// ✅ CORRECT - Full Doxygen documentation
/**
 * @brief Process an incoming CAN frame and emit decoded channel values.
 *
 * Decodes the frame according to the loaded protocol definition and
 * emits channelUpdated signals for each decoded value.
 *
 * @param frame Valid CAN frame received from the bus
 * @pre frame.isValid() must be true
 * @note Invalid frames are logged and skipped, not propagated
 */
void processFrame(const QCanBusFrame& frame);
```

### 2. Never Use String-Based Conditionals for Dispatching

Use maps, enums, registries, or polymorphism instead:

```cpp
// ❌ WRONG - String-based dispatch is unmaintainable
void onChannelUpdated(const QString& name, double value) {
    if (name == "rpm" || name == "RPM") {
        m_rpm = value;
    } else if (name == "throttlePosition" || name == "TPS") {
        m_throttle = value;
    }
    // ... 50 more else-if blocks
}

// ✅ CORRECT - Data-driven dispatch via registry
void onChannelUpdated(const QString& name, double value) {
    if (auto it = m_channelHandlers.find(name); it != m_channelHandlers.end()) {
        it->second(value);
    }
}
```

### 3. Configuration Comes from JSON Profiles, Not Code

Protocol-specific values (channel names, frame IDs, scaling factors) belong in JSON:

```cpp
// ❌ WRONG - Hardcoded protocol knowledge in DataBroker
if (channelName == "ECT") {  // Haltech-specific abbreviation
    m_coolantTemperature = value;
}

// ✅ CORRECT - Profile defines the mapping
// profiles/haltech-nexus.json:
// { "channelMappings": { "ECT": "coolantTemperature" } }
```

### 4. Use Strong Types Over Primitives

```cpp
// ❌ WRONG - Primitive obsession
void setTemperature(double value, int unit);

// ✅ CORRECT - Strong types
enum class TemperatureUnit { Celsius, Fahrenheit, Kelvin };
void setTemperature(Temperature value);  // Temperature class handles units
```

### 5. Validate All External Input

CAN data, JSON configs, user input - never trust it:

```cpp
// ❌ WRONG - Trusting external data
auto rpm = decodeUint16(payload, 0);
m_rpm = rpm;  // Could be garbage!

// ✅ CORRECT - Validate before use
auto rpm = decodeUint16(payload, 0);
if (rpm > MAX_VALID_RPM) {
    qWarning() << "Invalid RPM value:" << rpm << "- ignoring frame";
    return;
}
m_rpm = rpm;
```

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

**All public APIs must have Doxygen documentation. This is mandatory, not optional.**

Use `@` prefix style:

```cpp
/**
 * @brief Decode a CAN frame into channel values.
 *
 * Parses raw CAN payload according to the Haltech V2.35 protocol
 * and returns decoded values with unit conversions applied.
 *
 * @param frameId Hex frame ID (e.g., "0x360")
 * @param payload Raw 8-byte CAN payload
 * @return Map of channel names to decoded values, empty if frameId unrecognized
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

**Recommended tags:**
- `@pre` / `@post` - Preconditions and postconditions
- `@throws` - Document exceptions that may be thrown
- `@note` - Important usage notes
- `@warning` - Gotchas or dangerous behavior
- `@see` - Cross-references to related functions
- `@example` / `@code` - Usage examples for complex APIs

**Class documentation:**

```cpp
/**
 * @brief Central data hub for vehicle telemetry.
 *
 * DataBroker aggregates data from protocol adapters, handles unit
 * conversions based on user preferences, and exposes values to QML
 * via Q_PROPERTY bindings.
 *
 * Channel mappings are configured via JSON profiles, not hardcoded.
 * The broker is protocol-agnostic - it doesn't know or care whether
 * data comes from Haltech, OBD2, or a simulator.
 *
 * @note This class is thread-safe. Data updates from adapter threads
 *       are marshalled to the main thread via queued connections.
 *
 * @see IProtocolAdapter
 * @see ChannelRegistry
 */
class DataBroker : public QObject {
    Q_OBJECT
    
    /**
     * @brief Current engine RPM.
     * @note Update frequency depends on the adapter (typically 50Hz for Haltech).
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

## Anti-Patterns to Avoid

These patterns are enforced by clang-tidy and code review. Claude Code must not generate code with these issues.

### Giant If/Else Chains (OCP Violation)

```cpp
// ❌ This is an unmaintainable nightmare - violates Open/Closed
if (name == "rpm") { ... }
else if (name == "RPM") { ... }
else if (name == "engineSpeed") { ... }
else if (name == "throttle") { ... }
// 50 more conditions...

// ✅ Use a lookup table configured from JSON
auto handler = m_handlers.find(name);
if (handler != m_handlers.end()) {
    handler->second(value);
}
```

### Switch on Type (OCP Violation)

```cpp
// ❌ Adding new types requires modifying this code
switch (event.type) {
    case EventType::CanFrame: processCanFrame(event); break;
    case EventType::GpsUpdate: processGps(event); break;
    // Must modify for each new event type!
}

// ✅ Use polymorphism
class IEventHandler {
public:
    virtual void handle(const Event& event) = 0;
};

// Each event type has its own handler - no switch needed
m_handlers[event.type]->handle(event);
```

### Magic Strings

```cpp
// ❌ Protocol-specific strings scattered throughout code
if (channelName == "ECT") { ... }  // What is ECT? Haltech abbreviation!

// ✅ Use constants or enums, map from profile
if (channelId == StandardChannel::CoolantTemperature) { ... }
```

### Concrete Dependencies (DIP Violation)

```cpp
// ❌ Hard-coded dependency - impossible to test or swap
class DataBroker {
    void init() {
        m_adapter = new HaltechAdapter();  // Concrete dependency!
    }
};

// ✅ Inject dependencies through constructor
class DataBroker {
public:
    explicit DataBroker(std::unique_ptr<IProtocolAdapter> adapter)
        : m_adapter(std::move(adapter)) {}
};
```

### Throwing Overrides (LSP Violation)

```cpp
// ❌ Child breaks parent's contract
class IProtocolAdapter {
public:
    virtual void start() = 0;  // Contract: starts the adapter
};

class BrokenAdapter : public IProtocolAdapter {
    void start() override {
        throw std::runtime_error("Not implemented");  // LSP violation!
    }
};

// ✅ Don't inherit if you can't fulfill the contract
// Or provide a proper implementation
```

### Empty Overrides (LSP Violation)

```cpp
// ❌ Empty override doesn't fulfill the contract
class BrokenAdapter : public IProtocolAdapter {
    void stop() override {
        // Does nothing - violates LSP!
    }
};

// ✅ Either implement properly or don't override
void stop() override {
    m_canDevice->disconnectDevice();
    m_running = false;
}
```

### Mixing Concerns (SRP Violation)

```cpp
// ❌ DataBroker knowing Haltech-specific details
if (channelName == "TPS") {  // Haltech calls it TPS
    m_throttlePosition = value;
}

// ✅ Profile maps protocol names → standard property names
// DataBroker only knows standard names
```

### Primitive Obsession

```cpp
// ❌ What unit is this? PSI? kPa? Bar?
double m_oilPressure;

// ✅ Type carries its unit
Pressure m_oilPressure;  // Internally stored in kPa, converts on access
```

### Bloated Interfaces (ISP Violation)

```cpp
// ❌ Interface requires implementing unused methods
class IVehicleDataSource {
    virtual double getRpm() = 0;
    virtual double getSpeed() = 0;
    virtual GpsCoordinate getLocation() = 0;  // Not all sources have GPS!
    virtual CameraFrame getVideo() = 0;       // Not all sources have cameras!
};

// ✅ Segregate interfaces by capability
class IEngineDataSource {
    virtual double getRpm() = 0;
    virtual double getSpeed() = 0;
};

class IGpsSource {
    virtual GpsCoordinate getLocation() = 0;
};
```

### Generic Record Types

```cpp
// ❌ Too generic - loses type safety
std::map<std::string, std::any> config;

// ✅ Use specific types
struct AdapterConfig {
    QString interface;
    QString protocolFile;
    int baudRate;
};
```

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

**All code must pass linting with zero warnings.** CI will reject code with lint issues.

### clang-tidy

Configured in `.clang-tidy`. The linter enforces SOLID principles:

| SOLID Principle | clang-tidy Checks |
|-----------------|-------------------|
| **SRP** (Single Responsibility) | `readability-function-cognitive-complexity`, `readability-function-size` |
| **OCP** (Open/Closed) | Caught by code review - avoid switch-on-type patterns |
| **LSP** (Liskov Substitution) | `cppcoreguidelines-virtual-class-destructor`, `cppcoreguidelines-slicing`, `bugprone-parent-virtual-call` |
| **ISP** (Interface Segregation) | `bugprone-easily-swappable-parameters` (too many params = bloated interface) |
| **DIP** (Dependency Inversion) | Prefer interfaces over concrete types (enforced by code review) |

**Key checks enabled:**

- `cppcoreguidelines-avoid-magic-numbers` - Use named constants
- `cppcoreguidelines-virtual-class-destructor` - Base classes need virtual destructors
- `cppcoreguidelines-slicing` - Catch accidental object slicing
- `bugprone-virtual-near-miss` - Catch typos in virtual function overrides
- `readability-function-cognitive-complexity` - Keep functions simple (max 25)
- `readability-function-size` - Max 100 lines, 50 statements, 6 parameters
- `modernize-*` - Use C++20 features

**Suppressions go in code with comments explaining why:**

```cpp
// Frame ID is protocol-defined constant, documented in protocols/haltech-v2.35.json
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
constexpr uint32_t HALTECH_RPM_FRAME_ID = 0x360;
```

**Never suppress without a comment.** If you can't justify the suppression, fix the code.

### clang-format

Configured in `.clang-format`. Based on LLVM style with modifications for Qt.

```bash
# Check formatting
cmake --build build/dev --target format-check

# Fix formatting
cmake --build build/dev --target format-fix
```

Format before committing. CI will reject unformatted code.

### Pre-commit Checklist

Before committing, ensure:

1. `cmake --build build/dev` - Compiles without warnings
2. `cmake --build build/dev --target clang-tidy` - Zero lint issues
3. `cmake --build build/dev --target format-check` - Code is formatted
4. `ctest --test-dir build/dev` - All tests pass

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
./build/dev/devdash --profile profiles/debug.json --log-level debug
```

## Don't

- Don't use raw pointers for ownership (use `std::unique_ptr`, `QObject` parent)
- Don't block the main thread (CAN parsing, file I/O go in worker threads)
- Don't hardcode vehicle-specific values (use profiles)
- Don't use `QString::arg()` for many arguments (use `std::format` or `QStringLiteral`)
- Don't suppress warnings without comment explaining why
- Don't use string-based dispatch (if/else chains on string values)
- Don't put protocol-specific knowledge in DataBroker
- Don't skip Doxygen documentation on public APIs