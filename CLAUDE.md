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

# Run with realistic mock data (haltech-mock)
./scripts/setup-haltech-mock               # First-time setup (checks vcan0)
./scripts/run-with-mock idle               # Idle scenario
./scripts/run-with-mock track --loop       # Track laps
./scripts/run-with-mock overheat           # Test warnings
./scripts/run-with-pd16 lights             # PD16 power distribution
./scripts/list-scenarios                   # Show all scenarios

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

## Helper Scripts (like npm scripts)

For convenience, common tasks are available as shell scripts in `scripts/`:

```bash
# Build the project
./scripts/build [preset]          # Default: dev

# Run tests
./scripts/test [preset]            # Default: dev

# Lint code
./scripts/lint                     # Run clang-tidy

# Format code
./scripts/format                   # Fix formatting
./scripts/format check             # Check formatting only

# Test CI locally (requires Docker)
sudo ./scripts/test-ci-local       # List available jobs
sudo ./scripts/test-ci-local build # Test build job locally
sudo ./scripts/test-ci-local lint  # Test lint job locally
```

**Tip:** Testing CI jobs locally with `act` gives you a tight feedback loop - catch issues before pushing to GitHub!

## QML Development & Tooling

### QML Linting (qmllint)

**Always run qmllint before committing QML changes.** This catches syntax errors, type issues, and circular dependencies at development time.

```bash
# Lint all cluster QML files
cmake --build build/dev --target qmllint-cluster

# Or use the helper script
./scripts/check-qml
```

**Common qmllint warnings to fix:**
- `Tachometer is not a type` - Module import or registration issue
- `Cannot read property 'X' of null` - Accessing properties on non-existent parent
- `unknown grouped property scope` - Invalid property group usage

### QML Language Server (qmlls)

**IDE integration provides real-time QML error detection** with squiggly lines, just like clangd for C++.

**Configuration** (already set up in `.vscode/settings.json` and `.qmlls.ini`):
- Language server: `/usr/lib/qt6/bin/qmllint`
- Import paths include build directory for generated QML modules
- Warnings enabled for unqualified access, missing types, circular dependencies

### QML Component Loading Tests

**All QML windows and components have automated loading tests** to catch errors before runtime.

```cpp
// tests/cluster/test_qml_loading.cpp
TEST_CASE("ClusterMain.qml loads without errors", "[qml][cluster]") {
    DataBroker broker;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("dataBroker", &broker);
    engine.load(QUrl("qrc:/DevDash/Cluster/qml/ClusterMain.qml"));

    REQUIRE(!engine.rootObjects().isEmpty());
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    REQUIRE(window != nullptr);
}
```

**Why these tests matter:**
- QML errors often only show at runtime in specific code paths
- Tests catch compilation errors, missing imports, and type resolution failures
- Runs in CI with offscreen rendering (`QT_QPA_PLATFORM=offscreen`)

### QML Module Patterns

**Avoid circular dependencies in QML modules:**

```qml
// ❌ WRONG - ClusterMain.qml importing its own module
import DevDash.Cluster  // ClusterMain is PART OF DevDash.Cluster!

Tachometer {  // Type not available during module initialization
    value: rpm
}

// ✅ CORRECT - Load sibling components via Loader
Loader {
    source: "gauges/Tachometer.qml"  // Relative path within module
    onLoaded: {
        item.value = Qt.binding(function() { return rpm })
    }
}
```

**Why this matters:**
- Files within a QML module can't reliably use types from their own module during initialization
- Use `Loader` with relative paths for sibling components
- Module types are for external consumers, not internal use

### QML Debugging Tips

**QML errors don't always show in logs.** Use these tools to diagnose issues:

1. **Run QML tests** - Fastest way to see QML compilation errors
   ```bash
   build/dev/tests/devdash_tests "[qml]"
   ```

2. **Check qmlls output** - IDE diagnostics show errors in real-time

3. **Run qmllint manually** - See all warnings for a specific file
   ```bash
   /usr/lib/qt6/bin/qmllint src/cluster/qml/gauges/Tachometer.qml
   ```

