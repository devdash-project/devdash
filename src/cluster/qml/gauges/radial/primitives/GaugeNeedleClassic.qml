import QtQuick
import QtQuick.Shapes

/**
 * @brief Classic vintage-style gauge needle.
 *
 * Simple solid bar with optional counterweight.
 * Perfect for vintage/retro gauge aesthetics.
 *
 * @example
 * @code
 * GaugeNeedleClassic {
 *     angle: 45
 *     length: 140
 *     color: "#000000"
 *     counterweightRadius: 8
 * }
 * @endcode
 */
Item {
    id: root

    // === Rotation Properties ===

    /**
     * @brief Current rotation angle in degrees (0 = 3 o'clock).
     * @note This property animates with SpringAnimation
     */
    property real angle: 0

    // === Geometry Properties ===

    /**
     * @brief Length of needle from pivot to tip (pixels).
     * @default 100
     */
    property real length: 100

    /**
     * @brief Width of needle bar (pixels).
     * @default 3
     */
    property real needleWidth: 3

    /**
     * @brief Radius of decorative counterweight circle.
     * Set to 0 to disable.
     * @default 0 (no counterweight)
     */
    property real counterweightRadius: 0

    // === Appearance Properties ===

    /**
     * @brief Needle color.
     * @default "#000000" (black)
     */
    property color color: "#000000"

    /**
     * @brief Border width around needle edge (pixels).
     * @default 0 (no border)
     */
    property real borderWidth: 0

    /**
     * @brief Border color (if borderWidth > 0).
     * @default "transparent"
     */
    property color borderColor: "transparent"

    /**
     * @brief Overall opacity of needle.
     * @default 1.0
     */
    property real needleOpacity: 1.0

    // === Animation Properties ===

    /**
     * @brief Enable SpringAnimation for rotation.
     * @default true
     */
    property bool animated: true

    /**
     * @brief Spring stiffness (higher = faster response).
     * @default 3.5
     */
    property real spring: 3.5

    /**
     * @brief Damping factor (0-1, higher = less oscillation).
     * @default 0.25
     */
    property real damping: 0.25

    /**
     * @brief Mass of needle (affects momentum).
     * @default 1.0
     */
    property real mass: 1.0

    /**
     * @brief Threshold for stopping animation (degrees).
     * @default 0.25
     */
    property real epsilon: 0.25

    // === Internal Implementation ===

    implicitWidth: length * 2
    implicitHeight: length * 2

    // Needle bar
    Rectangle {
        id: needleBar
        width: root.needleWidth
        height: root.length
        radius: root.needleWidth / 2
        color: root.color
        opacity: root.needleOpacity
        border.width: root.borderWidth
        border.color: root.borderColor

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.verticalCenter
        anchors.bottomMargin: 0

        transformOrigin: Item.Bottom

        transform: Rotation {
            origin.x: needleBar.width / 2
            origin.y: needleBar.height
            angle: root.angle

            Behavior on angle {
                enabled: root.animated
                SpringAnimation {
                    spring: root.spring
                    damping: root.damping
                    mass: root.mass
                    epsilon: root.epsilon
                }
            }
        }
    }

    // Counterweight (decorative circle at pivot)
    Rectangle {
        id: counterweight
        visible: root.counterweightRadius > 0
        width: root.counterweightRadius * 2
        height: root.counterweightRadius * 2
        radius: root.counterweightRadius
        color: root.color
        opacity: root.needleOpacity
        border.width: root.borderWidth
        border.color: root.borderColor

        anchors.centerIn: parent
    }
}
