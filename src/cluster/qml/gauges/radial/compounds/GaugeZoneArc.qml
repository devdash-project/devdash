import QtQuick
import QtQuick.Shapes

/**
 * @brief Colored zone arc for warning/redline regions.
 *
 * GaugeZoneArc displays a static colored arc segment representing a zone
 * on the gauge (e.g., redline zone from 6500-8000 RPM).
 *
 * The arc is calculated from startValue to endValue within the gauge's range,
 * and can be styled with any color and opacity.
 *
 * @example
 * @code
 * GaugeZoneArc {
 *     minValue: 0
 *     maxValue: 8000
 *     startValue: 6500    // Redline starts at 6500
 *     endValue: 8000      // Redline ends at max
 *     zoneColor: "#aa2222"
 *     zoneOpacity: 0.3
 * }
 * @endcode
 */
Item {
    id: root

    // === Value Properties ===

    /**
     * @brief Minimum value of the gauge range.
     * @default 0
     */
    property real minValue: 0

    /**
     * @brief Maximum value of the gauge range.
     * @default 100
     */
    property real maxValue: 100

    /**
     * @brief Value where this zone starts.
     * @default minValue
     */
    property real startValue: minValue

    /**
     * @brief Value where this zone ends.
     * @default maxValue
     */
    property real endValue: maxValue

    // === Geometry Properties ===

    /**
     * @brief Starting angle of the gauge in degrees (0 = 3 o'clock).
     * @default -225 (typical gauge starting position)
     */
    property real gaugeStartAngle: -225

    /**
     * @brief Total sweep angle of the gauge in degrees.
     * @default 270 (three-quarter circle)
     */
    property real gaugeTotalSweep: 270

    /**
     * @brief Arc radius in pixels.
     * @default Auto-calculated to fit within bounds
     */
    property real radius: Math.min(width, height) / 2 - strokeWidth / 2

    /**
     * @brief Width of the arc stroke in pixels.
     * @default 20
     */
    property real strokeWidth: 20

    // === Appearance Properties ===

    /**
     * @brief Color of the zone arc.
     * @default "#aa2222" (dark red for redline)
     */
    property color zoneColor: "#aa2222"

    /**
     * @brief Opacity of the zone arc.
     * @default 0.3 (semi-transparent overlay)
     */
    property real zoneOpacity: 0.3

    // === Internal Calculations ===

    /**
     * @brief Computed starting angle for this zone.
     * @internal
     */
    readonly property real zoneStartAngle: {
        const normalizedStart = (startValue - minValue) / (maxValue - minValue)
        const clampedStart = Math.max(0, Math.min(1, normalizedStart))
        return gaugeStartAngle + (gaugeTotalSweep * clampedStart)
    }

    /**
     * @brief Computed sweep angle for this zone.
     * @internal
     */
    readonly property real zoneSweepAngle: {
        const normalizedStart = (startValue - minValue) / (maxValue - minValue)
        const normalizedEnd = (endValue - minValue) / (maxValue - minValue)
        const clampedStart = Math.max(0, Math.min(1, normalizedStart))
        const clampedEnd = Math.max(0, Math.min(1, normalizedEnd))
        return gaugeTotalSweep * (clampedEnd - clampedStart)
    }

    // === Implementation ===

    implicitWidth: 400
    implicitHeight: 400

    Loader {
        id: arcLoader
        anchors.fill: parent
        source: "../primitives/GaugeArc.qml"

        onLoaded: {
            // Set geometry
            item.startAngle = Qt.binding(() => root.zoneStartAngle)
            item.sweepAngle = Qt.binding(() => root.zoneSweepAngle)
            item.radius = Qt.binding(() => root.radius)
            item.strokeWidth = Qt.binding(() => root.strokeWidth)

            // Set appearance
            item.strokeColor = Qt.binding(() => root.zoneColor)
            item.fillColor = "transparent"
            item.capStyle = ShapePath.FlatCap
            item.arcOpacity = Qt.binding(() => root.zoneOpacity)

            // No animation for static zones
            item.animated = false
        }
    }
}
