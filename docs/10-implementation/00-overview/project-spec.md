# /dev/dash

## Project Overview

**/dev/dash** is a modular, open-source automotive dashboard and infotainment framework for Linux. It provides a flexible foundation for building custom instrument clusters and head units for any vehicle - from custom builds and kit cars to retrofits and racing applications.

### Design Philosophy

- **Modular**: Protocol adapters, display layouts, and features are pluggable
- **Vehicle Agnostic**: Core framework doesn't assume any specific vehicle or ECU
- **Linux Native**: Built on Linux, leveraging SocketCAN, GStreamer, PipeWire
- **SOLID Principles**: Clean separation of concerns, dependency injection, interface-based design
- **Dual Display**: First-class support for instrument cluster + head unit configurations

### Core Features

- Multi-display management (cluster + head unit)
- Configurable gauge rendering (analog, digital, bar, sweep)
- Multi-camera video aggregation
- Audio system integration
- Theming (day/night modes, custom themes)
- Alert/warning system with configurable thresholds
- Settings persistence
- Physical button input support

## Target Hardware (Reference Platform)

The reference implementation targets:

- **Compute**: NVIDIA Jetson Orin Nano (8GB)
- **Carrier Board**: Forecr DSBOARD-ORNX or D3 Embedded 8-Camera Carrier
- **Cameras**: GMSL2 cameras (FAKRA connectors, Power-over-Coax)
- **Displays**: 
  - Instrument cluster: Non-touch, driver-facing
  - Head unit: Capacitive touch, center console
- **CAN Interface**: USB-CAN adapter or native CAN on carrier board
- **Audio**: USB DAC or I2S output

However, the architecture supports any Linux-capable hardware with appropriate display outputs and CAN interfaces.

## Architecture

### Layer Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Vehicle Profile                                 │
│         (JSON config: which adapters, gauges, layouts, alerts)              │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
┌─────────────────────────────────────────────────────────────────────────────┐
│                           /dev/dash Application                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                              Core Services                                   │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────────┐  │
│  │   DataBroker    │  │  CameraManager  │  │     AudioManager            │  │
│  │ (unified data   │  │  (GStreamer)    │  │     (PipeWire)              │  │
│  │  model)         │  │                 │  │                             │  │
│  └────────┬────────┘  └────────┬────────┘  └─────────────┬───────────────┘  │
│           │                    │                         │                   │
│  ┌────────┴────────┐           │                         │                   │
│  │ Protocol        │           │                         │                   │
│  │ Adapters        │           │                         │                   │
│  │ ┌────────────┐  │           │                         │                   │
│  │ │  Haltech   │  │           │                         │                   │
│  │ ├────────────┤  │           │                         │                   │
│  │ │  OBD-II    │  │           │                         │                   │
│  │ ├────────────┤  │           │                         │                   │
│  │ │ MegaSquirt │  │           │                         │                   │
│  │ ├────────────┤  │           │                         │                   │
│  │ │   Custom   │  │           │                         │                   │
│  │ └────────────┘  │           │                         │                   │
│  └─────────────────┘           │                         │                   │
│                                │                         │                   │
│           ┌────────────────────┴─────────────────────────┘                   │
│           │                                                                  │
│           ▼                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        UI Layer (QML)                                │    │
│  │  ┌─────────────────┐              ┌─────────────────────────────┐   │    │
│  │  │  ClusterWindow  │              │     HeadUnitWindow          │   │    │
│  │  │  (Display :0)   │              │     (Display :1)            │   │    │
│  │  │                 │              │                             │   │    │
│  │  │  ┌───────────┐  │  Settings    │  ┌─────┬─────┬─────┬─────┐ │   │    │
│  │  │  │  Gauges   │◄─┼──────────────┼─►│ Cam │Audio│ Emu │ Set │ │   │    │
│  │  │  │  Alerts   │  │   Sync       │  └─────┴─────┴─────┴─────┘ │   │    │
│  │  │  │  Shift    │  │              │                             │   │    │
│  │  │  └───────────┘  │              │                             │   │    │
│  │  └─────────────────┘              └─────────────────────────────┘   │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Key Abstractions

#### IProtocolAdapter (Interface)

All ECU/vehicle protocol adapters implement this interface:

```cpp
class IProtocolAdapter : public QObject {
    Q_OBJECT
public:
    virtual ~IProtocolAdapter() = default;
    
    // Lifecycle
    virtual bool initialize(const QVariantMap& config) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    
    // Capability discovery
    virtual QStringList availableChannels() const = 0;
    virtual ChannelInfo channelInfo(const QString& channel) const = 0;
    
signals:
    // Unified data output
    void channelUpdated(const QString& channel, const QVariant& value);
    void connectionStateChanged(ConnectionState state);
    void errorOccurred(const QString& error);
};
```

#### DataBroker

Central data hub that:
- Loads the appropriate protocol adapter based on vehicle profile
- Normalizes all incoming data to a unified model
- Exposes data via Q_PROPERTY for QML binding
- Handles unit conversions based on user preferences
- Manages alert threshold evaluation

#### Vehicle Profile (JSON)

```json
{
  "name": "Moon Patrol",
  "description": "Custom rock crawler with Haltech Elite 2500",
  "protocol": {
    "adapter": "haltech",
    "config": {
      "interface": "can0",
      "bitrate": 1000000,
      "protocol_version": "2.35"
    }
  },
  "cluster": {
    "layout": "racing",
    "gauges": [
      {"type": "tachometer", "channel": "RPM", "position": "primary"},
      {"type": "digital", "channel": "Coolant Temperature", "position": "aux1"},
      {"type": "digital", "channel": "Oil Temperature", "position": "aux2"},
      {"type": "bar", "channel": "Oil Pressure", "position": "aux3"}
    ],
    "shiftLight": {
      "channel": "RPM",
      "thresholds": [5500, 6000, 6500, 7000]
    }
  },
  "alerts": {
    "Coolant Temperature": {"warning": 105, "critical": 115, "units": "C"},
    "Oil Pressure": {"warning_low": 15, "critical_low": 10, "units": "psi"}
  },
  "cameras": [
    {"id": "front", "label": "Front", "source": "/dev/video0"},
    {"id": "rear", "label": "Backup", "source": "/dev/video1"}
  ],
  "preferences": {
    "units": "imperial",
    "theme": "night"
  }
}
```

### Protocol Adapters

#### Included Adapters

1. **Haltech** (`haltech`)
   - Supports Elite series (550, 750, 1000, 1500, 2000, 2500)
   - Protocol V2.35 (100+ channels)
   - 1 Mbit/s CAN
   - Reference: `protocols/haltech/haltech-can-protocol-v2.35.json`

2. **OBD-II** (`obd2`) - Future
   - Standard PIDs
   - Works with any OBD-II compliant vehicle

3. **MegaSquirt** (`megasquirt`) - Future
   - MS1/MS2/MS3 support
   - CAN and serial protocols

4. **Simulation** (`simulator`)
   - For development and testing
   - Configurable fake data generation

#### Adding New Adapters

1. Create a class implementing `IProtocolAdapter`
2. Register in `ProtocolAdapterFactory`
3. Add protocol definition JSON if applicable
4. Adapter is now selectable in vehicle profiles

## Project Structure

