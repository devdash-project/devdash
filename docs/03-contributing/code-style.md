# Code Style Guide

This document defines the C++ and QML code style conventions for /dev/dash.

## C++ Standards

### Language Version

- **C++20** minimum (`-std=c++20`)
- Compiler: GCC 11+ or Clang 14+

### Modern C++ Features

**Use these C++20 features:**

```cpp
// std::format instead of QString::arg for complex formatting
std::string message = std::format("RPM: {}, Temp: {:.1f}°C", rpm, temp);

// std::span for safe array views
void processData(std::span<const uint8_t> payload);

// std::optional for fallible operations
std::optional<ChannelValue> getChannel(const QString& name) const;

// Concepts for template constraints
template<std::integral T>
T decodeValue(const QByteArray& data, int offset);

// Ranges for cleaner transformations
auto names = channels | std::views::keys | std::ranges::to<QStringList>();
```

**Avoid:**
- ❌ Raw `new` / `delete` - use smart pointers
- ❌ `NULL` - use `nullptr`
- ❌ C-style casts - use `static_cast`, `reinterpret_cast`
- ❌ Manual memory management - use RAII

### Error Handling

**Prefer `std::optional` and `std::expected` over exceptions** for recoverable errors:

```cpp
// Good: Returns std::optional for fallible operation
std::optional<ChannelValue> getChannel(const QString& name) const {
    auto it = m_channels.find(name);
    if (it != m_channels.end()) {
        return it.value();
    }
    return std::nullopt;
}

// Usage
if (auto value = adapter->getChannel("rpm")) {
    qInfo() << "RPM:" << value->value;
} else {
    qWarning() << "Channel not found";
}
```

**For errors:**
- Use `qWarning()`, `qCritical()` for logging (not `std::cerr`)
- CAN parse failures: log and skip frame, don't crash
- Config errors: fail fast at startup with clear message
- Never `abort()` or `exit()` from library code

### [[nodiscard]] Attribute

Use `[[nodiscard]]` on functions returning values that shouldn't be ignored:

```cpp
// Good: Forces caller to check return value
[[nodiscard]] bool start();
[[nodiscard]] std::optional<ChannelValue> getChannel(const QString& name) const;
[[nodiscard]] bool isRunning() const;

// Compiler warning if ignored:
adapter->start();  // Warning: ignoring return value of 'bool start()'

// Correct:
if (!adapter->start()) {
    qCritical() << "Failed to start adapter";
}
```

## SOLID Principles

The codebase follows **SOLID** object-oriented design:

### Single Responsibility Principle

**One class = one job.**

```cpp
// Good: HaltechAdapter only handles CAN communication
class HaltechAdapter : public IProtocolAdapter {
    // Connects to CAN, reads frames, emits channel updates
};

// Good: HaltechProtocol only handles frame decoding
class HaltechProtocol {
    // Parses CAN payloads, converts units
};

// Bad: Adapter doing too much
class HaltechAdapter {
    void connectToCAN();
    void renderGauge();  // ❌ Not adapter's responsibility!
};
```

### Open/Closed Principle

**Open for extension, closed for modification.**

Add new protocols via new adapters, not by modifying existing code:

```cpp
// Good: Add OBD-II support without touching HaltechAdapter
class OBD2Adapter : public IProtocolAdapter { };

// Register in factory
adapter = ProtocolAdapterFactory::create("obd2", config);

// Bad: Modifying HaltechAdapter to support OBD-II
class HaltechAdapter {
    if (protocolType == "obd2") { }  // ❌ Wrong!
};
```

### Liskov Substitution Principle

**Any `IProtocolAdapter*` works identically.**

```cpp
// Good: DataBroker doesn't care which adapter
void DataBroker::setAdapter(std::unique_ptr<IProtocolAdapter> adapter) {
    m_adapter = std::move(adapter);
    connect(m_adapter.get(), &IProtocolAdapter::channelUpdated, ...);
}

// All adapters work the same way:
setAdapter(std::make_unique<HaltechAdapter>());
setAdapter(std::make_unique<OBD2Adapter>());
setAdapter(std::make_unique<SimulatorAdapter>());
```

