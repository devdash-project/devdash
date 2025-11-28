# Vehicle Profiles

**Vehicle Profiles** are JSON configuration files that define how /dev/dash behaves for a specific vehicle. They specify which protocol adapter to use, how gauges are laid out, alert thresholds, camera configuration, and user preferences.

## Purpose

Vehicle profiles enable **configuration without code changes**. You can switch between:
- Different ECUs (Haltech, OBD-II, MegaSquirt)
- Different gauge layouts (racing, cruising, minimal)
- Different vehicles (each with its own profile)

**...all without recompiling the application.**

## Profile Location

Profiles live in the `profiles/` directory:

```
profiles/
├── example-simulator.json    # Mock data for development
├── example-haltech.json       # Haltech ECU example
├── example-obd2.json          # OBD-II example (future)
└── my-custom-vehicle.json     # Your custom profile
```

**Loading a profile:**

```bash
./build/dev/devdash --profile profiles/my-custom-vehicle.json
```

## Profile Structure

A complete vehicle profile has these sections:

```json
{
  "name": "Vehicle Name",
  "description": "Brief description",
  "protocol": { /* adapter configuration */ },
  "cluster": { /* instrument cluster layout */ },
  "headunit": { /* head unit configuration */ },
  "alerts": { /* warning/critical thresholds */ },
  "cameras": [ /* camera sources */ ],
  "preferences": { /* user preferences */ }
}
```

