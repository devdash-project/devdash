# Developer Guide: Adding Gauges

## Quick Start

Adding a new gauge = creating one QML file. No C++ changes needed.

### Step 1: Create QML File

Create `src/cluster/qml/gauges/YourGauge.qml`:

```qml
import QtQuick
import QtQuick.Shapes

BaseGauge {
    id: root

    // Your rendering here
    Text {
        text: Math.round(root.value)
        font.pixelSize: 48
        color: root.primaryColor
    }
}
```

### Step 2: Use in Profile

```json
{
  "layout": {
    "gauges": [
      {
        "type": "YourGauge",
        "channel": "coolantTemp",
        "position": [100, 100],
        "size": [200, 200]
      }
    ]
  }
}
```

That's it! The layout system auto-discovers your gauge.

## BaseGauge Properties

All gauges extend `BaseGauge` and get these for free:

| Property | Type | Description |
|----------|------|-------------|
| `channel` | string | Channel name (e.g., "rpm") |
| `value` | real | Current value from DataBroker |
| `config` | object | Custom config from profile |
| `theme` | object | Current theme |
| `primaryColor` | color | Theme primary color |
| `warningColor` | color | Warning threshold color |
| `criticalColor` | color | Critical threshold color |

## Example: Inclinometer Gauge

```qml
import QtQuick
import QtQuick.Shapes

BaseGauge {
    id: root

    // Config example
    property real maxAngle: config.maxAngle ?? 45

    Shape {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width

        // Horizon line rotates with pitch angle
        ShapePath {
            strokeWidth: 3
            strokeColor: root.primaryColor

            PathLine {
                x: 0
                y: root.height / 2
            }
            PathLine {
                x: root.width
                y: root.height / 2
            }
        }

        transform: Rotation {
            origin.x: root.width / 2
            origin.y: root.height / 2
            angle: -root.value  // Negative for correct orientation
        }
    }

    // Angle readout
    Text {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: Math.round(root.value) + "°"
        font.pixelSize: 24
        color: root.primaryColor
    }
}
```

### Usage

```json
{
  "type": "Inclinometer",
  "channel": "pitchAngle",
  "config": {"maxAngle": 60}
}
```

## Rendering Options

- **Qt Quick Shapes**: Vector graphics (arcs, paths)
- **Canvas**: 2D drawing API for complex shapes
- **SVG**: Import SVG files
- **Shaders**: OpenGL shaders for effects

## See Also

- [Layout System](layout-system.md) - How gauges are loaded
- [Channel Namespace](../../reference/channel-namespace.md) - Available channels
