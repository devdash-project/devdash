# Development Environment

This guide covers setting up your development environment for /dev/dash, both with and without Docker containers.

## Option 1: Dev Container (Recommended)

The easiest way to get started is using VS Code's Dev Containers extension, which provides a pre-configured Ubuntu environment with all dependencies installed.

### Prerequisites

- **VS Code** with the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- **Docker Desktop** (or Docker Engine + Docker Compose on Linux)

### Setup

```bash
# 1. Clone the repository
git clone https://github.com/yourorg/devdash.git
cd devdash

# 2. Open in VS Code
code .

# 3. Reopen in container
# Command Palette (Cmd/Ctrl+Shift+P) → "Dev Containers: Reopen in Container"
```

The container will automatically:
- Install all dependencies (Qt 6, CMake, GStreamer, CAN utils)
- Set up a virtual CAN interface (`vcan0`)
- Configure VS Code extensions (clangd, CMake Tools)
- Enable format-on-save with clang-format

### Inside the Container

```bash
# Configure
cmake --preset dev

# Build
cmake --build build/dev

# Run tests
ctest --test-dir build/dev --output-on-failure

# Run with simulator
./build/dev/devdash --profile profiles/example-simulator.json
```

### Dev Container Features

The `.devcontainer/` configuration includes:

- **ARM64 Ubuntu 22.04** base image (matches Jetson deployment target)
- **Privileged mode** for CAN interface access
- **Pre-installed Qt 6.5+** with QML, Multimedia, and SerialBus modules
- **Development tools**: clang, clang-format, clang-tidy, gdb
- **CAN utilities**: `can-utils`, `iproute2`
- **Python 3** for haltech-mock tool
- **VS Code extensions**:
  - `vscode-clangd` - C++ language server
  - `cmake-tools` - CMake integration
  - `cpptools` - C++ debugging

### Virtual CAN Setup

The dev container automatically runs `.devcontainer/setup-vcan.sh` on creation:

```bash
#!/bin/bash
set -e

# Load vcan module
sudo modprobe vcan || echo "vcan module not available (expected in container)"

# Create virtual CAN interface
sudo ip link add dev vcan0 type vcan 2>/dev/null || true
sudo ip link set up vcan0 2>/dev/null || true

echo "Virtual CAN interface vcan0 configured (if supported)"
```

To verify it worked:

```bash
ip link show vcan0
# Should show: vcan0: <NOARP,UP,LOWER_UP> ...
```

## Option 2: Manual Setup (Ubuntu 22.04+)

If you prefer not to use Docker, you can set up the environment manually.

### Install System Dependencies

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    clang \
    clang-format \
    clang-tidy \
    gdb \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-multimedia-dev \
    qt6-serialbus-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    can-utils \
    iproute2
```

### Set Up Virtual CAN

```bash
# Load vcan kernel module
sudo modprobe vcan

# Create vcan0 interface
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Verify
ip link show vcan0
```

**Make it persistent** (survives reboot):

```bash
# Create systemd service
sudo tee /etc/systemd/system/vcan0.service <<EOF
[Unit]
Description=Virtual CAN interface vcan0
After=network.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/sbin/ip link add dev vcan0 type vcan
ExecStart=/sbin/ip link set up vcan0
ExecStop=/sbin/ip link set down vcan0
ExecStop=/sbin/ip link delete dev vcan0

[Install]
WantedBy=multi-user.target
EOF