## Complete Example: Moon Patrol

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
    "theme": "night",
    "gauges": [
      {
        "type": "tachometer",
        "channel": "rpm",
        "position": "primary",
        "min": 0,
        "max": 8000,
        "redline": 7000
      },
      {
        "type": "digital",
        "channel": "coolantTemperature",
        "position": "aux1",
        "label": "Coolant"
      },
      {
        "type": "digital",
        "channel": "oilTemperature",
        "position": "aux2",
        "label": "Oil Temp"
      },
      {
        "type": "bar",
        "channel": "oilPressure",
        "position": "aux3",
        "min": 0,
        "max": 100
      }
    ],
    "shiftLight": {
      "enabled": true,
      "channel": "rpm",
      "thresholds": [5500, 6000, 6500, 7000]
    }
  },

  "headunit": {
    "defaultView": "cameras",
    "enabledViews": ["cameras", "audio", "settings"]
  },

  "alerts": {
    "coolantTemperature": {
      "warning": 105,
      "critical": 115,
      "unit": "C"
    },
    "oilPressure": {
      "warning_low": 15,
      "critical_low": 10,
      "unit": "psi"
    },
    "batteryVoltage": {
      "warning_low": 12.0,
      "critical_low": 11.5,
      "unit": "V"
    }
  },

  "cameras": [
    {
      "id": "front",
      "label": "Front Camera",
      "source": "/dev/video0",
      "enabled": true
    },
    {
      "id": "rear",
      "label": "Backup Camera",
      "source": "/dev/video1",
      "enabled": true,
      "reverseGear": true
    }
  ],

  "preferences": {
    "units": "imperial",
    "theme": "night",
    "brightness": 80
  }
}
```

## Profile Sections

### 1. Metadata

```json
{
  "name": "Moon Patrol",
  "description": "Custom rock crawler with Haltech Elite 2500"
}
```

- **name** (required): Display name for the vehicle
- **description** (optional): Brief description

### 2. Protocol Configuration

Specifies which adapter to use and how to configure it:

```json
{
  "protocol": {
    "adapter": "haltech",  // or "obd2", "simulator", etc.
    "config": {
      // Adapter-specific configuration
      "interface": "can0",
      "bitrate": 1000000,
      "protocol_version": "2.35"
    }
  }
}
```

#### Haltech Adapter Config

```json
{
  "protocol": {
    "adapter": "haltech",
    "config": {
      "interface": "can0",       // CAN interface name
      "bitrate": 1000000,         // 1 Mbit/s (Haltech default)
      "protocol_version": "2.35"  // Protocol version
    }
  }
}
```

#### Simulator Adapter Config

```json
{
  "protocol": {
    "adapter": "simulator",
    "config": {
      "update_rate": 20,     // Hz
      "scenario": "idle"     // "idle", "cruising", "racing", "redline"
    }
  }
}
```

#### OBD-II Adapter Config (Future)

```json
{
  "protocol": {
    "adapter": "obd2",
    "config": {
      "interface": "can0",
      "protocol": "ISO15765"  // CAN (11-bit 500k)
    }
  }
}
```

### 3. Cluster Configuration

Defines the instrument cluster layout and gauges:

```json
{
  "cluster": {
    "layout": "racing",       // "racing", "cruising", "minimal"
    "theme": "night",         // "day", "night", "custom"
    "gauges": [ /* gauge array */ ],
    "shiftLight": { /* shift light config */ }
  }
}
```

#### Gauge Types

##### Tachometer (Analog Sweep)

```json
{
  "type": "tachometer",
  "channel": "rpm",
  "position": "primary",
  "min": 0,
  "max": 8000,
  "redline": 7000,
  "majorTicks": 1000,
  "minorTicks": 500
}
```

##### Speedometer (Analog Sweep)

```json
{
  "type": "speedometer",
  "channel": "vehicleSpeed",
  "position": "primary",
  "min": 0,
  "max": 200,
  "unit": "mph"
}
```

##### Digital Gauge

```json
{
  "type": "digital",
  "channel": "coolantTemperature",
  "position": "aux1",
  "label": "Coolant",
  "precision": 1,     // Decimal places
  "unit": "°C"
}
```

##### Bar Gauge (Horizontal Bar)

```json
{
  "type": "bar",
  "channel": "oilPressure",
  "position": "aux2",
  "min": 0,
  "max": 100,
  "unit": "psi",
  "orientation": "horizontal"  // or "vertical"
}
```

##### Sweep Gauge (Arc)

```json
{
  "type": "sweep",
  "channel": "throttlePosition",
  "position": "aux3",
  "min": 0,
  "max": 100,
  "unit": "%",
  "arc": 180  // Sweep arc in degrees
}
```

#### Gauge Positions

Positions depend on the layout:

**Racing Layout:**
- `primary` - Center, large (tachometer or speedometer)
- `aux1`, `aux2`, `aux3`, `aux4` - Corners or sides

**Cruising Layout:**
- `left` - Left side (speedometer)
- `right` - Right side (tachometer)
- `bottom1`, `bottom2`, `bottom3` - Bottom row

**Minimal Layout:**
- `center` - Single large gauge
- `status` - Small status gauges

#### Shift Light Configuration

```json
{
  "shiftLight": {
    "enabled": true,
    "channel": "rpm",
    "thresholds": [5500, 6000, 6500, 7000],
    "colors": ["yellow", "yellow", "orange", "red"]
  }
}
```

Progressive shift light that changes color as RPM increases:
- 5500 RPM: Yellow (start flashing)
- 6000 RPM: Yellow (faster flashing)
- 6500 RPM: Orange (very fast)
- 7000 RPM: Red (solid)

### 4. Head Unit Configuration

```json
{
  "headunit": {
    "defaultView": "cameras",
    "enabledViews": ["cameras", "audio", "emulation", "settings"]
  }
}
```

- **defaultView**: View shown on startup (`cameras`, `audio`, `emulation`, `settings`)
- **enabledViews**: Which views are available (array)

### 5. Alert Thresholds

Define warning and critical thresholds for channels:

```json
{
  "alerts": {
    "coolantTemperature": {
      "warning": 105,     // Yellow warning at 105°C
      "critical": 115,    // Red critical at 115°C
      "unit": "C"
    },
    "oilPressure": {
      "warning_low": 15,  // Yellow warning below 15 psi
      "critical_low": 10, // Red critical below 10 psi
      "unit": "psi"
    },
    "batteryVoltage": {
      "warning_low": 12.0,
      "critical_low": 11.5,
      "unit": "V"
    },
    "manifoldPressure": {
      "warning_high": 30,  // Overboost warning
      "critical_high": 35,
      "unit": "psi"
    }
  }
}
```

**Alert types:**
- `warning` / `critical` - High thresholds (e.g., temperature too high)
- `warning_low` / `critical_low` - Low thresholds (e.g., pressure too low)
- `warning_high` / `critical_high` - High thresholds (explicit)

### 6. Camera Configuration

```json
{
  "cameras": [
    {
      "id": "front",
      "label": "Front Camera",
      "source": "/dev/video0",
      "enabled": true,
      "gstreamer_pipeline": "v4l2src device=/dev/video0 ! video/x-raw,width=1280,height=720 ! videoconvert"
    },
    {
      "id": "rear",
      "label": "Backup Camera",
      "source": "/dev/video1",
      "enabled": true,
      "reverseGear": true,    // Auto-show when in reverse
      "guideLines": true      // Show parking guide lines
    },
    {
      "id": "cabin",
      "label": "Cabin Camera",
      "source": "/dev/video2",
      "enabled": false
    }
  ]
}
```

**Camera properties:**
- **id** (required): Unique identifier
- **label** (required): Display name
- **source** (required): Video device path
- **enabled**: Enable/disable camera
- **reverseGear**: Auto-show when gear == -1 (reverse)
- **guideLines**: Show parking guide overlay
- **gstreamer_pipeline**: Custom GStreamer pipeline (optional)

### 7. User Preferences

```json
{
  "preferences": {
    "units": "imperial",       // "imperial" or "metric"
    "theme": "night",          // "day", "night", "auto"
    "brightness": 80,          // 0-100
    "volume": 50,              // 0-100
    "language": "en_US"
  }
}
```

## Example Profiles

### Minimal Profile (Simulator)

```json
{
  "name": "Simulator",
  "protocol": {
    "adapter": "simulator"
  },
  "cluster": {
    "layout": "racing",
    "gauges": [
      {"type": "tachometer", "channel": "rpm", "position": "primary", "max": 8000}
    ]
  }
}
```

### Racing Profile (Haltech)

```json
{
  "name": "Track Day",
  "protocol": {
    "adapter": "haltech",
    "config": {
      "interface": "can0",
      "bitrate": 1000000
    }
  },
  "cluster": {
    "layout": "racing",
    "gauges": [
      {"type": "tachometer", "channel": "rpm", "position": "primary", "max": 9000},
      {"type": "digital", "channel": "oilPressure", "position": "aux1"},
      {"type": "digital", "channel": "oilTemperature", "position": "aux2"},
      {"type": "digital", "channel": "coolantTemperature", "position": "aux3"},
      {"type": "bar", "channel": "throttlePosition", "position": "aux4"}
    ],
    "shiftLight": {
      "enabled": true,
      "channel": "rpm",
      "thresholds": [7500, 8000, 8500, 9000]
    }
  },
  "alerts": {
    "oilPressure": {"critical_low": 10, "unit": "psi"},
    "coolantTemperature": {"critical": 110, "unit": "C"}
  },
  "preferences": {
    "units": "imperial",
    "theme": "night"
  }
}
```

### Daily Driver Profile (OBD-II, Future)

```json
{
  "name": "Daily Driver",
  "protocol": {
    "adapter": "obd2",
    "config": {
      "interface": "can0",
      "protocol": "ISO15765"
    }
  },
  "cluster": {
    "layout": "cruising",
    "gauges": [
      {"type": "speedometer", "channel": "vehicleSpeed", "position": "left", "max": 140},
      {"type": "tachometer", "channel": "rpm", "position": "right", "max": 7000},
      {"type": "digital", "channel": "coolantTemperature", "position": "bottom1"},
      {"type": "digital", "channel": "fuelLevel", "position": "bottom2"},
      {"type": "digital", "channel": "batteryVoltage", "position": "bottom3"}
    ]
  },
  "alerts": {
    "coolantTemperature": {"warning": 100, "critical": 110, "unit": "C"},
    "batteryVoltage": {"warning_low": 12.0, "critical_low": 11.5, "unit": "V"}
  },
  "cameras": [
    {"id": "rear", "label": "Backup", "source": "/dev/video0", "reverseGear": true}
  ],
  "preferences": {
    "units": "imperial",
    "theme": "auto"
  }
}
```

## Loading Profiles

### Command Line

```bash
# Load specific profile
./build/dev/devdash --profile profiles/moon-patrol.json