```
devdash/
├── .devcontainer/
│   ├── devcontainer.json
│   ├── Dockerfile
│   └── setup-vcan.sh
├── CMakeLists.txt
├── README.md
│
├── src/
│   ├── main.cpp
│   ├── Application.cpp/.h           # Main app, display management
│   │
│   ├── core/
│   │   ├── IProtocolAdapter.h       # Protocol adapter interface
│   │   ├── ProtocolAdapterFactory.cpp/.h
│   │   ├── DataBroker.cpp/.h        # Unified data model
│   │   ├── ChannelInfo.h            # Channel metadata struct
│   │   ├── AlertManager.cpp/.h      # Warning/alert logic
│   │   ├── ConfigManager.cpp/.h     # Settings persistence
│   │   └── VehicleProfile.cpp/.h    # Profile loading/validation
│   │
│   ├── adapters/
│   │   ├── haltech/
│   │   │   ├── HaltechAdapter.cpp/.h
│   │   │   └── HaltechProtocol.cpp/.h   # Frame parsing
│   │   ├── obd2/
│   │   │   └── Obd2Adapter.cpp/.h
│   │   └── simulator/
│   │       └── SimulatorAdapter.cpp/.h
│   │
│   ├── video/
│   │   ├── CameraManager.cpp/.h
│   │   ├── VideoSurface.cpp/.h      # GStreamer → QML bridge
│   │   └── CameraConfig.h
│   │
│   ├── audio/
│   │   └── AudioManager.cpp/.h
│   │
│   ├── input/
│   │   └── PhysicalButtons.cpp/.h   # GPIO/USB HID
│   │
│   ├── cluster/
│   │   ├── ClusterWindow.cpp/.h
│   │   └── qml/
│   │       ├── ClusterMain.qml
│   │       ├── gauges/
│   │       │   ├── Tachometer.qml
│   │       │   ├── Speedometer.qml
│   │       │   ├── DigitalGauge.qml
│   │       │   ├── BarGauge.qml
│   │       │   └── SweepGauge.qml
│   │       ├── ShiftLight.qml
│   │       ├── GearIndicator.qml
│   │       ├── WarningPanel.qml
│   │       └── layouts/
│   │           ├── LayoutRacing.qml
│   │           ├── LayoutCruising.qml
│   │           └── LayoutMinimal.qml
│   │
│   ├── headunit/
│   │   ├── HeadUnitWindow.cpp/.h
│   │   └── qml/
│   │       ├── HeadUnitMain.qml
│   │       ├── views/
│   │       │   ├── CameraView.qml
│   │       │   ├── AudioView.qml
│   │       │   ├── EmulationView.qml
│   │       │   └── SettingsView.qml
│   │       └── settings/
│   │           ├── ClusterSettings.qml
│   │           ├── AlertSettings.qml
│   │           ├── DisplaySettings.qml
│   │           └── VehicleSettings.qml
│   │
│   └── shared/
│       └── qml/
│           ├── Theme.qml
│           ├── Units.qml            # Unit conversion helpers
│           └── components/
│               ├── TouchButton.qml
│               ├── Slider.qml
│               └── Toggle.qml
│
├── protocols/
│   └── haltech/
│       └── haltech-can-protocol-v2.35.json
│
├── profiles/
│   ├── example-haltech.json
│   ├── example-obd2.json
│   └── moon-patrol.json             # Your custom profile
│
├── resources/
│   ├── fonts/
│   ├── icons/
│   └── themes/
│       ├── day.json
│       └── night.json
│
└── tests/
    ├── test_haltech_adapter.cpp
    ├── test_data_broker.cpp
    ├── test_alert_logic.cpp
    └── test_unit_conversion.cpp
```

## Development Environment Setup

### Dev Container (Apple Silicon Mac / ARM64 Linux)

```dockerfile
# Base: Ubuntu 22.04 ARM64 (matches JetPack 6 / L4T)
FROM arm64v8/ubuntu:22.04

# This will NOT have NVIDIA GPU acceleration, but will allow:
# - Qt/QML development and testing
# - CAN bus protocol development with virtual CAN (vcan)
# - UI layout and logic development
# - Mock data integration testing
```

**Dev container requirements:**
- Ubuntu 22.04 ARM64 base
- Qt 6.x with QML, Quick, Multimedia, SerialBus modules
- GStreamer 1.x with base/good/bad/ugly plugins
- SocketCAN utilities (can-utils, iproute2)
- Virtual CAN (vcan) kernel module support
- PipeWire for audio development
- Build tools: CMake, Ninja, GCC/G++, pkg-config
- Development libraries: OpenGL ES, EGL
- Python 3 with python-can for mock data generation

### Hardware Deployment (Jetson)

When deploying to actual Jetson:
- Flash JetPack 6.x (Ubuntu 22.04 based)
- Install NVIDIA-specific: DeepStream, nvarguscamerasrc, CUDA (optional)
- Configure real CAN interface
- Configure GMSL2 camera drivers

## Development Phases

### Phase 1: Foundation
1. [ ] Set up dev container with Qt 6, GStreamer, SocketCAN tools
2. [ ] Create vcan setup script
3. [ ] Basic CMake project structure with Qt 6
4. [ ] Implement `IProtocolAdapter` interface
5. [ ] Implement `SimulatorAdapter` for testing
6. [ ] DataBroker with Q_PROPERTY bindings
7. [ ] Simple test QML displaying data

