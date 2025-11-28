# Quick Start

Get /dev/dash up and running in 5 minutes.

## Prerequisites

- **Linux** (Ubuntu 22.04+ recommended) or macOS with Docker
- **Qt 6.4+** with QML and SerialBus modules
- **CMake 3.25+**
- **C++20 compiler** (GCC 11+ or Clang 14+)

**For dev container users:** Skip to [Using Dev Container](#using-dev-container) below.

## Install Dependencies (Ubuntu)

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake ninja-build git \
    qt6-base-dev qt6-declarative-dev qt6-multimedia-dev qt6-serialbus-dev \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    can-utils iproute2
```

## Build and Run

```bash
# 1. Clone
git clone https://github.com/yourorg/devdash.git
cd devdash

# 2. Configure
cmake --preset dev

# 3. Build
cmake --build build/dev

# 4. Run tests
ctest --test-dir build/dev --output-on-failure

# 5. Run with simulator (no hardware needed)
./build/dev/devdash --profile profiles/example-simulator.json
```

That's it! You should see the dashboard running with simulated data.

## Using Dev Container

If you're using VS Code with the Dev Containers extension:

```bash
# 1. Clone
git clone https://github.com/yourorg/devdash.git
cd devdash

# 2. Open in VS Code
code .

# 3. Reopen in container
# Cmd/Ctrl+Shift+P → "Dev Containers: Reopen in Container"

# 4. Build and run (inside container)
cmake --preset dev
cmake --build build/dev
ctest --test-dir build/dev --output-on-failure
```

The dev container includes all dependencies pre-installed and sets up a virtual CAN interface automatically.

## Running with Real Hardware

### With Haltech ECU (CAN bus)

```bash
# 1. Set up virtual CAN for testing (or use real CAN interface like can0)
./scripts/setup-vcan.sh

# 2. Start Haltech mock data (simulates ECU)
haltech-mock --interface vcan0 --scenario idle &

# 3. Run with Haltech profile
./build/dev/devdash --profile profiles/example-haltech.json
```

### With Your Own Vehicle Profile

Create a JSON profile (see [Vehicle Profiles](../01-architecture/vehicle-profiles.md)) and run:

```bash
./build/dev/devdash --profile profiles/my-vehicle.json
```

## Next Steps

- **Learn the architecture:** [Architecture Overview](../01-architecture/overview.md)
- **Understand adapters:** [Protocol Adapters](../01-architecture/protocol-adapters.md)
- **Add your ECU:** [Adding Protocol Adapters](../03-contributing/adding-adapters.md)
- **Detailed setup:** [Development Environment](development-environment.md)

## Troubleshooting

### "Qt6 not found"

Ensure Qt 6.4+ is installed. On Ubuntu:
```bash
sudo apt-get install qt6-base-dev qt6-declarative-dev qt6-serialbus-dev
```

### "cmake: command not found"

Install CMake 3.25 or later:
```bash
sudo apt-get install cmake
```

### "clang-format: not found"

Install clang tools (optional for development):
```bash
sudo apt-get install clang-format clang-tidy
```

### "tests failing"

Ensure you're running from the project root:
```bash
cd /workspaces/devdash  # or your clone location
ctest --test-dir build/dev --output-on-failure
```

For more help, see [GitHub Issues](https://github.com/yourorg/devdash/issues).