# Default profile (if --profile not specified)
./build/dev/devdash  # Looks for profiles/default.json
```

### Programmatically

```cpp
#include "core/VehicleProfile.h"

VehicleProfile profile;
if (!profile.load("profiles/moon-patrol.json")) {
    qCritical() << "Failed to load profile:" << profile.error();
    return 1;
}

// Access profile data
QString adapterType = profile.adapterType();  // "haltech"
QJsonObject adapterConfig = profile.adapterConfig();
QJsonArray gauges = profile.clusterGauges();
```

## Profile Validation

The `VehicleProfile` class validates profiles on load:

```cpp
VehicleProfile profile;
if (!profile.load("profiles/invalid.json")) {
    qWarning() << "Validation errors:";
    for (const QString& error : profile.validationErrors()) {
        qWarning() << " -" << error;
    }
}
```

**Common validation errors:**
- Missing required fields (`protocol.adapter`)
- Invalid adapter type (not registered in factory)
- Invalid gauge types
- Invalid channel names (adapter doesn't provide that channel)
- Malformed JSON

## Hot Reloading (Future Feature)

Future versions may support hot-reloading profiles without restarting:

```cpp
// Watch profile file for changes
profile.setAutoReload(true);
connect(&profile, &VehicleProfile::changed, [&]() {
    qInfo() << "Profile changed, reloading...";
    dataBroker.reloadGauges();
});
```

## Best Practices

### ✅ Do

- **Use descriptive names**: `"Moon Patrol - Track Config"` not `"config1"`
- **Document custom configurations**: Add `description` field explaining purpose
- **Start with an example**: Copy `example-haltech.json` and modify
- **Test with simulator first**: Use `"adapter": "simulator"` to test layouts before connecting real hardware
- **Version control your profiles**: Check profiles into git
- **Validate before deploying**: Run with `--validate-profile` flag

### ❌ Don't

- **Don't hardcode paths**: Use relative paths (`profiles/my-vehicle.json`)
- **Don't duplicate profiles**: Use one profile per vehicle, modify as needed
- **Don't omit units**: Always specify `"unit"` in alerts for clarity
- **Don't use undefined channels**: Check adapter's `availableChannels()` first

## Troubleshooting

### Profile won't load

**Error**: `Failed to load profile: Unexpected token at offset X`

**Cause**: Invalid JSON syntax.

**Solution**: Validate JSON with `jsonlint` or online validator:

```bash
jsonlint profiles/my-vehicle.json
```

### Adapter not found

**Error**: `Unknown adapter type: "myecu"`

**Cause**: Adapter type not registered in `ProtocolAdapterFactory`.

**Solution**: Check available adapters:

```bash
./build/dev/devdash --list-adapters
# Available adapters: haltech, simulator
```

### Channel not found

**Error**: `Channel "turboBoost" not available from adapter`

**Cause**: Adapter doesn't provide that channel.

**Solution**: List available channels:

```bash
./build/dev/devdash --profile profiles/my-vehicle.json --list-channels
# Available channels: rpm, coolantTemperature, oilPressure, ...
```

### Gauges don't appear

**Cause**: Position doesn't exist in layout.

**Solution**: Check position names for your layout. `"racing"` layout uses `"primary"`, `"aux1"`, etc., not `"left"` or `"right"`.

## Next Steps

- **Understand adapters**: [Protocol Adapters](protocol-adapters.md)
- **Create your own profile**: Copy an example and customize
- **Contribute example profiles**: Share your vehicle configuration
