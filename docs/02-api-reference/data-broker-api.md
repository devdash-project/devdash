# DataBroker API Reference

**Status:** 🚧 Stub - To be completed

Complete API reference for the `DataBroker` class.

## Planned Content

### Class Overview

- Constructor and destructor
- Lifecycle methods
- Property getters
- Signals
- Slots

### Q_PROPERTY Reference

Complete list of all properties exposed to QML:

- Engine properties (rpm, throttlePosition, manifoldPressure)
- Temperature properties (coolant, oil, intake air)
- Pressure properties (oil, fuel)
- Electrical properties (batteryVoltage)
- Vehicle properties (vehicleSpeed, gear)
- Status properties (isConnected)

### Methods

- `setAdapter()` - Set the protocol adapter
- `start()` - Start receiving data
- `stop()` - Stop receiving data

### Signals

- Property change signals (`rpmChanged()`, etc.)
- Connection state signals
- Error signals

### Usage from QML

- Property binding examples
- Signal handling examples
- Connections usage

### Usage from C++

- Creating and configuring DataBroker
- Injecting adapters
- Connecting to signals
- Exposing to QML context

## Resources

Until this document is complete, refer to:

- **DataBroker Architecture:** [DataBroker](../01-architecture/data-broker.md)
- **Source Code:** `src/core/DataBroker.h`
- **Core Interfaces:** [Core Interfaces](core-interfaces.md)

## TODO

- [ ] Document all Q_PROPERTY declarations
- [ ] Document all signals
- [ ] Document all public methods
- [ ] Add QML usage examples
- [ ] Add C++ usage examples
- [ ] Document thread safety guarantees
- [ ] Document error handling

## Contributing

If you'd like to help complete this documentation, see [Contributing Guidelines](../03-contributing/code-style.md).
