# /dev/dash

[![CI](https://github.com/yourorg/devdash/workflows/CI/badge.svg)](https://github.com/yourorg/devdash/actions)
[![codecov](https://codecov.io/gh/yourorg/devdash/branch/main/graph/badge.svg)](https://codecov.io/gh/yourorg/devdash)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![Qt](https://img.shields.io/badge/Qt-6.5%2B-green.svg)](https://www.qt.io/)

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

### Development with VS Code Dev Container (Recommended)

The fastest way to get started:

```bash
# Clone the repository
git clone https://github.com/yourorg/devdash.git
cd devdash

# Open in VS Code
code .
```

In VS Code:
1. Press **Cmd+Shift+P** (Mac) or **Ctrl+Shift+P** (Windows/Linux)
2. Type: **"Dev Containers: Reopen in Container"**
3. Press **Enter**

The dev container includes all dependencies (Qt 6, CMake, compilers, tools).

**Setup virtual CAN (one-time):**

See the [Development Setup Guide](docs/00-getting-started/development-setup.md) for platform-specific vcan0 setup:
- **macOS**: Create vcan0 inside Docker Desktop VM
- **Linux**: Native vcan0 setup
- **Windows**: vcan0 in WSL2

**Build and test:**

```bash
# Build
cmake --build build/dev

# Run tests
ctest --test-dir build/dev

# Run with realistic mock data
./scripts/run-with-mock idle
```

### Native Build (Without Docker)

**Prerequisites:**
- CMake 3.25+
- Qt 6.5+ (Core, Quick, Qml, Multimedia, SerialBus)
- C++20 compiler (GCC 12+, Clang 15+)
- Linux with SocketCAN support

**Build:**

```bash
cmake --preset dev
cmake --build build/dev
ctest --test-dir build/dev
```

**Run:**

```bash
# With realistic mock data (requires haltech-mock + vcan0)
./scripts/run-with-mock idle

# With real Haltech ECU
./build/dev/devdash --profile profiles/example-haltech.json

# Cluster only
./build/dev/devdash --cluster-only --profile profiles/haltech-vcan.json
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

**New to the project?** See the [Development Setup Guide](docs/00-getting-started/development-setup.md) for detailed environment setup instructions.

### Code Style

- C++20 standard
- Qt/QML naming conventions
- clang-format for formatting (auto-applied on save in dev container)
- clang-tidy for static analysis

```bash
# Format code
cmake --build build/dev --target format-fix

# Check formatting
cmake --build build/dev --target format-check

# Run clang-tidy
cmake --build build/dev --target clang-tidy
```

### Adding a New Protocol Adapter

1. Create `src/adapters/newprotocol/` directory
2. Implement `IProtocolAdapter` interface
3. Register in `ProtocolAdapterFactory`
4. Add protocol definition JSON in `protocols/`
5. Add tests in `tests/adapters/newprotocol/`

## License

MIT License - see [LICENSE](LICENSE) for details.
