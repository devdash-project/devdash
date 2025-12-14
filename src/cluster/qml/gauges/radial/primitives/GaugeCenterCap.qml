import QtQuick

/**
 * @brief Atomic center cap primitive for gauge needle pivot.
 *
 * GaugeCenterCap renders a decorative cap that covers the needle
 * pivot point. Supports solid colors, gradients, and border effects.
 *
 * @example
 * @code
 * GaugeCenterCap {
 *     diameter: 20
 *     color: "#444444"
 *     borderWidth: 2
 *     borderColor: "#666666"
 * }
 * @endcode
 */
Item {
    id: root

    // === Geometry Properties ===

    /**
     * @brief Diameter of center cap (pixels).
     * @default 20
     */
    property real diameter: 20

    /**
     * @brief Border width (pixels).
     * @default 2
     */
    property real borderWidth: 2

    // === Appearance Properties ===

    /**
     * @brief Fill color.
     * @default "#444444" (dark gray)
     */
    property color color: "#444444"

    /**
     * @brief Border color.
     * @default "#666666" (medium gray)
     */
    property color borderColor: "#666666"

    /**
     * @brief Enable radial gradient.
     *
     * Creates metallic/3D effect with light at top.
     *
     * @default false
     */
    property bool hasGradient: false

    /**
     * @brief Gradient highlight color (top).
     * @default "#666666"
     */
    property color gradientTop: "#666666"

    /**
     * @brief Gradient shadow color (bottom).
     * @default "#333333"
     */
    property color gradientBottom: "#333333"

    /**
     * @brief Overall opacity.
     * @default 1.0
     */
    property real capOpacity: 1.0

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
        id: cap
        width: root.diameter
        height: root.diameter
        radius: root.diameter / 2
        anchors.centerIn: parent

        color: root.hasGradient ? "transparent" : root.color
        border.width: root.borderWidth
        border.color: root.borderColor
        opacity: root.capOpacity
        antialiasing: root.antialiasing

        // Radial gradient for metallic effect
        gradient: root.hasGradient ? capGradient : undefined

        Gradient {
            id: capGradient
            GradientStop { position: 0.0; color: root.gradientTop }
            GradientStop { position: 1.0; color: root.gradientBottom }
        }
    }
}