### Interface Segregation Principle

**Small, focused interfaces. Don't force adapters to implement unused methods.**

```cpp
// Good: Minimal interface
class IProtocolAdapter {
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual std::optional<ChannelValue> getChannel(...) = 0;
};

// Bad: Bloated interface
class IProtocolAdapter {
    virtual void renderGauge() = 0;  // ❌ Not all adapters render
    virtual void playSound() = 0;    // ❌ Unrelated
};
```

### Dependency Inversion Principle

**Depend on abstractions (interfaces), not concrete implementations.**

```cpp
// Good: DataBroker depends on IProtocolAdapter interface
class DataBroker {
    std::unique_ptr<IProtocolAdapter> m_adapter;  // Interface, not concrete
};

// Bad: DataBroker depends on concrete class
class DataBroker {
    std::unique_ptr<HaltechAdapter> m_adapter;  // ❌ Tightly coupled
};
```

## Defensive Programming

**Never trust external input.** Validate all data from CAN bus, serial, network, files.

```cpp
// Good: Validate CAN data
auto rpm = decodeUint16(frame.payload(), 0);
if (rpm > MAX_VALID_RPM) {
    qWarning() << "Invalid RPM value:" << rpm;
    return;  // Don't propagate garbage
}

// Good: Check preconditions
void setThreshold(int value) {
    Q_ASSERT(value >= 0 && value <= 100);  // Debug builds catch violations
    m_threshold = std::clamp(value, 0, 100);  // Release builds clamp safely
}

// Good: Use std::optional for fallible operations
std::optional<ChannelValue> getChannel(const QString& name) const;
```

## Qt/QML Conventions

### Q_PROPERTY

All QML-exposed properties must use `Q_PROPERTY` with `NOTIFY` signal:

```cpp
class DataBroker : public QObject {
    Q_OBJECT

    // Good: Complete property declaration
    Q_PROPERTY(double rpm READ rpm NOTIFY rpmChanged)
    Q_PROPERTY(double coolantTemperature READ coolantTemperature NOTIFY coolantTemperatureChanged)

public:
    [[nodiscard]] double rpm() const { return m_rpm; }
    [[nodiscard]] double coolantTemperature() const { return m_coolantTemperature; }

signals:
    void rpmChanged();
    void coolantTemperatureChanged();

private:
    double m_rpm{0.0};
    double m_coolantTemperature{0.0};
};
```

### Property Naming

- Use `camelCase` for properties (not `snake_case`)
- Match Qt conventions

```cpp
// Good
Q_PROPERTY(double coolantTemperature ...)
Q_PROPERTY(int gear ...)
Q_PROPERTY(bool isConnected ...)

// Bad
Q_PROPERTY(double coolant_temperature ...)  // ❌ snake_case
Q_PROPERTY(double CoolantTemperature ...)  // ❌ PascalCase
```

### Signal Naming

- **Past tense** for events: `dataUpdated`, `frameReceived`
- **Present tense** for state: `isConnected`, `isRunning`

```cpp
signals:
    void rpmChanged();          // Good: state changed
    void channelUpdated(...);   // Good: event occurred
    void errorOccurred(...);    // Good: event occurred
```

### Enums for QML

Use `Q_ENUM` to expose enums to QML:

```cpp
class AlertLevel : public QObject {
    Q_OBJECT
public:
    enum Level {
        None,
        Warning,
        Critical
    };
    Q_ENUM(Level)
};

// In QML:
if (alert.level === AlertLevel.Critical) { }
```

### QML Component Naming

- QML files: `PascalCase.qml`
- One component per file

