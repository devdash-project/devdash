# Gauge Cluster Layout System

## Overview

Profile-driven layout system that allows:
- JSON-configured gauge placement
- Multiple positioning modes (absolute, percentage, anchor)
- Runtime layout switching from head unit
- Developer extensibility (drop-in QML gauges)

## Quick Start

### 1. Define Layout in Profile

```json
{
  "layout": {
    "designResolution": [1920, 720],
    "scaleMode": "fit",
    "gauges": [
      {
        "id": "tachometer",
        "type": "Tachometer",
        "channel": "rpm",
        "anchor": "center-left",
        "offset": [50, 0],
        "size": ["30%", "80%"],
        "config": {
          "maxValue": 8000,
          "redlineStart": 6500
        }
      },
      {
        "id": "gear",
        "type": "FlipClock",
        "channel": "gear",
        "anchor": "center",
        "size": [200, 150]
      }
    ]
  }
}
```

### 2. Gauge Auto-Loads from QML

`GaugeLayout.qml` uses `Repeater` + `Loader` to instantiate gauges from the profile.

## Positioning Modes

### Absolute (Pixels)
```json
{"position": [100, 50], "size": [400, 400]}
```

### Percentage-Based
```json
{"position": ["10%", "5%"], "size": ["25%", "50%"]}
```

### Anchor-Based
```json
{
  "anchor": "center",
  "offset": [0, -50],
  "size": [400, 400]
}
```

**Anchors**: `top-left`, `top-center`, `top-right`, `center-left`, `center`, `center-right`, `bottom-left`, `bottom-center`, `bottom-right`

## Scale Modes

- `fit` - Scale uniformly, maintain aspect ratio (letterbox)
- `fill` - Scale to fill viewport (may crop)
- `stretch` - Stretch to fill (distorts aspect)
- `none` - No scaling, use absolute positions

## Runtime Layout Switching

Head unit can change cluster layout:

```cpp
layoutManager->setActiveLayout("track");  // Switches to track preset
```

```json
{
  "layoutPresets": {
    "street": { "gauges": [/*...*/] },
    "track": { "gauges": [/*...*/] },
    "rock-crawler": { "gauges": [/*...*/] }
  },
  "defaultLayout": "street"
}
```

## Architecture

```
LayoutManager (C++)
  ↓ Parses profile JSON
  ↓ Exposes gauges list to QML
  ↓
GaugeLayout.qml
  ↓ Repeater over gauges
  ↓ Loader dynamically loads gauge QML
  ↓
BaseGauge.qml
  ↑ All gauges extend this
  ↑ Provides channel binding, theme
```

## See Also

- [Adding Gauges](adding-gauges.md) - How to create new gauge types
- [Channel Namespace](../../reference/channel-namespace.md) - Available channels
