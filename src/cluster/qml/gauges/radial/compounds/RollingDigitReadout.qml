import QtQuick

/**
 * @brief Classic mechanical counter with rolling digits.
 *
 * Displays a numeric value using a mechanical odometer/counter aesthetic
 * with individual rotating digit drums. Perfect for engine hours, mileage,
 * trip counters, or any cumulative metric.
 *
 * Each digit animates smoothly when changing, creating the classic
 * "rolling" effect seen in vintage speedometers and odometers.
 *
 * @example
 * @code
 * RollingDigitReadout {
 *     value: 1234.5
 *     digitCount: 6
 *     decimalPlaces: 1
 *     label: "ENGINE HOURS"
 * }
 * @endcode
 */
Item {
    id: root

    // === Value Properties ===

    /**
     * @brief Current value to display.
     * @default 0
     */
    property real value: 0

    /**
     * @brief Number of digit positions.
     * @default 6
     */
    property int digitCount: 6

    /**
     * @brief Number of decimal places.
     * Set to 0 for whole numbers only.
     * @default 1
     */
    property int decimalPlaces: 1

    /**
     * @brief Optional label above the readout.
     * @default ""
     */
    property string label: ""

    // === Appearance Properties ===

    /**
     * @brief Background color of digit drums.
     * @default "#0c0c0c" (near black)
     */
    property color backgroundColor: "#0c0c0c"

    /**
     * @brief Digit text color.
     * @default "#f8f8f8" (off-white)
     */
    property color digitColor: "#f8f8f8"

    /**
     * @brief Frame/bezel color.
     * @default "#2a2a2a"
     */
    property color frameColor: "#2a2a2a"

    /**
     * @brief Label text color.
     * @default "#888888"
     */
    property color labelColor: "#888888"

    // === Typography Properties ===

    /**
     * @brief Font family for digits.
     * @default "Courier New" (monospace)
     */
    property string digitFontFamily: "Courier New"

    /**
     * @brief Font size for digits (pixels).
     * @default 32
     */
    property int digitFontSize: 32

    /**
     * @brief Label font family.
     * @default "Roboto"
     */
    property string labelFontFamily: "Roboto"

    /**
     * @brief Label font size (pixels).
     * @default 12
     */
    property int labelFontSize: 12

    // === Internal State ===

    /**
     * @brief Formatted value string with leading zeros.
     * @internal
     */
    readonly property string formattedValue: {
        const totalDigits = root.digitCount
        const formatted = root.value.toFixed(root.decimalPlaces)
        const padded = formatted.padStart(totalDigits + (root.decimalPlaces > 0 ? 1 : 0), '0')
        return padded.slice(-totalDigits - (root.decimalPlaces > 0 ? 1 : 0))
    }

    // === Implementation ===

    implicitWidth: digitCount * 40 + 20
    implicitHeight: 80

    Column {
        anchors.centerIn: parent
        spacing: 4

        // Label
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.label !== ""
            text: root.label
            font.family: root.labelFontFamily
            font.pixelSize: root.labelFontSize
            font.weight: Font.Bold
            color: root.labelColor
        }

        // Digit display frame
        Rectangle {
            width: root.digitCount * 40
            height: 50
            color: root.frameColor
            radius: 4

            Row {
                anchors.centerIn: parent
                spacing: 2

                Repeater {
                    model: root.formattedValue.length

                    delegate: Item {
                        width: 36
                        height: 46

                        readonly property string digitChar: root.formattedValue[index]
                        readonly property bool isDecimalPoint: digitChar === '.'

                        // Decimal point
                        Rectangle {
                            visible: isDecimalPoint
                            width: 8
                            height: 8
                            radius: 4
                            color: root.digitColor
                            anchors.centerIn: parent
                            anchors.verticalCenterOffset: 15
                        }

                        // Digit drum
                        Rectangle {
                            visible: !isDecimalPoint
                            anchors.fill: parent
                            color: root.backgroundColor
                            radius: 3
                            border.width: 1
                            border.color: Qt.darker(root.backgroundColor, 1.3)

                            // Digit text
                            Text {
                                anchors.centerIn: parent
                                text: parent.parent.digitChar
                                font.family: root.digitFontFamily
                                font.pixelSize: root.digitFontSize
                                font.weight: Font.Bold
                                color: root.digitColor

                                // Smooth rolling animation
                                Behavior on text {
                                    SequentialAnimation {
                                        // Brief fade during change for "rolling" effect
                                        NumberAnimation {
                                            target: parent
                                            property: "opacity"
                                            to: 0.3
                                            duration: 50
                                        }
                                        NumberAnimation {
                                            target: parent
                                            property: "opacity"
                                            to: 1.0
                                            duration: 100
                                        }
                                    }
                                }
                            }

                            // Subtle highlight for drum effect
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.margins: 2
                                width: parent.width * 0.3
                                height: parent.height * 0.6
                                radius: 2
                                color: "#ffffff"
                                opacity: 0.05
                            }
                        }
                    }
                }
            }
        }
    }
}