### Phase 2: Haltech Adapter
1. [ ] Load protocol definition from JSON
2. [ ] CAN frame parsing (all frame types)
3. [ ] Proper frame rate handling
4. [ ] Unit conversion support
5. [ ] Integration with external `haltech-mock` library for testing

### Phase 3: Instrument Cluster
1. [ ] ClusterWindow with display detection
2. [ ] Tachometer component (smooth animation)
3. [ ] Generic gauge components (bar, sweep, digital)
4. [ ] Shift light bar
5. [ ] Gear indicator
6. [ ] Warning panel
7. [ ] Layout system (multiple presets)
8. [ ] Day/night theming

### Phase 4: Head Unit
1. [ ] HeadUnitWindow for second display
2. [ ] Navigation structure with swipe/touch
3. [ ] Settings view (including cluster configuration)
4. [ ] Vehicle profile editor
5. [ ] Mock camera views (placeholders)
6. [ ] Camera grid layouts

### Phase 5: Integration
1. [ ] Settings ↔ Cluster synchronization
2. [ ] Physical button input (GPIO/USB HID)
3. [ ] Alert system with thresholds from profile
4. [ ] Full settings persistence

### Phase 6: Audio & Extras
1. [ ] PipeWire/PulseAudio integration
2. [ ] Bluetooth audio (BlueZ)
3. [ ] EmulationStation integration (safety interlock)

### Phase 7: Hardware Deployment
1. [ ] Deploy to Jetson Orin Nano
2. [ ] Real CAN bus testing
3. [ ] GMSL2 camera integration
4. [ ] Multi-display configuration
5. [ ] Performance optimization
6. [ ] Boot time optimization

## Coding Standards

### C++
- Modern C++17/20 features
- SOLID principles throughout
- Dependency injection for adapters
- Interface-based design for extensibility
- `Q_PROPERTY` with `NOTIFY` for all QML-exposed data
- Defensive programming (validate CAN data, handle disconnects)

### QML
- Keep declarative; complex logic in C++
- Bind to properties, don't poll
- Components < 200 lines
- Consistent naming: `ComponentName.qml`

### Testing
- Unit tests for protocol adapters
- Unit tests for data parsing
- Unit tests for unit conversions
- Integration tests for data flow

## Getting Started

```bash
# Clone repository
git clone <repo-url> devdash
cd devdash

# Open in VS Code with Dev Containers
code .
# Cmd+Shift+P → "Dev Containers: Reopen in Container"

# Inside container: Set up virtual CAN
./scripts/setup-vcan.sh

# Start mock data (using haltech-mock library)
haltech-mock --interface vcan0 --scenario idle &

# Build
mkdir -p build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja

# Run with a vehicle profile
./devdash --profile ../profiles/moon-patrol.json

# Run with simulator adapter (no CAN needed)
./devdash --profile ../profiles/example-simulator.json
```

## External Dependencies

### haltech-mock (Separate Project)

For testing Haltech adapter without real hardware, use the `haltech-mock` library:
- Repository: (separate repo)
- Provides realistic CAN data simulation
- Multiple test scenarios
- Python library and CLI tool

See the haltech-mock project for installation and usage.

## References

### Qt Documentation
- Qt 6 Overview: https://doc.qt.io/qt-6/
- Qt Quick/QML: https://doc.qt.io/qt-6/qtquick-index.html
- Qt Serial Bus (CAN): https://doc.qt.io/qt-6/qtserialbus-index.html
- Qt Multimedia: https://doc.qt.io/qt-6/qtmultimedia-index.html

### NVIDIA Jetson
- JetPack 6: https://developer.nvidia.com/embedded/jetpack
- GStreamer on Jetson: https://docs.nvidia.com/jetson/archives/r36.2/DeveloperGuide/
- DeepStream: https://developer.nvidia.com/deepstream-sdk

### CAN Bus
- SocketCAN: https://www.kernel.org/doc/html/latest/networking/can.html
- python-can: https://python-can.readthedocs.io/
- can-utils: https://github.com/linux-can/can-utils

### Protocol References
- Haltech CAN: See `protocols/haltech/haltech-can-protocol-v2.35.json`
- OBD-II PIDs: https://en.wikipedia.org/wiki/OBD-II_PIDs

## License

TBD - considering MIT or GPLv3
