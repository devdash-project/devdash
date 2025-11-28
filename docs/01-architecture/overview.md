# Architecture Overview

## Design Philosophy

**/dev/dash** is built on a foundation of modularity and extensibility:

- **Modular**: Protocol adapters, display layouts, and features are pluggable
- **Vehicle Agnostic**: Core framework doesn't assume any specific vehicle or ECU
- **SOLID Principles**: Clean separation of concerns, dependency injection, interface-based design
- **Linux Native**: Built on Linux, leveraging SocketCAN, GStreamer, and Qt 6
- **Dual Display**: First-class support for instrument cluster + head unit configurations

## Layer Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Vehicle Profile                                 │
│         (JSON config: which adapters, gauges, layouts, alerts)              │
└────────────────────────────────┬────────────────────────────────────────────┘
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

## Data Flow

```
Vehicle (CAN bus)
    ↓
Protocol Adapter (Haltech, OBD-II, etc.)
    ↓ channelUpdated signal
DataBroker
    ↓ Q_PROPERTY changes
QML UI (Cluster + Head Unit)
    ↓
User sees gauges update in real-time
```

## Key Components

### Protocol Adapters

**Purpose:** Translate vehicle-specific protocols (CAN, OBD-II, serial) into standard channel names.

**Key Point:** All adapters implement the `IProtocolAdapter` interface, making them **interchangeable**.

See [Protocol Adapters](protocol-adapters.md) for detailed explanation.

### DataBroker

**Purpose:** Central data hub that:
- Receives channel updates from adapters
- Normalizes data (unit conversions)
- Exposes data to QML via Q_PROPERTY
- Handles alert threshold evaluation

See [DataBroker](data-broker.md) for detailed explanation.

### UI Layer (QML)

**Purpose:** Displays data to the user across two windows:
- **ClusterWindow**: Driver-facing instrument cluster (non-touch)
- **HeadUnitWindow**: Center console head unit (touch interface)

Both windows bind to the same DataBroker properties, ensuring synchronized data display.

See [UI Layer](ui-layer.md) for detailed explanation.

### Vehicle Profiles (JSON)

**Purpose:** Configuration files that define:
- Which protocol adapter to use
- Gauge layout and types
- Alert thresholds
- Camera configuration
- User preferences

See [Vehicle Profiles](vehicle-profiles.md) for detailed explanation.

## Dependency Inversion in Action

The architecture demonstrates the **Dependency Inversion Principle** (the "D" in SOLID):

```
High-level modules (DataBroker, QML)
        ↓ depends on
    IProtocolAdapter (interface)
        ↑ implemented by
Low-level modules (HaltechAdapter, OBD2Adapter, etc.)
```

This means:
- ✅ **Easy to add new adapters** - implement `IProtocolAdapter` and you're done
- ✅ **Easy to test** - use `SimulatorAdapter` or mocks
- ✅ **Easy to swap** - change one line in your vehicle profile JSON

## Thread Safety

- **CAN bus reading**: Runs on adapter's own thread
- **Data emission**: Uses Qt signals (automatically marshalled to main thread)
- **QML updates**: Always on main thread (Qt handles this)
- **Result**: Thread-safe by design, no manual locking needed

## Configuration Flow

```
1. User creates vehicle-profile.json
2. ProtocolAdapterFactory reads profile
3. Factory creates the specified adapter (e.g., HaltechAdapter)
4. DataBroker receives the adapter via setAdapter()
5. DataBroker starts the adapter
6. QML binds to DataBroker properties
7. Data flows: Adapter → DataBroker → QML
```

## File Organization

```
src/
├── core/                  # Interfaces, DataBroker
├── adapters/              # Protocol implementations
│   ├── haltech/
│   ├── simulator/
│   └── [future: obd2/]
├── cluster/               # Instrument cluster QML
├── headunit/              # Head unit QML
└── main.cpp               # Application entry point
```

## Next Steps

- **Understand adapters:** [Protocol Adapters](protocol-adapters.md)
- **Understand data flow:** [DataBroker](data-broker.md)
- **Add your own adapter:** [Adding Protocol Adapters](../03-contributing/adding-adapters.md)
