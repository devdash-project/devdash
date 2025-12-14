import QtQuick

/**
 * @brief Atomic tick label primitive for gauge scales.
 *
 * GaugeTickLabel renders a single text label at a specified angle,
 * with automatic rotation to keep text upright and readable.
 *
 * Supports value formatting (precision, divisor, prefix/suffix).
 *
 * @example
 * @code
 * GaugeTickLabel {
 *     angle: 45
 *     distanceFromCenter: 120
 *     text: "4"
 *     fontSize: 18
 *     color: "#ffffff"
 * }
 * @endcode
 */
Item {
    id: root

    // === Position Properties ===

    /**
     * @brief Rotation angle in degrees (0 = 3 o'clock).
     * @default 0
     */
    property real angle: 0

    /**
     * @brief Distance from gauge center to label center (pixels).
     * @default 80
     */
    property real distanceFromCenter: 80

    // === Content Properties ===

    /**
     * @brief Display text.
     *
     * Can be set directly or auto-generated from `value`.
     *
     * @default "0"
     */
    property string text: "0"

    /**
     * @brief Numeric value (for formatting).
     *
     * When set, text is auto-generated using precision, divisor, etc.
     *
     * @default 0
     */
    property real value: 0

    /**
     * @brief Decimal places for value formatting.
     * @default 0 (integer)
     */
    property int precision: 0

    /**
     * @brief Divide value by this before display.
     *
     * Useful for showing "8" instead of "8000" on RPM gauge.
     *
     * @default 1 (no division)
     */
    property real divisor: 1

    /**
     * @brief Text prefix (e.g., "$", "+").
     * @default ""
     */
    property string prefix: ""

    /**
     * @brief Text suffix (e.g., "k", "°", "%").
     * @default ""
     */
    property string suffix: ""

    // === Typography Properties ===

    /**
     * @brief Font family.
     * @default "sans-serif"
     */
    property string fontFamily: "sans-serif"

    /**
     * @brief Font size in pixels.
     * @default 18
     */
    property int fontSize: 18

    /**
     * @brief Font weight.
     * @default Font.Bold
     */
    property int fontWeight: Font.Bold

    /**
     * @brief Text color.
     * @default "#888888"
     */
    property color color: "#888888"

    /**
     * @brief Enable text outline/stroke.
     *
     * Creates a border around text for better visibility.
     * Classic gauge aesthetic.
     *
     * @default false
     */
    property bool showOutline: false

    /**
     * @brief Outline color.
     * @default "#000000" (black)
     */
    property color outlineColor: "#000000"

    // === Behavior Properties ===

    /**
     * @brief Keep text upright regardless of gauge rotation.
     *
     * When true, text rotates to remain readable.
     * When false, text rotates with gauge.
     *
     * @default true
     */
    property bool keepUpright: true

    // === Internal Implementation ===

    implicitWidth: distanceFromCenter * 2
    implicitHeight: distanceFromCenter * 2

    // Formatted text (if using value instead of text)
    readonly property string displayText: {
        if (text !== "0" || value === 0) {
            return prefix + text + suffix
        }
        var formatted = (value / divisor).toFixed(precision)
        return prefix + formatted + suffix
    }

    Text {
        id: label
        text: root.displayText
        font.family: root.fontFamily
        font.pixelSize: root.fontSize
        font.weight: root.fontWeight
        color: root.color

        // Text outline for classic gauge aesthetic
        style: root.showOutline ? Text.Outline : Text.Normal
        styleColor: root.outlineColor

        // Center the label
        anchors.centerIn: parent

        // Position at specified radius from center
        x: (parent.width / 2) + (root.distanceFromCenter * Math.cos(root.angle * Math.PI / 180)) - (width / 2)
        y: (parent.height / 2) + (root.distanceFromCenter * Math.sin(root.angle * Math.PI / 180)) - (height / 2)

        // Rotate to keep upright (if enabled)
        rotation: root.keepUpright ? -root.angle : 0
    }
}
