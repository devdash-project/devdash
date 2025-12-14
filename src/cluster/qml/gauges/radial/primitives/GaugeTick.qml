import QtQuick

/**
 * @brief Atomic tick mark primitive for gauge scales.
 *
 * GaugeTick renders a single rotatable tick mark at a specified angle
 * and distance from the gauge center. Used by GaugeTickRing to create
 * complete gauge scales.
 *
 * @example
 * @code
 * GaugeTick {
 *     angle: 45
 *     length: 15
 *     width: 3
 *     distanceFromCenter: 140
 *     color: "#888888"
 * }
 * @endcode
 */
Item {
    id: root

    // === Position Properties ===

    /**
     * @brief Rotation angle in degrees (0 = 3 o'clock).
     *
     * The tick rotates around the gauge center to this position.
     *
     * @default 0
     */
    property real angle: 0

    /**
     * @brief Distance from gauge center to tick start (pixels).
     *
     * This is the radius at which the tick begins.
     * The tick extends inward by `length` pixels.
     *
     * @default 100
     */
    property real distanceFromCenter: 100

    // === Geometry Properties ===

    /**
     * @brief Length of tick mark (pixels).
     *
     * Major ticks are typically 15px, minor ticks 8px.
     *
     * @default 15
     */
    property real length: 15

    /**
     * @brief Width of tick mark (pixels).
     *
     * Major ticks are typically 3px, minor ticks 2px.
     *
     * @default 3
     */
    property real tickWidth: 3

    // === Appearance Properties ===

    /**
     * @brief Tick mark color.
     * @default "#888888" (gray)
     */
    property color color: "#888888"

    /**
     * @brief Overall opacity.
     * @default 1.0
     */
    property real tickOpacity: 1.0

    /**
     * @brief Rounded ends for smoother appearance.
     * @default true
     */
    property bool roundedEnds: true

    /**
     * @brief Show decorative circle at inner end of tick.
     *
     * Creates a classic gauge aesthetic like vintage speedometers.
     *
     * @default false
     */
    property bool showInnerCircle: false

    /**
     * @brief Diameter of inner circle (if enabled).
     * @default tickWidth * 2
     */
    property real innerCircleDiameter: tickWidth * 2

    /**
     * @brief Color of inner circle.
     * @default color (matches tick mark)
     */
    property color innerCircleColor: color

    // === Internal Implementation ===

    implicitWidth: distanceFromCenter * 2
    implicitHeight: distanceFromCenter * 2

    Rectangle {
        id: tick
        width: root.tickWidth
        height: root.length
        radius: root.roundedEnds ? root.tickWidth / 2 : 0
        color: root.color
        opacity: root.tickOpacity

        // Position at specified radius, extending inward
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: (parent.height / 2) - root.distanceFromCenter

        // Rotate around center
        transform: Rotation {
            origin.x: tick.width / 2
            origin.y: (root.height / 2) - tick.y
            angle: root.angle
        }
    }

    // Inner decorative circle (classic gauge style)
    Rectangle {
        id: innerCircle
        visible: root.showInnerCircle
        width: root.innerCircleDiameter
        height: root.innerCircleDiameter
        radius: root.innerCircleDiameter / 2
        color: root.innerCircleColor
        opacity: root.tickOpacity

        // Position at inner end of tick mark
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: (parent.height / 2) - root.distanceFromCenter + root.length - (root.innerCircleDiameter / 2)

        // Rotate with tick mark
        transform: Rotation {
            origin.x: innerCircle.width / 2
            origin.y: (root.height / 2) - innerCircle.y
            angle: root.angle
        }
    }
}
