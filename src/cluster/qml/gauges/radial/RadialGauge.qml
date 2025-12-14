import QtQuick

/**
 * @brief Complete analog radial gauge template.
 *
 * AnalogGauge is a fully-featured radial gauge that composes primitives
 * and compounds into a professional analog gauge display.
 *
 * Includes (all toggleable):
 * - Background face
 * - Background arc track
 * - Zone arcs (redline, warning)
 * - Tick marks and labels
 * - Value arc (fills to current value)
 * - Animated needle
 * - Center cap
 * - Digital readout
 * - Decorative bezel
 *
 * @example
 * @code
 * AnalogGauge {
 *     value: dataBroker.rpm
 *     minValue: 0
 *     maxValue: 8000
 *     label: "RPM"
 *     redlineStart: 6500
 *     warningThreshold: 6000
 * }
 * @endcode
 */
Item {
    id: root

    // === Value Properties ===

    /**
     * @brief Current value to display.
     */
    property real value: 0

    /**
     * @brief Minimum value of the range.
     */
    property real minValue: 0

    /**
     * @brief Maximum value of the range.
     */
    property real maxValue: 100

    /**
     * @brief Gauge label (e.g., "RPM", "Speed").
     */
    property string label: ""

    /**
     * @brief Unit for digital readout (e.g., "RPM", "mph").
     */
    property string unit: ""

    // === Threshold Properties ===

    /**
     * @brief Value where warning zone starts.
     */
    property real warningThreshold: maxValue

    /**
     * @brief Value where critical/redline zone starts.
     */
    property real redlineStart: maxValue

    // === Geometry Properties ===

    /**
     * @brief Starting angle in degrees.
     * @default -225 (7:30 position)
     */
    property real startAngle: -225

    /**
     * @brief Total sweep angle in degrees.
     * @default 270 (three-quarter circle)
     */
    property real sweepAngle: 270

    // === Feature Toggles ===

    /**
     * @brief Show background face plate.
     */
    property bool showFace: true

    /**
     * @brief Show background arc track.
     */
    property bool showBackgroundArc: true

    /**
     * @brief Show value-filled arc.
     */
    property bool showValueArc: true

    /**
     * @brief Show redline zone arc.
     */
    property bool showRedline: true

    /**
     * @brief Show tick marks and labels.
     */
    property bool showTicks: true

    /**
     * @brief Show animated needle.
     */
    property bool showNeedle: true

    /**
     * @brief Show center pivot cap.
     */
    property bool showCenterCap: true

    /**
     * @brief Show digital numeric readout.
     */
    property bool showDigitalReadout: false

    /**
     * @brief Show decorative bezel frame.
     */
    property bool showBezel: false

    // === Tick Configuration ===

    /**
     * @brief Interval between major ticks.
     */
    property real majorTickInterval: (maxValue - minValue) / 10

    /**
     * @brief Interval between minor ticks (0 = disabled).
     */
    property real minorTickInterval: majorTickInterval / 5

    /**
     * @brief Divisor for tick labels (e.g., 1000 shows "8" for 8000).
     */
    property real labelDivisor: 1

    // === Color Scheme ===

    property color faceColor: "#1a1a1a"
    property color bezelColor: "#2a2a2a"
    property color backgroundArcColor: "#333333"
    property color valueArcColor: "#00aaff"
    property color needleColor: "#ffffff"
    property color tickColor: "#888888"
    property color redlineColor: "#aa2222"
    property color warningColor: "#ffaa00"
    property color criticalColor: "#ff4444"

    // === Needle Customization ===

    /**
     * @brief Width of needle at base (pixels).
     * @default 4
     */
    property real needleWidth: 4

    /**
     * @brief Width of needle at tip (pixels).
     * @default 2 (tapered)
     */
    property real needleTipWidth: 2

    /**
     * @brief Border width around needle edge (pixels).
     * @default 0 (no border)
     */
    property real needleBorderWidth: 0

    /**
     * @brief Border color around needle.
     * @default "transparent"
     */
    property color needleBorderColor: "transparent"

    /**
     * @brief Needle type to use.
     *
     * Supported types:
     * - "tapered": Thin tip, wide base (default - professional racing style)
     * - "classic": Simple solid bar (vintage gauges)
     * - "arrow": Open arrow outline (future)
     * - "modern": Flat bar for digital gauges (future)
     *
     * @default "tapered"
     */
    property string needleType: "tapered"

    /**
     * @brief Counterweight extension behind pivot (pixels).
     * @default 0 (no counterweight)
     */
    property real needlePivotOffset: 0

    // === Center Cap Customization ===

    /**
     * @brief Diameter of center cap (pixels).
     * @default 30
     */
    property real centerCapDiameter: 30

    /**
     * @brief Center cap fill color.
     * @default faceColor
     */
    property color centerCapColor: faceColor

    /**
     * @brief Center cap border color.
     * @default needleColor
     */
    property color centerCapBorderColor: needleColor

    /**
     * @brief Center cap border width (pixels).
     * @default 2
     */
    property real centerCapBorderWidth: 2

    /**
     * @brief Enable metallic gradient on center cap.
     * @default false
     */
    property bool centerCapGradient: false

    /**
     * @brief Gradient highlight color (top).
     * @default "#888888"
     */
    property color centerCapGradientTop: "#888888"

    /**
     * @brief Gradient shadow color (bottom).
     * @default "#444444"
     */
    property color centerCapGradientBottom: "#444444"

    // === Typography Customization ===

    /**
     * @brief Font family for tick labels.
     * @default "Roboto"
     */
    property string tickLabelFontFamily: "Roboto"

    /**
     * @brief Font size for tick labels (pixels).
     * @default 18
     */
    property int tickLabelFontSize: 18

    /**
     * @brief Font weight for tick labels.
     * @default Font.Bold
     */
    property int tickLabelFontWeight: Font.Bold

    /**
     * @brief Font family for gauge label.
     * @default "Roboto"
     */
    property string gaugeLabelFontFamily: "Roboto"

    /**
     * @brief Font size for gauge label (pixels).
     * @default 18
     */
    property int gaugeLabelFontSize: 18

    /**
     * @brief Font weight for gauge label.
     * @default Font.Bold
     */
    property int gaugeLabelFontWeight: Font.Bold

    /**
     * @brief Show outline on tick label text.
     *
     * Creates black border around numbers for classic gauge aesthetic.
     *
     * @default false
     */
    property bool showTickLabelOutline: false

    /**
     * @brief Color of tick label outline.
     * @default "#000000"
     */
    property color tickLabelOutlineColor: "#000000"

    // === Tick Mark Customization ===

    /**
     * @brief Show decorative circles at inner end of ticks.
     *
     * Creates classic vintage gauge aesthetic.
     *
     * @default false
     */
    property bool showTickInnerCircles: false

    /**
     * @brief Diameter of tick inner circles (if enabled).
     * @default 6
     */
    property real tickInnerCircleDiameter: 6

    // === Internal Helpers ===

    /**
     * @brief Get needle QML file path based on type.
     * @internal
     */
    function getNeedleSource(type) {
        const needleMap = {
            "tapered": "primitives/GaugeNeedleTapered.qml",
            "classic": "primitives/GaugeNeedleClassic.qml"
        }
        return needleMap[type] || needleMap["tapered"]
    }

    // === Implementation ===

    implicitWidth: 400
    implicitHeight: 400

    // Layer 1: Background face
    Loader {
        active: showFace
        anchors.fill: parent
        source: "primitives/GaugeFace.qml"
        onLoaded: {
            item.diameter = Qt.binding(() => Math.min(root.width, root.height))
            item.color = Qt.binding(() => root.faceColor)
        }
    }

    // Layer 2: Background arc track
    Loader {
        active: showBackgroundArc
        anchors.fill: parent
        source: "primitives/GaugeArc.qml"
        onLoaded: {
            item.startAngle = Qt.binding(() => root.startAngle)
            item.sweepAngle = Qt.binding(() => root.sweepAngle)
            item.strokeColor = Qt.binding(() => root.backgroundArcColor)
            item.strokeWidth = 20
            item.animated = false
        }
    }

    // Layer 3: Redline zone arc
    Loader {
        active: showRedline && redlineStart < maxValue
        anchors.fill: parent
        source: "compounds/GaugeZoneArc.qml"
        onLoaded: {
            item.minValue = Qt.binding(() => root.minValue)
            item.maxValue = Qt.binding(() => root.maxValue)
            item.startValue = Qt.binding(() => root.redlineStart)
            item.endValue = Qt.binding(() => root.maxValue)
            item.gaugeStartAngle = Qt.binding(() => root.startAngle)
            item.gaugeTotalSweep = Qt.binding(() => root.sweepAngle)
            item.zoneColor = Qt.binding(() => root.redlineColor)
            item.zoneOpacity = 0.3
            item.strokeWidth = 20
        }
    }

    // Layer 4: Tick ring
    Loader {
        active: showTicks
        anchors.fill: parent
        source: "compounds/GaugeTickRing.qml"
        onLoaded: {
            // Values and geometry
            item.minValue = Qt.binding(() => root.minValue)
            item.maxValue = Qt.binding(() => root.maxValue)
            item.majorTickInterval = Qt.binding(() => root.majorTickInterval)
            item.minorTickInterval = Qt.binding(() => root.minorTickInterval)
            item.labelDivisor = Qt.binding(() => root.labelDivisor)
            item.startAngle = Qt.binding(() => root.startAngle)
            item.sweepAngle = Qt.binding(() => root.sweepAngle)

            // Colors
            item.warningStart = Qt.binding(() => root.warningThreshold)
            item.criticalStart = Qt.binding(() => root.redlineStart)
            item.normalColor = Qt.binding(() => root.tickColor)
            item.warningColor = Qt.binding(() => root.warningColor)
            item.criticalColor = Qt.binding(() => root.criticalColor)

            // Typography
            item.fontSize = Qt.binding(() => root.tickLabelFontSize)
            item.fontFamily = Qt.binding(() => root.tickLabelFontFamily)
            item.fontWeight = Qt.binding(() => root.tickLabelFontWeight)
            item.showLabelOutline = Qt.binding(() => root.showTickLabelOutline)
            item.labelOutlineColor = Qt.binding(() => root.tickLabelOutlineColor)

            // Decorations
            item.showInnerCircles = Qt.binding(() => root.showTickInnerCircles)
            item.innerCircleDiameter = Qt.binding(() => root.tickInnerCircleDiameter)
        }
    }

    // Layer 5: Value arc
    Loader {
        active: showValueArc
        anchors.fill: parent
        source: "compounds/GaugeValueArc.qml"
        onLoaded: {
            item.value = Qt.binding(() => root.value)
            item.minValue = Qt.binding(() => root.minValue)
            item.maxValue = Qt.binding(() => root.maxValue)
            item.startAngle = Qt.binding(() => root.startAngle)
            item.totalSweepAngle = Qt.binding(() => root.sweepAngle)
            item.warningThreshold = Qt.binding(() => root.warningThreshold)
            item.criticalThreshold = Qt.binding(() => root.redlineStart)
            item.normalColor = Qt.binding(() => root.valueArcColor)
            item.warningColor = Qt.binding(() => root.warningColor)
            item.criticalColor = Qt.binding(() => root.criticalColor)
            item.strokeWidth = 22
        }
    }

    // Layer 6: Needle
    Loader {
        active: showNeedle
        anchors.fill: parent
        source: getNeedleSource(root.needleType)

        onLoaded: {
            // Calculate angle based on value
            item.angle = Qt.binding(() => {
                const norm = (root.value - root.minValue) / (root.maxValue - root.minValue)
                const clamped = Math.max(0, Math.min(1, norm))
                return root.startAngle + (root.sweepAngle * clamped)
            })

            // Geometry
            item.length = Math.min(root.width, root.height) / 2 - 60
            item.needleWidth = Qt.binding(() => root.needleWidth)

            // Tapered needle has tipWidth, classic doesn't
            if (root.needleType === "tapered" && item.hasOwnProperty("tipWidth")) {
                item.tipWidth = Qt.binding(() => root.needleTipWidth)
                item.pivotOffset = Qt.binding(() => root.needlePivotOffset)
            }

            // Classic needle has counterweightRadius
            if (root.needleType === "classic" && item.hasOwnProperty("counterweightRadius")) {
                item.counterweightRadius = Qt.binding(() => root.needlePivotOffset)
            }

            // Appearance (all needle types)
            item.color = Qt.binding(() => root.needleColor)
            item.borderWidth = Qt.binding(() => root.needleBorderWidth)
            item.borderColor = Qt.binding(() => root.needleBorderColor)
        }
    }

    // Layer 7: Center cap
    Loader {
        active: showCenterCap
        anchors.centerIn: parent
        source: "primitives/GaugeCenterCap.qml"
        onLoaded: {
            // Geometry
            item.diameter = Qt.binding(() => root.centerCapDiameter)
            item.borderWidth = Qt.binding(() => root.centerCapBorderWidth)

            // Appearance
            item.color = Qt.binding(() => root.centerCapColor)
            item.borderColor = Qt.binding(() => root.centerCapBorderColor)

            // Gradient effect (for metallic look)
            item.hasGradient = Qt.binding(() => root.centerCapGradient)
            item.gradientTop = Qt.binding(() => root.centerCapGradientTop)
            item.gradientBottom = Qt.binding(() => root.centerCapGradientBottom)
        }
    }

    // Layer 8: Digital readout (center)
    Loader {
        active: showDigitalReadout
        anchors.centerIn: parent
        anchors.verticalCenterOffset: Math.min(root.width, root.height) / 4
        source: "compounds/DigitalReadout.qml"
        onLoaded: {
            item.value = Qt.binding(() => root.value)
            item.unit = Qt.binding(() => root.unit)
            item.precision = 0
            item.valueFontSize = 32
            item.warningThreshold = Qt.binding(() => root.warningThreshold)
            item.criticalThreshold = Qt.binding(() => root.redlineStart)
        }
    }

    // Layer 9: Label (bottom)
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        text: root.label
        font.family: root.gaugeLabelFontFamily
        font.pixelSize: root.gaugeLabelFontSize
        font.weight: root.gaugeLabelFontWeight
        color: root.tickColor
        visible: root.label !== ""
    }

    // Layer 10: Bezel (outermost)
    Loader {
        active: showBezel
        anchors.fill: parent
        source: "primitives/GaugeBezel.qml"
        onLoaded: {
            item.outerRadius = Qt.binding(() => Math.min(root.width, root.height) / 2)
            item.bezelWidth = 20
            item.bezelColor = Qt.binding(() => root.bezelColor)
        }
    }
}
