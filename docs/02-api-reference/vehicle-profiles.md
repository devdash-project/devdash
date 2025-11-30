# Vehicle Profiles

Vehicle profiles are JSON configuration files that define how DevDash interprets data from different ECUs and adapts to specific vehicle configurations.

## Profile Location

Profiles are stored in `profiles/*.json` and selected at runtime via the `--profile` flag:

```bash
./devdash --profile profiles/haltech-vcan.json
```

## Profile Structure

```json
{
    "$schema": "../protocols/profile-schema.json",
    "name": "My Vehicle Profile",
    "description": "Profile description",
    "adapter": "haltech",
    "adapterConfig": {
        "interface": "can0",
        "protocolFile": "../protocols/haltech/haltech-can-protocol-v2.35.json"
    },
    "display": { ... },
    "units": { ... },
    "warnings": { ... },
    "channelMappings": { ... },
    "gearMapping": { ... }
}
```

## Channel Mappings

Maps protocol-specific channel names to DataBroker properties:

```json
"channelMappings": {
    "RPM": "rpm",
    "TPS": "throttlePosition",
    "ECT": "coolantTemperature"
}
```

**Format:** `"Protocol Channel Name": "standardPropertyName"`

- **Key (left side):** Channel name as sent by the protocol adapter
- **Value (right side):** DataBroker property name (camelCase)

### Available Standard Properties

| Property Name | Type | Description | Unit |
|---------------|------|-------------|------|
| `rpm` | double | Engine RPM | RPM |
| `throttlePosition` | double | Throttle position | % (0-100) |
| `manifoldPressure` | double | Manifold absolute pressure | kPa |
| `coolantTemperature` | double | Engine coolant temperature | °C |
| `oilTemperature` | double | Engine oil temperature | °C |
| `intakeAirTemperature` | double | Intake air temperature | °C |
| `oilPressure` | double | Engine oil pressure | kPa |
| `fuelPressure` | double | Fuel rail pressure | kPa |
| `fuelLevel` | double | Fuel level | % (0-100) |
| `airFuelRatio` | double | Air/fuel ratio | ratio |
| `batteryVoltage` | double | Battery/system voltage | V |
| `vehicleSpeed` | double | Vehicle speed | km/h |
| `gear` | QString | Current gear | string |

## Gear Mapping

**NEW in v0.2.0** - Defines how numeric gear values from the protocol are displayed as strings.

### Why Gear Mapping?

Different transmissions use different gear representations:
- **Manual:** R, N, 1, 2, 3, 4, 5, 6
- **Automatic:** P, R, N, D, S, L, M
- **Sequential:** R, N, 1, 2, 3, 4, 5, 6
- **CVT:** P, R, N, D, S, L

Gear mapping allows you to customize gear display for your specific transmission without code changes.

### Manual Transmission Example

```json
"gearMapping": {
    "-1": "R",
    "0": "N",
    "1": "1",
    "2": "2",
    "3": "3",
    "4": "4",
    "5": "5",
    "6": "6"
}
```

**Protocol sends:** `0.0` → **Display shows:** "N"
**Protocol sends:** `3.0` → **Display shows:** "3"

### Automatic Transmission Example

```json
"gearMapping": {
    "-2": "P",
    "-1": "R",
    "0": "N",
    "1": "D",
    "2": "S",
    "3": "L"
}
```

**Protocol sends:** `-2.0` → **Display shows:** "P"
**Protocol sends:** `1.0` → **Display shows:** "D"

### Sequential/Tiptronic Example

```json
"gearMapping": {
    "-1": "R",
    "0": "N",
    "1": "D",
    "2": "S1",
    "3": "S2",
    "4": "S3",
    "5": "S4",
    "6": "S5",
    "7": "S6"
}
```

### CVT with Drive Modes

```json
"gearMapping": {
    "-2": "P",
    "-1": "R",
    "0": "N",
    "1": "D",
    "2": "E",
    "3": "S"
}
```

