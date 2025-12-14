# Testing with Mock CAN Data

## Overview

The `haltech-mock` Python package provides realistic Haltech ECU and PD16 Power Distribution Module simulation for testing devdash without a running engine or real hardware. It includes:

- **9 ECU scenarios** for engine simulation (idle, track, warnings, etc.)
- **5 PD16 scenarios** for power distribution testing
- **167 CAN channels** covering all Haltech V2.35 protocol data
- **Physics-based simulation** for realistic engine behavior
- **Custom JSON scenarios** for specific test cases

## Quick Start

```bash
# One-liner to start with idle scenario
./scripts/run-with-mock idle

# Test track lap scenario (looping)
./scripts/run-with-mock track --loop

# Test warning conditions
./scripts/run-with-mock overheat

# Test PD16 power distribution
./scripts/run-with-pd16 lights
```

## Requirements

- Virtual CAN interface (`vcan0`)
- haltech-mock installed (`/opt/haltech-venv/bin/haltech-mock`)

### Setting up vcan0

#### On Host System

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
```

#### In Dev Container

The dev container cannot create vcan interfaces directly. Create `vcan0` on the host first, then it will be accessible inside the container.

```bash
# On host machine:
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# Inside container:
./scripts/setup-vcan.sh  # Verifies vcan0 exists
```

## ECU Scenarios

### Available Scenarios

| Scenario | Description | Duration | Use Case |
|----------|-------------|----------|----------|
| `idle` | Engine idling with RPM ~800-900 | 30s | Basic testing, gauge check |
| `accel` | Smooth acceleration 0→8000 RPM | 5s | Throttle response testing |
| `warmup` | Cold start → operating temp | 120s | Temperature gauge testing |
| `track` | Single track lap | 90s | Full dynamics, gear changes |
| `track-session` | Multi-lap session (loops) | 90s/lap | Extended testing |
| `overheat` | Engine overheating condition | 30s | Warning light testing |
| `low-oil` | Low oil pressure condition | 20s | Warning light testing |
| `redline` | Rev limiter activation | 15s | Limiter indicator testing |
| `battery` | Alternator failure | 20s | Electrical system warnings |

### Running ECU Scenarios

```bash
# Basic usage
./scripts/run-with-mock <scenario>

# With looping (runs continuously)
./scripts/run-with-mock <scenario> --loop

# With custom duration
./scripts/run-with-mock <scenario> --duration 120

# Examples
./scripts/run-with-mock idle                  # 30s idle
./scripts/run-with-mock track --loop          # Continuous laps
./scripts/run-with-mock overheat --duration 60  # 60s overheat test
```

## PD16 Power Distribution Scenarios

### Available PD16 Scenarios

| Scenario | Description | Outputs Used |
|----------|-------------|--------------|
| `lights` | Parking → low beam → high beam → turn signals | 8A: 0-4 |
| `cooling` | Temperature-based fan control (0% → 30% → 80%) | 25A: 0 |
| `fuel-pump` | Priming sequence → steady operation | 25A: 1 |
| `fault` | Cycle through fault conditions (overcurrent, short, open) | 8A: 5 |
| `startup` | Boot sequence with output self-test | Multiple |

### Running PD16 Scenarios

```bash
# Basic usage
./scripts/run-with-pd16 <scenario>

# With device ID (A, B, C, or D)
./scripts/run-with-pd16 <scenario> --device B

# Examples
./scripts/run-with-pd16 lights            # Headlight cycling
./scripts/run-with-pd16 cooling           # Fan duty ramping
./scripts/run-with-pd16 fault --device A  # Fault injection testing
```

Note: PD16 scenarios run in loop mode by default for continuous testing.

## Physics-Based Simulation

For interactive testing, use physics mode to manually control throttle and gear:

```bash
# Start in physics mode (manual control)
/opt/haltech-venv/bin/haltech-mock run --physics --interface vcan0 &
./build/dev/devdash --profile profiles/haltech-vcan.json

