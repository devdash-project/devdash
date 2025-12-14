import QtQuick

/**
 * @brief Atomic bezel/frame primitive for gauge exterior.
 *
 * GaugeBezel renders the outer decorative frame around a gauge.
 * Supports solid colors, chrome/metallic gradients, and various styles.
 *
 * @example
 * @code
 * GaugeBezel {
 *     outerRadius: 200
 *     innerRadius: 190
 *     style: "chrome"
 * }
 * @endcode
 */
Item {
    id: root

    // === Geometry Properties ===

    /**
     * @brief Outer radius of bezel (pixels).
     * @default 200
     */
    property real outerRadius: 200

    /**
     * @brief Inner radius of bezel (pixels).
     *
     * Bezel thickness = outerRadius - innerRadius
     *
     * @default 190
     */
    property real innerRadius: 190

    /**
     * @brief Additional border width (pixels).
     *
     * Adds thin line at inner/outer edges.
     *
     * @default 0
     */
    property real borderWidth: 0

    // === Appearance Properties ===

    /**
     * @brief Bezel fill color.
     * @default "#444444"
     */
    property color color: "#444444"

    /**
     * @brief Border color (if borderWidth > 0).
     * @default "#555555"
     */
    property color borderColor: "#555555"

    /**
     * @brief Bezel visual style.
     *
     * Supported styles:
     * - "flat": Solid color
     * - "chrome": Metallic gradient
     * - "carbon": Carbon fiber texture (if textureSource set)
     * - "brushed": Brushed metal effect
     *
     * @default "flat"
     */
    property string style: "flat"

    /**
     * @brief Chrome gradient highlight color.
     * @default "#888888"
     */
    property color chromeHighlight: "#888888"

    /**
     * @brief Chrome gradient shadow color.
     * @default "#222222"
     */
    property color chromeShadow: "#222222"

    /**
     * @brief Overall opacity.
     * @default 1.0
     */
    property real bezelOpacity: 1.0

    /**
     * @brief Texture image source (for carbon, brushed styles).
     * @default ""
     */
    property string textureSource: ""

    // === Advanced ===

    /**
     * @brief Enable antialiasing.
     * @default true
     */
    property bool antialiasing: true

    // === Internal Implementation ===

    implicitWidth: outerRadius * 2
    implicitHeight: outerRadius * 2

    // Bezel ring - using thick border for donut shape (pure Qt6, no masking needed)
    Rectangle {
        id: bezelRing
        width: root.outerRadius * 2
        height: root.outerRadius * 2
        radius: root.outerRadius
        anchors.centerIn: parent

        // Transparent fill - the border IS the bezel
        color: "transparent"

        // Thick border creates the ring/donut shape
        border.width: root.outerRadius - root.innerRadius
        border.color: root.style === "chrome" ? "transparent" : root.color

        opacity: root.bezelOpacity
        antialiasing: root.antialiasing

        // Chrome gradient (applied to border via nested Rectangle)
        Rectangle {
            visible: root.style === "chrome"
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"

            border.width: parent.border.width
            gradient: chromeGradient

            Gradient {
                id: chromeGradient
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: root.chromeHighlight }
                GradientStop { position: 0.3; color: Qt.lighter(root.chromeHighlight, 1.2) }
                GradientStop { position: 0.5; color: root.color }
                GradientStop { position: 0.7; color: Qt.darker(root.color, 1.2) }
                GradientStop { position: 1.0; color: root.chromeShadow }
            }
        }

        // Texture overlay for carbon/brushed styles
        Item {
            visible: root.textureSource !== ""
            anchors.fill: parent
            clip: true

            Image {
                id: texture
                anchors.fill: parent
                source: root.textureSource
                fillMode: Image.PreserveAspectCrop
                smooth: true

                // Clip to ring shape using parent's circular clip
                layer.enabled: true
                layer.smooth: true
            }

            // Mask to create ring (hide center)
            Rectangle {
                width: root.innerRadius * 2
                height: root.innerRadius * 2
                radius: root.innerRadius
                anchors.centerIn: parent
                color: "#000000"  // Hides texture in center
            }
        }

        // Optional inner border accent
        Rectangle {
            visible: root.borderWidth > 0
            width: root.innerRadius * 2
            height: root.innerRadius * 2
            radius: root.innerRadius
            anchors.centerIn: parent
            color: "transparent"
            border.width: root.borderWidth
            border.color: root.borderColor
            antialiasing: root.antialiasing
        }
    }
}