```
Tachometer.qml           ✅
Speedometer.qml          ✅
DigitalGauge.qml         ✅
tachometer.qml           ❌
TachometerGauge.qml      ✅ (if needed for specificity)
```

## Naming Conventions

### Classes

**PascalCase:**

```cpp
class DataBroker;
class HaltechAdapter;
class ProtocolAdapterFactory;
```

### Functions and Methods

**camelCase:**

```cpp
void processFrame(const QCanBusFrame& frame);
double convertToFahrenheit(double celsius);
bool isRunning() const;
std::optional<ChannelValue> getChannel(const QString& name) const;
```

### Member Variables

**`m_` prefix:**

```cpp
class HaltechAdapter {
private:
    QCanBusDevice* m_canDevice = nullptr;
    QString m_interfaceName;
    double m_oilPressure{0.0};
    int m_currentGear{0};
    bool m_running{false};
};
```

### Constants

**SCREAMING_SNAKE_CASE or `k` prefix:**

```cpp
// Option 1: SCREAMING_SNAKE_CASE
constexpr int MAX_RPM = 8000;
constexpr double ATMOSPHERIC_PRESSURE_KPA = 101.325;

// Option 2: k prefix (Google style)
constexpr int kMaxRpm = 8000;
constexpr double kAtmosphericPressureKpa = 101.325;

// Be consistent within a file/module
```

### Namespaces

**lowercase, with `::` nesting:**

```cpp
namespace devdash {
namespace adapters {
namespace haltech {
    class HaltechProtocol;
}
}
}

// Or inline:
namespace devdash::adapters::haltech {
    class HaltechProtocol;
}
```

## Documentation (Doxygen)

All **public APIs** must have Doxygen documentation. Use `@` prefix style.

### Function Documentation

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

### Required Tags

- `@brief` - One-line summary (required for all public functions/classes)
- `@param` - Document each parameter
- `@return` - Document return value (if non-void)
- `@throws` - Document exceptions that may be thrown

### Optional Tags

- `@note` - Important usage notes
- `@warning` - Gotchas or dangerous behavior
- `@see` - Cross-references to related functions
- `@example` / `@code` - Usage examples
- `@pre` / `@post` - Preconditions and postconditions

### Class Documentation

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
};
```

### Don't Document

- Private implementation details (unless complex)
- Obvious getters/setters (the `@brief` is enough)
- Self-explanatory one-liners

## File Organization

### File Extensions

- Headers: `.h` extension
- Sources: `.cpp` extension

### File Naming

- Match class name: `DataBroker.h` / `DataBroker.cpp`
- One class per file (exceptions: small related classes)

### Include Guards

Use `#pragma once`:

```cpp
#pragma once

#include <QObject>

namespace devdash {
    class DataBroker : public QObject { };
}
```

### Include Order

Follow this order (enforced by clang-format):

```cpp
// 1. Corresponding header (for .cpp files)
#include "DataBroker.h"

// 2. Project headers
#include "IProtocolAdapter.h"
#include "adapters/HaltechAdapter.h"

// 3. Qt headers
#include <QObject>
#include <QString>
#include <QVariant>

// 4. System/STL headers
#include <memory>
#include <optional>
#include <vector>
```

## Formatting

### clang-format

The project uses `.clang-format` configuration based on LLVM style with Qt modifications.

**Format before committing:**

```bash
# Check formatting
cmake --build build/dev --target format-check

# Fix formatting
cmake --build build/dev --target format-fix
```

**Key style points:**

- **Indentation**: 4 spaces (no tabs)
- **Column limit**: 100 characters
- **Braces**: Attach style (`{` on same line)
- **Pointers**: Left-aligned (`int* ptr`, not `int *ptr`)
- **Line breaking**: Break before `&&`, `||` in long conditions

### Manual Style Points

**Brace placement:**

```cpp
// Good: Attach braces
void foo() {
    if (condition) {
        doSomething();
    }
}

// Bad: K&R style
void foo()
{
    if (condition)
    {
        doSomething();
    }
}
```

