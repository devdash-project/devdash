import QtQuick

/**
 * @brief Digital numeric readout with unit label.
 *
 * DigitalReadout displays a numeric value with optional unit label,
 * commonly used as a center display in analog gauges or standalone.
 *
 * Supports:
 * - Automatic decimal precision
 * - Unit labels (RPM, mph, °F, etc.)
 * - Color changes based on thresholds
 * - Smooth font scaling
 *
 * @example
 * @code
 * DigitalReadout {
 *     value: rpm
 *     unit: "RPM"
 *     precision: 0
 *     warningThreshold: 6000
 *     criticalThreshold: 6500
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
     * @brief Unit label (e.g., "RPM", "mph", "°F").
     * @default ""
     */
    property string unit: ""

    /**
     * @brief Decimal precision for value display.
     * @default 0 (whole numbers)
     */
    property int precision: 0

    /**
     * @brief Threshold where color changes to warning.
     * @default Infinity (never warns)
     */
    property real warningThreshold: Infinity

    /**
     * @brief Threshold where color changes to critical.
     * @default Infinity (never critical)
     */
    property real criticalThreshold: Infinity

    // === Typography Properties ===

    /**
     * @brief Font size for the value.
     * @default 48
     */
    property real valueFontSize: 48

    /**
     * @brief Font size for the unit label.
     * @default valueFontSize / 2
     */
    property real unitFontSize: valueFontSize / 2

    /**
     * @brief Font family.
     * @default "Roboto"
     */
    property string fontFamily: "Roboto"

    /**
     * @brief Font weight for value.
     * @default Font.Bold
     */
    property int fontWeight: Font.Bold

    // === Color Properties ===

    /**
     * @brief Color when value is below warningThreshold.
     * @default "#ffffff"
     */
    property color normalColor: "#ffffff"

    /**
     * @brief Color when value is between warningThreshold and criticalThreshold.
     * @default "#ffaa00"
     */
    property color warningColor: "#ffaa00"

    /**
     * @brief Color when value is above criticalThreshold.
     * @default "#ff4444"
     */
    property color criticalColor: "#ff4444"

    // === Internal State ===

    /**
     * @brief Current color based on value and thresholds.
     * @internal
     */
    readonly property color currentColor: {
        if (value >= criticalThreshold) return criticalColor
        if (value >= warningThreshold) return warningColor
        return normalColor
    }

    /**
     * @brief Formatted value string.
     * @internal
     */
    readonly property string formattedValue: value.toFixed(precision)

    // === Implementation ===

    implicitWidth: 150
    implicitHeight: 70

    Column {
        anchors.centerIn: parent
        spacing: 4

        // Value text
        Text {
            id: valueText
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.formattedValue
            font.family: root.fontFamily
            font.pixelSize: root.valueFontSize
            font.weight: root.fontWeight
            color: root.currentColor

            // Smooth color transitions
            Behavior on color {
                ColorAnimation {
                    duration: 200
                }
            }
        }

        // Unit label
        Text {
            id: unitText
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.unit !== ""
            text: root.unit
            font.family: root.fontFamily
            font.pixelSize: root.unitFontSize
            font.weight: Font.Normal
            color: root.currentColor
            opacity: 0.8

            // Smooth color transitions
            Behavior on color {
                ColorAnimation {
                    duration: 200
                }
            }
        }
    }
}
