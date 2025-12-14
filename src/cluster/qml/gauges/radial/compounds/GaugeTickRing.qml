import QtQuick

/**
 * @brief Complete ring of tick marks and labels for a radial gauge.
 *
 * GaugeTickRing generates major and minor tick marks with corresponding
 * labels around the gauge arc. Ticks can be colored based on value ranges
 * (e.g., normal, warning, critical zones).
 *
 * @example
 * @code
 * GaugeTickRing {
 *     minValue: 0
 *     maxValue: 8000
 *     majorTickInterval: 1000
 *     minorTickInterval: 200
 *     labelDivisor: 1000      // Show "8" instead of "8000"
 *     warningStart: 6000
 *     criticalStart: 6500
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
     * @brief Interval between major ticks.
     * @default 10
     */
    property real majorTickInterval: 10

    /**
     * @brief Interval between minor ticks.
     * Set to 0 to disable minor ticks.
     * @default 2
     */
    property real minorTickInterval: 2

    /**
     * @brief Divisor for label values (e.g., 1000 shows "8" for 8000).
     * @default 1
     */
    property real labelDivisor: 1

    /**
     * @brief Value where ticks change to warning color.
     * @default maxValue (never warns)
     */
    property real warningStart: maxValue

    /**
     * @brief Value where ticks change to critical color.
     * @default maxValue (never critical)
     */
    property real criticalStart: maxValue

    // === Geometry Properties ===

    /**
     * @brief Starting angle of the gauge in degrees (0 = 3 o'clock).
     * @default -225
     */
    property real startAngle: -225

    /**
     * @brief Total sweep angle of the gauge in degrees.
     * @default 270
     */
    property real sweepAngle: 270

    /**
     * @brief Radius to the inner edge of ticks.
     * @default Auto-calculated from component size
     */
    property real innerRadius: Math.min(width, height) / 2 - 60

    /**
     * @brief Radius to tick labels.
     * @default innerRadius - 25
     */
    property real labelRadius: innerRadius - 25

    /**
     * @brief Length of major tick marks.
     * @default 15
     */
    property real majorTickLength: 15

    /**
     * @brief Width of major tick marks.
     * @default 2
     */
    property real majorTickWidth: 2

    /**
     * @brief Length of minor tick marks.
     * @default 8
     */
    property real minorTickLength: 8

    /**
     * @brief Width of minor tick marks.
     * @default 1
     */
    property real minorTickWidth: 1

    // === Color Properties ===

    /**
     * @brief Color for normal ticks and labels.
     * @default "#888888"
     */
    property color normalColor: "#888888"

    /**
     * @brief Color for warning zone ticks and labels.
     * @default "#ffaa00"
     */
    property color warningColor: "#ffaa00"

    /**
     * @brief Color for critical zone ticks and labels.
     * @default "#ff4444"
     */
    property color criticalColor: "#ff4444"

    // === Typography Properties ===

    /**
     * @brief Font size for tick labels.
     * @default 16
     */
    property real fontSize: 16

    /**
     * @brief Font family for tick labels.
     * @default "Roboto"
     */
    property string fontFamily: "Roboto"

    /**
     * @brief Font weight for tick labels.
     * @default Font.Bold
     */
    property int fontWeight: Font.Bold

    /**
     * @brief Show outline/stroke on tick labels.
     * @default false
     */
    property bool showLabelOutline: false

    /**
     * @brief Color of label outline.
     * @default "#000000"
     */
    property color labelOutlineColor: "#000000"

    // === Tick Decoration Properties ===

    /**
     * @brief Show decorative circles at inner end of ticks.
     * @default false
     */
    property bool showInnerCircles: false

    /**
     * @brief Diameter of tick inner circles (if enabled).
     * @default 6
     */
    property real innerCircleDiameter: 6

    // === Internal Functions ===

    /**
     * @brief Get tick color based on value.
     * @internal
     */
    function getColorForValue(value) {
        if (value >= criticalStart) return criticalColor
        if (value >= warningStart) return warningColor
        return normalColor
    }

    /**
     * @brief Calculate angle for a given value.
     * @internal
     */
    function angleForValue(value) {
        const normalized = (value - minValue) / (maxValue - minValue)
        return startAngle + (sweepAngle * normalized)
    }

    // === Implementation ===

    implicitWidth: 400
    implicitHeight: 400

    // Major ticks and labels
    Repeater {
        model: {
            const count = Math.floor((maxValue - minValue) / majorTickInterval) + 1
            return count
        }

        delegate: Item {
            id: majorTickDelegate

            readonly property real tickValue: minValue + (index * majorTickInterval)
            readonly property real tickAngle: angleForValue(tickValue)
            readonly property color tickColor: getColorForValue(tickValue)

            // Major tick mark
            Loader {
                anchors.fill: parent
                source: "../primitives/GaugeTick.qml"
                onLoaded: {
                    item.angle = majorTickDelegate.tickAngle
                    item.distanceFromCenter = root.innerRadius
                    item.length = root.majorTickLength
                    item.tickWidth = root.majorTickWidth
                    item.color = majorTickDelegate.tickColor
                    item.showInnerCircle = root.showInnerCircles
                    item.innerCircleDiameter = root.innerCircleDiameter
                    item.innerCircleColor = majorTickDelegate.tickColor
                }
            }

            // Tick label
            Loader {
                anchors.fill: parent
                source: "../primitives/GaugeTickLabel.qml"
                onLoaded: {
                    item.angle = majorTickDelegate.tickAngle
                    item.distanceFromCenter = root.labelRadius
                    item.text = (majorTickDelegate.tickValue / root.labelDivisor).toFixed(root.labelDivisor >= 1000 ? 0 : 1)
                    item.fontSize = root.fontSize
                    item.fontFamily = root.fontFamily
                    item.fontWeight = root.fontWeight
                    item.color = majorTickDelegate.tickColor
                    item.showOutline = root.showLabelOutline
                    item.outlineColor = root.labelOutlineColor
                    item.keepUpright = true
                }
            }
        }
    }

    // Minor ticks (if enabled)
    Repeater {
        model: {
            if (minorTickInterval <= 0) return 0
            const count = Math.floor((maxValue - minValue) / minorTickInterval) + 1
            return count
        }

        delegate: Item {
            id: minorTickDelegate

            readonly property real tickValue: minValue + (index * minorTickInterval)
            readonly property real tickAngle: angleForValue(tickValue)
            readonly property color tickColor: getColorForValue(tickValue)
            readonly property bool isMajorTick: (tickValue % majorTickInterval) === 0

            // Only render if not a major tick position
            Loader {
                active: !minorTickDelegate.isMajorTick
                anchors.fill: parent
                source: "../primitives/GaugeTick.qml"
                onLoaded: {
                    item.angle = minorTickDelegate.tickAngle
                    item.distanceFromCenter = root.innerRadius
                    item.length = root.minorTickLength
                    item.tickWidth = root.minorTickWidth
                    item.color = minorTickDelegate.tickColor
                    item.showInnerCircle = root.showInnerCircles
                    item.innerCircleDiameter = root.innerCircleDiameter * 0.7 // Smaller for minor ticks
                    item.innerCircleColor = minorTickDelegate.tickColor
                }
            }
        }
    }
}
