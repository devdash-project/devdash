import QtQuick
import QtQuick.Shapes

/**
 * @brief Atomic arc primitive for gauge components.
 *
 * GaugeArc renders a single arc segment using Qt Quick Shapes PathAngleArc.
 * This is the foundational primitive used for:
 * - Background arc tracks
 * - Value indicator arcs
 * - Redline/warning zone arcs
 * - Any curved gauge element
 *
 * All properties are fully configurable for maximum flexibility.
 *
 * @example
 * @code
 * GaugeArc {
 *     startAngle: -225
 *     sweepAngle: 270
 *     radius: 150
 *     strokeWidth: 20
 *     strokeColor: "#00aaff"
 * }
 * @endcode
 */
Item {
    id: root

    // === Geometry Properties ===

    /**
     * @brief X coordinate of arc center point.
     * @default width / 2 (centered horizontally)
     */
    property real centerX: width / 2

    /**
     * @brief Y coordinate of arc center point.
     * @default height / 2 (centered vertically)
     */
    property real centerY: height / 2

    /**
     * @brief Radius of the arc.
     * @default Auto-calculated to fit within bounds minus stroke width
     */
    property real radius: Math.min(width, height) / 2 - strokeWidth / 2

    /**
     * @brief Starting angle in degrees (0 = 3 o'clock, positive = clockwise).
     *
     * Qt uses 0 degrees at 3 o'clock position:
     * - 0° = 3 o'clock (right)
     * - 90° = 6 o'clock (bottom)
     * - 180° = 9 o'clock (left)
     * - 270° = 12 o'clock (top)
     *
     * Common gauge start: -225° (7:30 position)
     *
     * @default -225 (typical gauge starting position)
     */
    property real startAngle: -225

    /**
     * @brief Arc sweep angle in degrees (positive = clockwise).
     *
     * Common values:
     * - 270° = three-quarter circle (typical automotive gauge)
     * - 180° = half circle
     * - 360° = full circle
     *
     * @default 270 (three-quarter sweep)
     */
    property real sweepAngle: 270

    /**
     * @brief Width of the arc stroke in pixels.
     * @default 20
     */
    property real strokeWidth: 20

    // === Appearance Properties ===

    /**
     * @brief Stroke color.
     *
     * Can be:
     * - Solid color: "#00aaff"
     * - Transparent: "transparent"
     * - Qt color: Qt.rgba(0, 0.67, 1, 1)
     *
     * @default "#333333" (dark gray)
     */
    property color strokeColor: "#333333"

    /**
     * @brief Fill color for the arc interior.
     *
     * Usually left transparent for gauge arcs, but can be filled
     * for pie chart or segment effects.
     *
     * @default "transparent"
     */
    property color fillColor: "transparent"

    /**
     * @brief Line cap style for arc endpoints.
     *
     * Options:
     * - ShapePath.RoundCap: Rounded ends (smooth)
     * - ShapePath.FlatCap: Flat ends (flush)
     * - ShapePath.SquareCap: Square ends (extended)
     *
     * @default ShapePath.RoundCap
     */
    property int capStyle: ShapePath.RoundCap

    /**
     * @brief Overall opacity of the arc.
     * @default 1.0 (fully opaque)
     */
    property real arcOpacity: 1.0

    // === Gradient Support ===

    /**
     * @brief Enable gradient stroke instead of solid color.
     *
     * When true, uses gradientStart and gradientStop colors
     * to create a linear gradient along the arc.
     *
     * @default false
     */
    property bool useGradient: false

    /**
     * @brief Gradient start color (if useGradient is true).
     * @default strokeColor
     */
    property color gradientStart: strokeColor

    /**
     * @brief Gradient stop color (if useGradient is true).
     * @default strokeColor
     */
    property color gradientStop: strokeColor

    // === Animation Properties ===

    /**
     * @brief Enable smooth animations for property changes.
     *
     * When true, changes to sweepAngle, strokeColor, etc.
     * will animate smoothly instead of jumping instantly.
     *
     * @default true
     */
    property bool animated: true

    /**
     * @brief Animation duration in milliseconds.
     * @default 100
     */
    property int animationDuration: 100

    // === Advanced ===

    /**
     * @brief Enable antialiasing for smooth edges.
     *
     * Disable for performance if many arcs are rendering.
     *
     * @default true
     */
    property bool antialiasing: true

    // === Internal Implementation ===

    implicitWidth: 400
    implicitHeight: 400

    Shape {
        id: shape
        anchors.fill: parent
        opacity: root.arcOpacity

        // Antialiasing
        antialiasing: root.antialiasing
        smooth: true

        ShapePath {
            id: arcPath

            // Stroke configuration
            strokeWidth: root.strokeWidth
            strokeColor: root.strokeColor
            fillColor: root.fillColor
            capStyle: root.capStyle

            // Start point of arc
            startX: root.centerX + root.radius * Math.cos(root.startAngle * Math.PI / 180)
            startY: root.centerY + root.radius * Math.sin(root.startAngle * Math.PI / 180)

            // Arc segment
            PathAngleArc {
                id: arc
                centerX: root.centerX
                centerY: root.centerY
                radiusX: root.radius
                radiusY: root.radius
                startAngle: root.startAngle
                sweepAngle: root.sweepAngle

                // Smooth animation for sweep changes
                Behavior on sweepAngle {
                    enabled: root.animated
                    NumberAnimation {
                        duration: root.animationDuration
                        easing.type: Easing.OutQuad
                    }
                }
            }
        }

        // Smooth animation for color changes
        Behavior on opacity {
            enabled: root.animated
            NumberAnimation {
                duration: root.animationDuration
                easing.type: Easing.OutQuad
            }
        }
    }

    // Color animation for stroke
    Behavior on strokeColor {
        enabled: root.animated && !root.useGradient
        ColorAnimation {
            duration: root.animationDuration
        }
    }

    // Gradient color animations
    Behavior on gradientStart {
        enabled: root.animated && root.useGradient
        ColorAnimation {
            duration: root.animationDuration
        }
    }

    Behavior on gradientStop {
        enabled: root.animated && root.useGradient
        ColorAnimation {
            duration: root.animationDuration
        }
    }
}