# With performance preset
/opt/haltech-venv/bin/haltech-mock run --physics --performance sport --interface vcan0 &
```

**Note**: Physics mode requires manual throttle/gear control via the haltech-mock API (Python integration).

## Custom JSON Scenarios

Create custom test sequences using JSON files.

### Example: Custom Warning Test

Create `custom-warning.json`:

```json
{
  "name": "Dashboard Warning Test",
  "description": "Test all warning lights sequentially",
  "stages": [
    {
      "name": "Normal Operation",
      "duration": 5.0,
      "channels": {
        "RPM": {"range": [800, 900], "variation": "sine", "period": 2.0},
        "Engine Coolant Temperature": {"value": 358.15},
        "Oil Pressure": {"value": 350},
        "Battery Voltage": {"value": 14.2}
      }
    },
    {
      "name": "Oil Pressure Warning",
      "duration": 5.0,
      "channels": {
        "RPM": {"value": 850},
        "Oil Pressure": {"value": 80}
      }
    },
    {
      "name": "Overheating",
      "duration": 5.0,
      "channels": {
        "RPM": {"value": 850},
        "Engine Coolant Temperature": {"value": 393.15}
      }
    }
  ]
}
```

Run it:

```bash
/opt/haltech-venv/bin/haltech-mock scenario custom-warning.json --interface vcan0 --loop &
./build/dev/devdash --profile profiles/haltech-vcan.json
```

### JSON Scenario Format

```json
{
  "name": "Scenario Name",
  "description": "Description",
  "loop": false,
  "stages": [
    {
      "name": "Stage 1",
      "duration": 10.0,
      "channels": {
        "RPM": 2500,
        "Oil Pressure": {"range": [300, 400], "variation": "noise"},
        "Throttle Position": {"value": 50}
      },
      "transitions": {
        "RPM": {"from": 1000, "to": 7000, "curve": "ease_in"}
      }
    }
  ]
}
```

**Channel value options:**
- `"RPM": 3500` - Static value
- `{"value": 3500}` - Explicit static value
- `{"range": [300, 400], "variation": "sine"}` - Oscillating value
- `{"range": [0.95, 1.05], "variation": "random_walk"}` - Random variation

**Transition curves:** `linear`, `exponential`, `ease_in`, `ease_out`, `ease_in_out`

## Monitoring CAN Traffic

Use `candump` to monitor raw CAN frames:

```bash
# Terminal 1: Start haltech-mock
/opt/haltech-venv/bin/haltech-mock scenario idle --interface vcan0 --loop

# Terminal 2: Monitor traffic
candump vcan0

# Terminal 3: Run devdash
./build/dev/devdash --profile profiles/haltech-vcan.json
```

### Inspecting Channels and Frames

```bash
# List all 167 available channels
/opt/haltech-venv/bin/haltech-mock channels

# Filter channels by name
/opt/haltech-venv/bin/haltech-mock channels temp

# List all CAN frames
/opt/haltech-venv/bin/haltech-mock frames

# Show details for specific frame
/opt/haltech-venv/bin/haltech-mock frames 0x360

# Decode a raw CAN frame
/opt/haltech-venv/bin/haltech-mock decode 0x360 0DAC03E800000000
```

## Troubleshooting

### vcan0 not found

**Problem**: `Error: vcan0 not found`

**Solution**:
```bash
# Create vcan0 (on host if using dev container)
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up

# Verify
ip link show vcan0
```

### haltech-mock not found

**Problem**: `Error: haltech-mock not found`

**Solution**:
```bash
# Check installation
/opt/haltech-venv/bin/haltech-mock --version

# If not installed
pip install haltech-mock
```

### No data on dashboard

**Problem**: Dashboard shows no data or zeros

**Checklist**:
1. Is vcan0 up? `ip link show vcan0 | grep UP`
2. Is haltech-mock running? `ps aux | grep haltech-mock`
3. Is CAN traffic present? `candump vcan0`
4. Is profile correct? Check `profiles/haltech-vcan.json`
5. Are channel mappings correct? Check `channelMapping` in profile

### Permission denied on vcan0

**Problem**: `Permission denied` when creating vcan0

**Solution**:
```bash
# Add user to netdev group (if exists)
sudo usermod -aG netdev $USER

# Or run with sudo
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
```

## Advanced Usage

### Combining ECU + PD16

Run both ECU and PD16 scenarios simultaneously:

```bash
# Terminal 1: ECU simulation
/opt/haltech-venv/bin/haltech-mock scenario track --interface vcan0 --loop &

# Terminal 2: PD16 simulation
/opt/haltech-venv/bin/haltech-mock pd16 scenario lights --interface vcan0 --loop &

# Terminal 3: Run devdash
./build/dev/devdash --profile profiles/haltech-vcan.json
```

### Scripted Testing

Create test scripts that cycle through scenarios:

```bash
#!/bin/bash
# test-warnings.sh - Test all warning conditions

for scenario in overheat low-oil battery; do
    echo "Testing $scenario..."
    ./scripts/run-with-mock $scenario --duration 30
    sleep 2
done
```

## Best Practices

1. **Start with idle**: Always test basic connectivity with `idle` scenario first
2. **Use looping for development**: `--loop` flag is useful for iterative UI work
3. **Monitor with candump**: Keep `candump vcan0` running to verify data flow
4. **Test warnings separately**: Run warning scenarios individually to verify each condition
5. **Custom scenarios for bugs**: Create JSON scenarios to reproduce specific issues
6. **Check channel names**: Use `/opt/haltech-venv/bin/haltech-mock channels` to verify exact channel names

## See Also

- [haltech-mock GitHub](https://github.com/devdash-project/haltech-mock)
- [haltech-mock PyPI](https://pypi.org/project/devdash-haltech-mock/)
- [Haltech CAN Protocol V2.35](https://www.haltech.com/support/can-protocol/)
- `./scripts/list-scenarios` - Quick reference of all scenarios
