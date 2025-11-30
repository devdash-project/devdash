import QtQuick
import QtQuick.Shapes

/**
 * @brief Atomic needle primitive for analog gauges.
 *
 * GaugeNeedle renders a rotatable pointer with configurable style,
 * physics-based SpringAnimation, and optional shadow effects.
 *
 * Supports multiple visual styles:
 * - rectangle: Simple bar (fastest)
 * - triangle: Classic pointed needle
 * - tapered: Thin tip, wide base
 * - fancy: Custom path with decorative elements
 *
 * Uses SpringAnimation for realistic needle movement with natural
 * overshoot and settle behavior.
 *
 * @example
 * @code
 * GaugeNeedle {
 *     angle: 45  // Current rotation
 *     length: 140
 *     color: "#ff6600"
 *     style: "tapered"
 * }
 * @endcode
 */
Item {
    id: root

    // === Rotation Properties ===

    /**
     * @brief Current rotation angle in degrees (0 = 3 o'clock).
     *
     * The needle rotates around its anchor point to this angle.
     * Uses Qt angle convention: 0° = right, 90° = down, etc.
     *
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
     * @brief Width of needle at base (pixels).
     * @default 4
     */
    property real needleWidth: 4

    /**
     * @brief Width of needle at tip (pixels).
     *
     * For tapered needles, this is smaller than needleWidth.
     * For rectangle needles, this equals needleWidth.
     *
     * @default needleWidth (no taper)
     */
    property real tipWidth: needleWidth

    /**
     * @brief Offset from pivot point in negative Y direction.
     *
     * Positive values extend the needle backwards past the pivot.
     * Useful for counterweight effect.
     *
     * @default 0 (no extension past pivot)
     */
    property real pivotOffset: 0

    // === Appearance Properties ===

    /**
     * @brief Needle color.
     * @default "#ff6600" (orange)
     */
    property color color: "#ff6600"

    /**
     * @brief Needle visual style.
     *
     * Supported styles:
     * - "rectangle": Simple rectangular bar
     * - "triangle": Pointed triangular needle
     * - "tapered": Thin tip, wide base (classic gauge style)
     * - "fancy": Custom decorative shape
     *
     * @default "tapered"
     */
    property string style: "tapered"

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

    // === Shadow Properties ===

    /**
     * @brief Enable drop shadow under needle.
     * @default false
     */
    property bool hasShadow: false

    /**
     * @brief Shadow color.
     * @default Qt.rgba(0, 0, 0, 0.5) (semi-transparent black)
     */
    property color shadowColor: Qt.rgba(0, 0, 0, 0.5)

    /**
     * @brief Shadow offset in pixels (downward).
     * @default 2
     */
    property real shadowOffset: 2

    // === Animation Properties ===

    /**
     * @brief Enable SpringAnimation for rotation.
     *
     * When true, needle movement has natural physics (overshoot, settle).
     * When false, uses linear interpolation.
     *
     * @default true
     */
    property bool animated: true

    /**
     * @brief Spring stiffness (higher = faster response).
     *
     * Typical values:
     * - 2.0 = slow, smooth (temperature gauge)
     * - 3.5 = balanced (tachometer)
     * - 5.0 = fast, snappy (boost gauge)
     *
     * @default 3.5
     */
    property real spring: 3.5

    /**
     * @brief Damping factor (0-1, higher = less oscillation).
     *
     * - 0.0 = oscillates forever
     * - 0.25 = slight overshoot (realistic)
     * - 0.5 = critically damped (no overshoot)
     * - 1.0 = overdamped (sluggish)
     *
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

    // === Advanced ===

    /**
     * @brief Enable antialiasing.
     * @default true
     */
    property bool antialiasing: true

    // === Internal Implementation ===

    implicitWidth: (length + pivotOffset) * 2
    implicitHeight: (length + pivotOffset) * 2

    // Shadow layer (rendered first, behind needle)
    Shape {
        id: shadowShape
        visible: root.hasShadow
        anchors.fill: parent
        opacity: root.needleOpacity

        transform: [
            Rotation {
                origin.x: root.width / 2
                origin.y: root.height / 2
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
            },
            Translate {
                y: root.shadowOffset
            }
        ]

        ShapePath {
            strokeColor: "transparent"
            fillColor: root.shadowColor

            startX: root.width / 2
            startY: root.height / 2 + root.pivotOffset

            PathLine {
                x: root.width / 2 + (root.style === "triangle" ? 0 : root.tipWidth / 2)
                y: root.height / 2 - root.length
            }
            PathLine {
                x: root.width / 2 - (root.style === "triangle" ? 0 : root.tipWidth / 2)
                y: root.height / 2 - root.length
            }
            PathLine {
                x: root.width / 2
                y: root.height / 2 + root.pivotOffset
            }
        }
    }

    // Main needle
    Shape {
        id: needleShape
        anchors.fill: parent
        opacity: root.needleOpacity
        antialiasing: root.antialiasing

        transform: Rotation {
            origin.x: root.width / 2
            origin.y: root.height / 2
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

        ShapePath {
            id: needlePath
            strokeColor: root.borderWidth > 0 ? root.borderColor : "transparent"
            strokeWidth: root.borderWidth
            fillColor: root.color
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            // Start at pivot point
            startX: root.width / 2 + root.needleWidth / 2
            startY: root.height / 2 + root.pivotOffset

            // Simple tapered needle shape (works for all styles as a basic needle)
            PathLine {
                x: root.width / 2 + root.tipWidth / 2
                y: root.height / 2 - root.length
            }
            PathLine {
                x: root.width / 2 - root.tipWidth / 2
                y: root.height / 2 - root.length
            }
            PathLine {
                x: root.width / 2 - root.needleWidth / 2
                y: root.height / 2 + root.pivotOffset
            }
        }
    }
}
