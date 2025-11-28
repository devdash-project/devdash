# /dev/dash Documentation

> Modular automotive dashboard framework for Linux

## Quick Navigation

**New to /dev/dash?** Start here:
- [Quick Start](00-getting-started/quick-start.md) - Get up and running in 5 minutes
- [Architecture Overview](01-architecture/overview.md) - Understand the system design
- [Adding Protocol Adapters](03-contributing/adding-adapters.md) - Add support for your ECU

**Looking for API docs?**
- [Core Interfaces](02-api-reference/core-interfaces.md) - IProtocolAdapter, ChannelValue
- [Haltech Protocol API](02-api-reference/haltech-protocol.md) - HaltechProtocol, PD16Protocol
- [DataBroker API](02-api-reference/data-broker-api.md) - QML property bindings

---

## 📚 Documentation Sections

### [00-getting-started](00-getting-started/)
Start here if you're new to the project.

- **[Quick Start](00-getting-started/quick-start.md)** - Clone, build, and run in 5 minutes
- **[Development Environment](00-getting-started/development-environment.md)** - Dev container and manual setup
- **[Running Tests](00-getting-started/running-tests.md)** - Unit tests, integration tests, and mocking

### [01-architecture](01-architecture/)
Understand the system design and architecture.

- **[Overview](01-architecture/overview.md)** - High-level architecture, layer diagram, data flow
- **[Protocol Adapters](01-architecture/protocol-adapters.md)** - The adapter pattern, IProtocolAdapter interface, swappable protocols
- **[DataBroker](01-architecture/data-broker.md)** - Central data hub, QML exposure, signal/slot connections
- **[UI Layer](01-architecture/ui-layer.md)** - QML architecture, ClusterWindow, HeadUnitWindow
- **[Vehicle Profiles](01-architecture/vehicle-profiles.md)** - JSON configuration, gauge layouts, alert thresholds

### [02-api-reference](02-api-reference/)
Detailed API documentation for developers.

- **[Core Interfaces](02-api-reference/core-interfaces.md)** - IProtocolAdapter, ChannelValue struct
- **[Haltech Protocol](02-api-reference/haltech-protocol.md)** - HaltechProtocol and PD16Protocol classes
- **[DataBroker API](02-api-reference/data-broker-api.md)** - Q_PROPERTY bindings and signals

### [03-contributing](03-contributing/)
Contributing guidelines and development workflows.

- **[Code Style](03-contributing/code-style.md)** - C++ and QML conventions, naming, documentation
- **[Adding Protocol Adapters](03-contributing/adding-adapters.md)** - Step-by-step guide for new ECU protocols
- **[Adding Gauges](03-contributing/adding-gauges.md)** - Creating new QML gauge components
- **[Testing Guidelines](03-contributing/testing-guidelines.md)** - Test patterns, mocking, coverage

### [10-implementation](10-implementation/)
Implementation phase documentation (for contributors working through the project phases).

- **[00-overview](10-implementation/00-overview/)** - Project spec and repository setup
- **[01-foundation](10-implementation/01-foundation/)** - Core abstractions and base classes
- **[02-haltech-adapter](10-implementation/02-haltech-adapter/)** - Haltech ECU and PD16 adapter implementation
- **[03-instrument-cluster](10-implementation/03-instrument-cluster/)** - Instrument cluster UI
- **[04-head-unit](10-implementation/04-head-unit/)** - Head unit / infotainment UI
- **[05-integration](10-implementation/05-integration/)** - Cross-window integration and settings sync
- **[06-audio-extras](10-implementation/06-audio-extras/)** - Audio and emulation features
- **[07-hardware-deployment](10-implementation/07-hardware-deployment/)** - Jetson deployment and hardware reference

---

## Project Links

- **Repository:** [github.com/yourorg/devdash](https://github.com/yourorg/devdash)
- **Issue Tracker:** [github.com/yourorg/devdash/issues](https://github.com/yourorg/devdash/issues)
- **Discussions:** [github.com/yourorg/devdash/discussions](https://github.com/yourorg/devdash/discussions)

## External Resources

- **Qt Documentation:** [doc.qt.io/qt-6](https://doc.qt.io/qt-6/)
- **SocketCAN:** [kernel.org/doc/html/latest/networking/can.html](https://www.kernel.org/doc/html/latest/networking/can.html)
- **NVIDIA Jetson:** [developer.nvidia.com/jetson](https://developer.nvidia.com/jetson)

---

## License

TBD - considering MIT or GPLv3