# Enable and start
sudo systemctl enable vcan0.service
sudo systemctl start vcan0.service
```

### Configure CMake

```bash
# From project root
cmake --preset dev
```

This uses the `dev` preset from `CMakePresets.json`:
- **Build type**: Debug
- **Tests**: Enabled
- **Sanitizers**: ASan + UBSan enabled
- **clang-tidy**: Disabled by default (enable with `-DDEVDASH_ENABLE_CLANG_TIDY=ON`)

### Build

```bash
cmake --build build/dev
```

### Run Tests

```bash
ctest --test-dir build/dev --output-on-failure
```

## CMake Presets

The project includes three presets in `CMakePresets.json`:

### `dev` (Development)

```bash
cmake --preset dev
cmake --build build/dev
```

- Debug build with symbols
- Tests enabled
- Address Sanitizer + Undefined Behavior Sanitizer
- Warnings enabled (not errors)
- clang-tidy disabled (too slow for iteration)

**Use for**: Day-to-day development, debugging

### `release` (Production)

```bash
cmake --preset release
cmake --build build/release
```

- Optimized build (-O3)
- Tests disabled
- Link-Time Optimization (LTO) enabled
- No sanitizers

**Use for**: Deployment builds, performance testing

### `ci` (Continuous Integration)

```bash
cmake --preset ci
cmake --build build/ci
```

- Debug build (for better diagnostics)
- Tests enabled
- Sanitizers enabled
- clang-tidy enabled
- **Warnings as errors** (strict)

**Use for**: CI/CD pipelines, pre-commit checks

## IDE Setup

### VS Code (Recommended)

The dev container configures VS Code automatically. For manual setups, install these extensions:

- [vscode-clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) - C++ language server
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) - Debugging

**Settings** (`.vscode/settings.json`):

```json
{
  "cmake.configureOnOpen": true,
  "cmake.generator": "Ninja",
  "clangd.arguments": [
    "--background-index",
    "--clang-tidy",
    "--header-insertion=iwyu",
    "--completion-style=detailed"
  ],
  "editor.formatOnSave": true,
  "[cpp]": {
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd"
  }
}
```

### CLion

CLion works well with CMake projects:

1. **Open** the project root directory
2. CLion auto-detects `CMakeLists.txt`
3. **Select preset**: Choose `dev` from the CMake preset dropdown
4. **Enable clang-tidy**: Preferences → Editor → Inspections → C/C++ → Clang-Tidy

### Qt Creator

Qt Creator has excellent QML support:

1. **Open** `CMakeLists.txt` as a project
2. **Configure kit**: Desktop Qt 6.5+ (system installation)
3. **QML debugging**: Ensure `QML_IMPORT_PATH` is set in run configuration

## Useful Scripts

### Format Code

```bash
# Check formatting (CI-safe)
cmake --build build/dev --target format-check

# Fix formatting
cmake --build build/dev --target format-fix
```

### Run Static Analysis

```bash
# Enable clang-tidy in CMake
cmake --preset dev -DDEVDASH_ENABLE_CLANG_TIDY=ON

# Build (clang-tidy runs automatically)
cmake --build build/dev
```

### Generate Documentation

```bash
# Doxygen (if configured)
cmake --build build/dev --target docs

# Output: build/dev/docs/html/index.html
```

## Testing the CAN Setup

### Send Test CAN Frames

```bash
# Install cansend (part of can-utils)
sudo apt-get install can-utils

# Send a test frame (Haltech RPM frame, 3500 RPM)
cansend vcan0 360#0DAC03E8000003E8
```

### Monitor CAN Traffic

```bash
# Watch all frames on vcan0
candump vcan0

# With timestamps
candump -t a vcan0

# Filter specific frame ID
candump vcan0,360:7FF
```

### Use haltech-mock

Install the mock ECU simulator (if available):

```bash
pip3 install haltech-mock
```

Run a scenario:

```bash
# Simulate idle engine
haltech-mock --interface vcan0 --scenario idle

# Simulate high RPM
haltech-mock --interface vcan0 --scenario redline
```

## Troubleshooting

### Qt6 not found

**Error**: `Could not find a package configuration file provided by "Qt6"`

**Solution**: Install Qt 6.5+ development packages:

```bash
sudo apt-get install qt6-base-dev qt6-declarative-dev qt6-serialbus-dev
```

### vcan0 not available

**Error**: `RTNETLINK answers: Operation not supported`

**Cause**: Running in a container without `NET_ADMIN` capability or kernel without vcan support.

**Solution**:
- For containers: Use `--cap-add=NET_ADMIN` or `--privileged`
- For WSL2: vcan is not supported; use Docker Desktop instead

### Sanitizer errors on macOS

**Error**: `Sanitizer does not support this architecture`

**Solution**: Sanitizers require x86_64 or ARM64 Linux. On macOS, disable them:

```bash
cmake --preset dev -DDEVDASH_ENABLE_SANITIZERS=OFF
```

### clang-tidy is very slow

**Cause**: clang-tidy analyzes every source file, adding significant compile time.

**Solution**: Only enable for CI or pre-commit checks:

```bash
# Disable for dev builds (default)
cmake --preset dev

# Enable only when needed
cmake --preset ci
```

### Tests fail with "Could not load the Qt platform plugin"

**Error**: `qt.qpa.plugin: Could not load the Qt platform plugin "xcb"`

**Cause**: Running GUI tests without a display server.

**Solution**: Use the offscreen platform:

```bash
export QT_QPA_PLATFORM=offscreen
ctest --test-dir build/dev
```

## Next Steps

- **Run the application**: [Quick Start](quick-start.md)
- **Write tests**: [Running Tests](running-tests.md)
- **Understand the architecture**: [Architecture Overview](../01-architecture/overview.md)
