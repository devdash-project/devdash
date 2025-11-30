import QtQuick

/**
 * @brief Atomic gauge face/dial plate primitive.
 *
 * GaugeFace renders the background circle or plate behind gauge elements.
 * Supports solid colors, gradients, opacity for video overlay mode,
 * and optional border/bezel integration.
 *
 * @example
 * @code
 * GaugeFace {
 *     diameter: 380
 *     color: "#222222"
 *     borderWidth: 2
 *     borderColor: "#444444"
 * }
 * @endcode
 */
Item {
    id: root

    // === Geometry Properties ===

    /**
     * @brief Diameter of the gauge face (pixels).
     * @default 380
     */
    property real diameter: 380

    /**
     * @brief Border width around face edge (pixels).
     * @default 0
     */
    property real borderWidth: 0

    // === Appearance Properties ===

    /**
     * @brief Face background color.
     * @default "#222222" (dark gray)
     */
    property color color: "#222222"

    /**
     * @brief Border color (if borderWidth > 0).
     * @default "#444444"
     */
    property color borderColor: "#444444"

    /**
     * @brief Overall opacity (0-1).
     *
     * For video overlay mode, set to ~0.3-0.6 for translucent effect.
     *
     * @default 1.0 (fully opaque)
     */
    property real faceOpacity: 1.0

    /**
     * @brief Enable radial gradient fill.
     *
     * When true, creates gradient from center to edge
     * using gradientCenter and gradientEdge colors.
     *
     * @default false
     */
    property bool useGradient: false

    /**
     * @brief Gradient color at center (if useGradient is true).
     * @default color
     */
    property color gradientCenter: color

    /**
     * @brief Gradient color at edge (if useGradient is true).
     * @default Qt.darker(color, 1.2)
     */
    property color gradientEdge: Qt.darker(color, 1.2)

    /**
     * @brief Enable texture/image background.
     *
     * When set, loads an image for gauge face background.
     * Useful for carbon fiber, brushed metal, etc.
     *
     * @default "" (no texture)
     */
    property string textureSource: ""

    // === Advanced ===

    /**
     * @brief Enable antialiasing.
     * @default true
     */
    property bool antialiasing: true

    // === Internal Implementation ===

    implicitWidth: diameter
    implicitHeight: diameter

    Rectangle {
        id: face
        width: root.diameter
        height: root.diameter
        radius: root.diameter / 2
        anchors.centerIn: parent

        color: root.useGradient ? "transparent" : root.color
        border.width: root.borderWidth
        border.color: root.borderColor
        opacity: root.faceOpacity

        antialiasing: root.antialiasing
        clip: true  // Enable clipping for circular texture

        // Radial gradient (if enabled)
        gradient: root.useGradient ? faceGradient : undefined

        Gradient {
            id: faceGradient
            GradientStop { position: 0.0; color: root.gradientCenter }
            GradientStop { position: 1.0; color: root.gradientEdge }
        }

        // Texture overlay (if set) - clipped to circle by parent
        Image {
            id: texture
            visible: root.textureSource !== ""
            anchors.fill: parent
            source: root.textureSource
            fillMode: Image.PreserveAspectCrop
            smooth: true
        }
    }
}