**Line length:**

- Target 100 characters (enforced by clang-format)
- Break long parameter lists:

```cpp
// Good: Vertical alignment
void longFunctionName(
    const QString& parameter1,
    const QJsonObject& parameter2,
    std::optional<int> parameter3
);

// Good: One parameter per line
connect(
    m_adapter.get(),
    &IProtocolAdapter::channelUpdated,
    this,
    &DataBroker::onChannelUpdated
);
```

## Linting & Static Analysis

### clang-tidy

Configured in `.clang-tidy`. Key checks enabled:

- `cppcoreguidelines-*` - SOLID, modern C++
- `performance-*` - Avoid copies, unnecessary allocations
- `bugprone-*` - Common mistakes
- `modernize-*` - Use modern C++ features
- `readability-*` - Clear code

**Run clang-tidy:**

```bash
cmake --preset dev -DDEVDASH_ENABLE_CLANG_TIDY=ON
cmake --build build/dev
```

**Suppressions:**

Use `NOLINTNEXTLINE` with a comment explaining why:

```cpp
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
constexpr double ATMOSPHERIC_PRESSURE_KPA = 101.325;  // Standard sea-level pressure
```

## Common Patterns

### Smart Pointers

```cpp
// Good: unique_ptr for ownership
std::unique_ptr<IProtocolAdapter> m_adapter;

// Good: QObject parent for Qt objects
auto* broker = new DataBroker(this);  // 'this' owns it

// Bad: Raw pointers for ownership
IProtocolAdapter* m_adapter;  // ❌ Who owns this?
```

### Range-Based For Loops

```cpp
// Good: Auto with const reference
for (const auto& channel : channels) {
    qInfo() << channel.name;
}

// Good: Structured bindings
for (const auto& [name, value] : channelMap) {
    qInfo() << name << "=" << value;
}

// Bad: Unnecessary copy
for (auto channel : channels) {  // ❌ Copies each element
    qInfo() << channel.name;
}
```

### QString Usage

```cpp
// Good: QStringLiteral for compile-time strings
const QString name = QStringLiteral("Haltech Adapter");

// Good: std::format for complex formatting
QString msg = QString::fromStdString(
    std::format("RPM: {}, Temp: {:.1f}", rpm, temp)
);

// Avoid: QString::arg() with many arguments
QString msg = QString("RPM: %1, Temp: %2, Pressure: %3")  // ❌ Hard to read
    .arg(rpm).arg(temp).arg(pressure);
```

## Don'ts

❌ **Don't use raw pointers for ownership**
- Use `std::unique_ptr`, `std::shared_ptr`, or QObject parent

❌ **Don't block the main thread**
- CAN parsing, file I/O go in worker threads
- Use Qt signals for cross-thread communication

❌ **Don't hardcode vehicle-specific values**
- Use vehicle profiles (`profiles/*.json`)

❌ **Don't suppress warnings without explanation**
- Always comment why a warning is suppressed

❌ **Don't use `QString::arg()` for many arguments**
- Use `std::format` or QStringLiteral

❌ **Don't `abort()` or `exit()` from library code**
- Return error codes or use std::optional

## CI Enforcement

The CI pipeline enforces:

- ✅ Code formatted with clang-format
- ✅ No clang-tidy warnings
- ✅ All warnings as errors (in CI preset)
- ✅ Tests pass
- ✅ Address Sanitizer + Undefined Behavior Sanitizer pass

**Before pushing, run locally:**

```bash
cmake --preset ci
cmake --build build/ci
ctest --preset ci
cmake --build build/ci --target format-check
```

## Next Steps

- **See examples**: Browse `src/adapters/haltech/` for reference code
- **Write tests**: [Testing Guidelines](testing-guidelines.md)
- **Add an adapter**: [Adding Protocol Adapters](adding-adapters.md)
