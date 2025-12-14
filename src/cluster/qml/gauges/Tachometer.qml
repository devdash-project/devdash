import QtQuick

/**
 * @brief Tachometer gauge for displaying engine RPM.
 *
 * This is a specialized configuration of the AnalogGauge template
 * optimized for displaying engine RPM with appropriate tick intervals,
 * redline zone, and color thresholds.
 *
 * @example
 * @code
 * Tachometer {
 *     value: dataBroker.rpm
 *     maxValue: 8000
 *     redlineStart: 6500
 * }
 * @endcode
 */
Item {
    id: root

    // === Public Properties ===

    /**
     * @brief Current RPM value.
     * @default 0
     */
    property real value: 0

    /**
     * @brief Maximum RPM value.
     * @default 8000
     */
    property real maxValue: 8000

    /**
     * @brief Minimum RPM value.
     * @default 0
     */
    property real minValue: 0

    /**
     * @brief RPM value where redline zone starts.
     * @default 6500
     */
    property real redlineStart: 6500

    /**
     * @brief Gauge label text.
     * @default "RPM"
     */
    property string label: "RPM"

    // === Implementation ===

    implicitWidth: 400
    implicitHeight: 400

    Loader {
        anchors.fill: parent
        source: "radial/RadialGauge.qml"

        onLoaded: {
            // Value binding
            item.value = Qt.binding(() => root.value)
            item.minValue = Qt.binding(() => root.minValue)
            item.maxValue = Qt.binding(() => root.maxValue)

            // Labels
            item.label = Qt.binding(() => root.label)
            item.unit = ""

            // Thresholds
            item.warningThreshold = Qt.binding(() => root.maxValue * 0.75)  // 75% of max
            item.redlineStart = Qt.binding(() => root.redlineStart)

            // Tick configuration for RPM (every 1000, labels show thousands)
            item.majorTickInterval = 1000
            item.minorTickInterval = 200
            item.labelDivisor = 1000

            // Feature toggles (RPM-specific display)
            item.showFace = true
            item.showBackgroundArc = true
            item.showValueArc = true
            item.showRedline = true
            item.showTicks = true
            item.showNeedle = true
            item.showCenterCap = true
            item.showDigitalReadout = false  // Analog only for tach
            item.showBezel = false

            // Color scheme (racing black theme)
            item.faceColor = "#1a1a1a"
            item.bezelColor = "#2a2a2a"
            item.backgroundArcColor = "#333333"
            item.valueArcColor = "#00aaff"
            item.needleColor = "#ffffff"
            item.tickColor = "#888888"
            item.redlineColor = "#aa2222"
            item.warningColor = "#ffaa00"
            item.criticalColor = "#ff4444"
        }
    }
}
