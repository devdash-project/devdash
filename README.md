# /dev/dash

A modular automotive dashboard framework built with Qt 6/QML and C++20. Targets NVIDIA Jetson but runs anywhere with Qt support.

## Features

- **Dual Display Support**: Separate instrument cluster and head unit windows
- **Protocol Adapters**: Modular support for different ECU protocols (Haltech CAN, OBD-II)
- **Simulator Mode**: Built-in data simulator for development without hardware
- **Profile-Based Configuration**: Vehicle-specific settings in JSON profiles
- **Modern C++20**: Concepts, ranges, `std::format`, `std::optional`

## Architecture

```
IProtocolAdapter (interface)
    └── HaltechAdapter, Obd2Adapter, SimulatorAdapter
DataBroker (central data hub, exposes Q_PROPERTY for QML)
    └── Receives data from adapters, handles unit conversion
ClusterWindow / HeadUnitWindow (QML-based UIs)
```

## Quick Start

### Prerequisites

- CMake 3.25+
- Qt 6.5+ (Core, Quick, Qml, Multimedia, SerialBus)
- C++20 compiler (GCC 12+, Clang 15+)
- Linux with SocketCAN support (for real CAN bus)

### Build

```bash
# Configure
cmake --preset dev

# Build
cmake --build build/dev

# Run tests
ctest --preset dev
```

### Run

```bash
# With simulator (default)
./build/dev/devdash

# With Haltech ECU
./build/dev/devdash --profile profiles/example-haltech.json

# Cluster only
./build/dev/devdash --cluster-only

# Head unit only
./build/dev/devdash --headunit-only
```

### Virtual CAN for Testing

```bash
# Setup virtual CAN interface
./scripts/setup-vcan.sh

# Send test frames
cansend vcan0 360#0DAC01F400000000  # RPM=3500, TPS=50%
```

## Project Structure

```
devdash/
├── src/
│   ├── core/           # DataBroker, IProtocolAdapter interface
│   ├── adapters/       # Protocol adapters (Haltech, Simulator)
│   ├── cluster/        # Instrument cluster window + QML
│   └── headunit/       # Head unit window + QML
├── tests/              # Catch2 unit tests
├── profiles/           # Vehicle configuration profiles
├── protocols/          # Protocol definitions (JSON)
├── cmake/              # CMake modules
└── scripts/            # Helper scripts
```

## Configuration

Vehicle profiles are JSON files that specify:
- Adapter type and configuration
- Display settings (screens, layouts)
- Unit preferences (metric/imperial)
- Warning thresholds

See `profiles/example-simulator.json` for a complete example.

## Development

### Code Style

- C++20 standard
- Qt/QML naming conventions
- clang-format for formatting
- clang-tidy for static analysis

```bash
# Format code
./scripts/format-all.sh

# Check formatting
./scripts/format-all.sh --check

# Run with clang-tidy
cmake --preset ci
cmake --build build/ci
```

### Adding a New Protocol Adapter

1. Create `src/adapters/newprotocol/` directory
2. Implement `IProtocolAdapter` interface
3. Register in `ProtocolAdapterFactory`
4. Add protocol definition JSON in `protocols/`
5. Add tests in `tests/adapters/newprotocol/`

## License

MIT License - see [LICENSE](LICENSE) for details.
