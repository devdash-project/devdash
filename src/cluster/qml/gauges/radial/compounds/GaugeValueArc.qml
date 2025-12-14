import QtQuick
import QtQuick.Shapes

/**
 * @brief Value-driven arc that fills from minValue to currentValue.
 *
 * GaugeValueArc is a compound component that composes a GaugeArc primitive
 * with automatic sweep calculation and color transitions based on thresholds.
 *
 * The arc automatically:
 * - Calculates sweep angle based on value/maxValue ratio
 * - Changes color at warning and critical thresholds
 * - Animates smoothly with SmoothedAnimation
 *
 * @example
 * @code
 * GaugeValueArc {
 *     value: rpm
 *     minValue: 0
 *     maxValue: 8000
 *     warningThreshold: 6000
 *     criticalThreshold: 6500
 *     normalColor: "#00aaff"
 *     warningColor: "#ffaa00"
 *     criticalColor: "#ff4444"
 * }
 * @endcode
 */
Item {
    id: root

    // === Value Properties ===

    /**
     * @brief Current value to display.
     * @default 0
     */
    property real value: 0

    /**
     * @brief Minimum value of the range.
     * @default 0
     */
    property real minValue: 0

    /**
     * @brief Maximum value of the range.
     * @default 100
     */
    property real maxValue: 100

    /**
     * @brief Threshold where color changes from normal to warning.
     * @default maxValue (never warns)
     */
    property real warningThreshold: maxValue

    /**
     * @brief Threshold where color changes from warning to critical.
     * @default maxValue (never critical)
     */
    property real criticalThreshold: maxValue

    // === Geometry Properties ===

    /**
     * @brief Starting angle in degrees (0 = 3 o'clock, positive = clockwise).
     * @default -225 (typical gauge starting position at 7:30)
     */
    property real startAngle: -225

    /**
     * @brief Total sweep angle range in degrees.
     * @default 270 (three-quarter circle)
     */
    property real totalSweepAngle: 270

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

    // === Color Properties ===

    /**
     * @brief Color when value is below warningThreshold.
     * @default "#00aaff" (blue)
     */
    property color normalColor: "#00aaff"

    /**
     * @brief Color when value is between warningThreshold and criticalThreshold.
     * @default "#ffaa00" (orange)
     */
    property color warningColor: "#ffaa00"

    /**
     * @brief Color when value is above criticalThreshold.
     * @default "#ff4444" (red)
     */
    property color criticalColor: "#ff4444"

    // === Animation Properties ===

    /**
     * @brief Enable smooth animations.
     * @default true
     */
    property bool animated: true

    /**
     * @brief Animation velocity in degrees per second.
     * @default 360 (one full rotation per second)
     */
    property real animationVelocity: 360

    // === Internal State ===

    /**
     * @brief Computed sweep angle based on current value.
     * @internal
     */
    readonly property real valueSweepAngle: {
        const normalized = (value - minValue) / (maxValue - minValue)
        const clamped = Math.max(0, Math.min(1, normalized))
        return totalSweepAngle * clamped
    }

    /**
     * @brief Current color based on value and thresholds.
     * @internal
     */
    readonly property color currentColor: {
        if (value >= criticalThreshold) return criticalColor
        if (value >= warningThreshold) return warningColor
        return normalColor
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
            item.startAngle = Qt.binding(() => root.startAngle)
            item.radius = Qt.binding(() => root.radius)
            item.strokeWidth = Qt.binding(() => root.strokeWidth)

            // Set appearance
            item.strokeColor = Qt.binding(() => root.currentColor)
            item.fillColor = "transparent"
            item.capStyle = ShapePath.RoundCap

            // Set animation
            item.animated = Qt.binding(() => root.animated)

            // Bind sweep with SmoothedAnimation
            const sweepBinding = Qt.binding(() => root.valueSweepAngle)
            if (root.animated) {
                item.sweepAngle = root.valueSweepAngle
                // Note: SmoothedAnimation is applied in the primitive's Behavior
            } else {
                item.sweepAngle = sweepBinding
            }
        }
    }
}