4. **Enable QML debugging** - Set QML_IMPORT_TRACE=1 for import resolution issues
   ```bash
   QML_IMPORT_TRACE=1 ./build/dev/devdash --profile profiles/haltech-vcan.json
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

**All APIs must have Doxygen documentation, public APIs should include examples. This is mandatory, not optional.**

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
 * data comes from real hardware or haltech-mock simulation.
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

### Switch Statements (Banned - Use Lookup Tables)

**Switch statements are not allowed.** Use lookup tables instead. This is idiomatic modern C++ and follows the Open/Closed Principle.

```cpp
// ❌ BANNED - switch statements violate OCP
switch (frameId) {
    case 0x360: return decodeRpmTps(payload);
    case 0x362: return decodeTemps(payload);
    // Must modify for each new frame type!
}

// ❌ BANNED - if/else chains on type are equivalent to switch
if (ioType == IOType::Output25A) { ... }
else if (ioType == IOType::Output8A) { ... }

// ✅ REQUIRED - Lookup table pattern
using FrameDecoder = std::function<std::vector<ChannelValue>(const QByteArray&)>;
QHash<uint32_t, FrameDecoder> m_decoders;

// Built dynamically from JSON at load time
void buildDecoderTable() {
    for (const auto& [frameId, frameDef] : m_frameDefinitions) {
        m_decoders[frameId] = createDecoderFor(frameDef);
    }
}

// Single O(1) lookup at decode time
auto it = m_decoders.find(frameId);
if (it != m_decoders.end()) {
    return it.value()(payload);
}
```

**Why lookup tables:**
- Open/Closed: Add new types via data, not code changes
- Testable: Each handler is a separate unit-testable function
- Readable: Table initialization shows all mappings in one place
- Standard: Used throughout Qt, STL, game engines, protocol parsers

**For enum-to-value mappings, use `std::array`:**

```cpp
// ❌ BANNED
QString getIOTypeName(IOType type) {
    switch (type) {
        case IOType::Output25A: return "25A";
        case IOType::Output8A: return "8A";
        // ...
    }
}

// ✅ REQUIRED - static array indexed by enum
const std::array<QString, 5> IO_TYPE_NAMES = {
    "25A", "8A", "HBO", "SPI", "AVI"
};

QString getIOTypeName(IOType type) {
    auto index = static_cast<size_t>(type);
    return (index < IO_TYPE_NAMES.size()) ? IO_TYPE_NAMES[index] : "Unknown";
}
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

## Type Safety and Configuration Validation

While C++ doesn't have TypeScript's string literal types, we achieve similar safety through **runtime validation with loud failures** and **compile-time tests**:

### Problem: Silent Configuration Mismatches

A profile might reference channel names that don't exist in the protocol:

```json
// Profile says:
"channelMappings": {
    "Engine Coolant Temperature": "coolantTemperature"  // ❌ Doesn't exist!
}

// But protocol has:
"channels": [
    { "name": "Coolant Temperature", ... }  // ✅ Actual name
]
```

In TypeScript, this would be a compile error. In C++, it's a runtime mismatch that causes silent data loss.

### Solution 1: Loud Runtime Failures

**DataBroker logs CRITICAL errors for unmapped channels:**

```cpp
// src/core/broker/DataBroker.cpp:processQueue()
if (!standardChannel.has_value()) {
    // LOUD FAILURE: Unmapped channel indicates configuration error
    qCCritical(logBroker) << "UNMAPPED CHANNEL:" << update.channelName
                          << "- Check profile channelMappings!";
    qCCritical(logBroker) << "Available mappings:" << m_channelMappings.keys();
    qCCritical(logBroker) << "This indicates a mismatch between protocol and profile.";
    return;  // Don't silently drop data
}
```

**Why this works:**
- Failures are LOUD (qCCritical, not qDebug)
- Provides actionable context (expected channel names)
- Fails at first occurrence, not buried in logs
- Shows up immediately during development/testing

### Solution 2: Compile-Time Validation Tests

**Test verifies profile mappings match protocol definitions:**