### Format Rules

- **Keys:** Numeric strings representing the value from the protocol
- **Values:** Non-empty strings to display (typically 1-2 characters)
- **Optional:** If not specified, defaults to manual transmission (R, N, 1-6)

### How It Works

1. Protocol adapter sends gear as `double` value (e.g., `3.0`)
2. DataBroker converts to `int` (e.g., `3`)
3. Looks up in `gearMapping` (e.g., `"3": "3"`)
4. Updates `QString gear` property with mapped value (e.g., `"3"`)
5. QML displays the string (e.g., "3")

### Default Behavior

If `gearMapping` is not specified in the profile, DevDash uses a default 6-speed manual transmission mapping:

```json
{
    "-1": "R",
    "0": "N",
    "1": "1",
    "2": "2",
    "3": "3",
    "4": "4",
    "5": "5",
    "6": "6"
}
```

## Profile Examples

- **`haltech-vcan.json`** - Manual transmission with Haltech ECU on vcan0
- **`example-haltech.json`** - Manual transmission example
- **`example-automatic.json`** - Automatic transmission example (P/R/N/D/S/L)

## Creating a New Profile

### Step 1: Copy an Example

```bash
cp profiles/example-haltech.json profiles/my-vehicle.json
```

### Step 2: Update Metadata

```json
{
    "name": "My 2024 Civic Type R",
    "description": "Honda FK8 with Haltech Elite 2500",
    ...
}
```

### Step 3: Configure Adapter

```json
"adapter": "haltech",
"adapterConfig": {
    "interface": "can0",
    "protocolFile": "../protocols/haltech/haltech-can-protocol-v2.35.json"
}
```

### Step 4: Map Channels

Refer to your ECU's protocol documentation to find channel names, then map to standard properties:

```json
"channelMappings": {
    "RPM": "rpm",
    "TPS": "throttlePosition",
    ...
}
```

### Step 5: Configure Gear Mapping

Choose the mapping that matches your transmission:

**6-speed manual:**
```json
"gearMapping": {
    "-1": "R",
    "0": "N",
    "1": "1",
    "2": "2",
    "3": "3",
    "4": "4",
    "5": "5",
    "6": "6"
}
```

**Automatic with sport mode:**
```json
"gearMapping": {
    "-2": "P",
    "-1": "R",
    "0": "N",
    "1": "D",
    "2": "S"
}
```

### Step 6: Test Your Profile

```bash
./devdash --profile profiles/my-vehicle.json
```

Watch the logs for any mapping warnings:
```
DataBroker: Loaded 12 channel mappings
DataBroker: Loaded 8 gear mappings
```

## Troubleshooting

### Gear Shows "N" for All Values

**Problem:** Gear values from protocol don't match your `gearMapping` keys.

**Solution:** Check protocol output and adjust mapping:
```bash
# Monitor CAN bus to see what values your ECU sends
candump can0 | grep gear
```

### Channel Not Updating

**Problem:** Channel name mismatch between protocol and profile.

**Solution:** Enable debug logging to see protocol channel names:
```bash
QT_LOGGING_RULES="*.debug=true" ./devdash --profile profiles/my-vehicle.json
```

Look for lines like:
```
DataBroker: Mapped RPM -> rpm
HaltechAdapter: Decoded channel: RPM = 3500.0
```

### Unknown Property Name Warning

**Problem:** Typo in channel mapping value.

**Solution:** Check spelling against the table above. Property names are case-sensitive:
- ✅ `"rpm"` (lowercase)
- ❌ `"RPM"` (uppercase - this is the protocol name)
- ❌ `"Rpm"` (wrong case)

## See Also

- [DataBroker API Reference](./data-broker-api.md)
- [Protocol Adapter Development](../03-contributing/adding-adapters.md)
- [Channel System Architecture](../01-architecture/data-broker.md)