```cpp
// tests/core/broker/test_data_broker.cpp
TEST_CASE("Profile channel mappings match protocol definition", "[typesafety]") {
    // Load real protocol
    HaltechProtocol protocol;
    protocol.loadDefinition("protocols/haltech/haltech-can-protocol-v2.35.json");

    // Load real profile
    QFile profileFile("profiles/haltech-vcan.json");
    QJsonDocument profileDoc = QJsonDocument::fromJson(profileFile.readAll());
    QJsonObject mappings = profileDoc.object()["channelMappings"].toObject();

    // Get all available channel names from protocol
    QSet<QString> protocolChannels = protocol.availableChannels();

    // Verify every profile channel exists in protocol
    for (auto it = mappings.begin(); it != mappings.end(); ++it) {
        const QString& channelName = it.key();
        REQUIRE(protocolChannels.contains(channelName));  // ❌ FAILS if mismatch
    }
}
```

**This test caught real bugs:**
- `"Engine Coolant Temperature"` → should be `"Coolant Temperature"`
- `"Gear Position"` → should be `"Current Gear"`
- `"Intake Air Temperature"` → should be `"Air Temperature"`

**Why this works:**
- Runs on every build/PR
- Uses REAL protocol and profile files (not mocks)
- Fails at compile/test time, not production
- Prevents "works on my machine" config bugs

### Type Safety Best Practices

1. **Use strong types at C++ boundaries:**
   ```cpp
   // ❌ Primitive obsession
   void updateChannel(const QString& name, double value);

   // ✅ Strong typing
   void updateChannel(const ChannelName& name, const ChannelValue& value);
   ```

2. **Add tests for configuration consistency:**
   - Profile channels exist in protocol
   - Gear mappings reference valid gear numbers
   - Warning thresholds are in valid ranges

3. **Make invalid states unrepresentable:**
   ```cpp
   // ❌ Can be invalid
   struct ChannelValue {
       double value;
       bool valid;  // Redundant flag
   };

   // ✅ Invalid state is std::nullopt
   std::optional<double> getChannelValue(const QString& name);
   ```

4. **Validate external input at boundaries:**
   ```cpp
   // CAN data - never trust it
   if (rpm > MAX_VALID_RPM) {
       qCWarning() << "Invalid RPM:" << rpm << "- ignoring";
       return;
   }

   // JSON config - fail fast on load
   if (!profile.contains("channelMappings")) {
       qCCritical() << "Profile missing required 'channelMappings' section";
       return false;  // Don't continue with invalid config
   }
   ```

5. **Use logging categories for debuggability:**
   ```cpp
   // Not this:
   qWarning() << "Channel" << name << "not found";

   // This - filterable and searchable:
   qCWarning(logBroker) << "Unmapped channel:" << name;
   ```

### When to Use Each Approach

| Scenario | Approach | Why |
|----------|----------|-----|
| **Protocol/profile mismatch** | Compile-time test | Catches config errors before deploy |
| **Invalid CAN data** | Runtime validation + loud logs | Can't predict all invalid states |
| **User input** | Strong types + validation | Prevent injection/overflow |
| **Internal invariants** | `Q_ASSERT` + defensive code | Debug in dev, safe in release |

### Type Safety vs. Configuration Flexibility

We balance type safety with runtime flexibility:

- **Type-safe**: C++ code uses enums, strong types, `std::optional`
- **Flexible**: JSON configs allow new channels without recompiling
- **Validated**: Tests ensure configs match expected schemas

This is a hybrid approach - stronger than pure runtime validation, more flexible than pure compile-time types.

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

### IDE Diagnostics (Language Server)

**Claude Code must check IDE diagnostics before considering code complete.**

Use the `mcp__ide__getDiagnostics` tool to verify that the language server (clangd) reports zero warnings/errors:

```typescript
// Check all diagnostics from clangd
mcp__ide__getDiagnostics()
```

This catches issues in real-time that may not appear in build output:
- Implicit type conversions
- Unused variables
- Incorrect function signatures
- Type mismatches
- And all clang-tidy warnings

**When to check diagnostics:**
- After writing or modifying any C++ code
- Before marking a task as complete
- When user reports seeing errors in their IDE

### Pre-commit Checklist

Before committing, ensure:

1. `cmake --build build/dev` - Compiles without warnings
2. `cmake --build build/dev --target clang-tidy` - Zero lint issues
3. `cmake --build build/dev --target format-check` - Code is formatted
4. `ctest --test-dir build/dev` - All tests pass
5. `mcp__ide__getDiagnostics()` - Zero IDE diagnostics (warnings/errors from clangd)

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