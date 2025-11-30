import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real value: 0
    property real maxValue: 8000
    property real minValue: 0
    property real redlineStart: 6500
    property string label: "RPM"

    // Internal
    readonly property real startAngle: -225
    readonly property real endAngle: 45
    readonly property real sweepAngle: endAngle - startAngle
    readonly property real valueAngle: startAngle + (value - minValue) / (maxValue - minValue) * sweepAngle

    width: 400
    height: 400

    // Background circle
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.95
        height: width
        radius: width / 2
        color: "#222222"
        border.color: "#444444"
        border.width: 3
    }

    // Gauge arc background
    Shape {
        id: bgShape
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height * 0.8

        ShapePath {
            strokeWidth: 20
            strokeColor: "#333333"
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: bgShape.width / 2
                centerY: bgShape.height / 2
                radiusX: bgShape.width / 2 - 10
                radiusY: bgShape.height / 2 - 10
                startAngle: root.startAngle
                sweepAngle: root.sweepAngle
            }
        }
    }

    // Redline arc
    Shape {
        id: redlineShape
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height * 0.8

        ShapePath {
            strokeWidth: 20
            strokeColor: "#aa2222"
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: redlineShape.width / 2
                centerY: redlineShape.height / 2
                radiusX: redlineShape.width / 2 - 10
                radiusY: redlineShape.height / 2 - 10
                startAngle: root.startAngle + (root.redlineStart - root.minValue) / (root.maxValue - root.minValue) * root.sweepAngle
                sweepAngle: root.sweepAngle - (root.redlineStart - root.minValue) / (root.maxValue - root.minValue) * root.sweepAngle
            }
        }
    }

    // Value arc
    Shape {
        id: valueShape
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height * 0.8

        ShapePath {
            strokeWidth: 20
            strokeColor: root.value >= root.redlineStart ? "#ff4444" : "#00aaff"
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: valueShape.width / 2
                centerY: valueShape.height / 2
                radiusX: valueShape.width / 2 - 10
                radiusY: valueShape.height / 2 - 10
                startAngle: root.startAngle
                sweepAngle: Math.max(0, (root.value - root.minValue) / (root.maxValue - root.minValue) * root.sweepAngle)
            }

            Behavior on strokeColor {
                ColorAnimation { duration: 200 }
            }
        }
    }

    // Tick marks
    Repeater {
        model: Math.floor((root.maxValue - root.minValue) / 1000) + 1

        Item {
            property real tickValue: root.minValue + index * 1000
            property real tickAngle: root.startAngle + (tickValue - root.minValue) / (root.maxValue - root.minValue) * root.sweepAngle

            anchors.centerIn: parent
            width: parent.width * 0.8
            height: parent.height * 0.8

            Rectangle {
                width: 3
                height: 15
                color: tickValue >= root.redlineStart ? "#ff4444" : "#888888"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 35
                transform: Rotation {
                    origin.x: 1.5
                    origin.y: parent.height / 2 - 35
                    angle: tickAngle + 90
                }
            }

            Text {
                text: (tickValue / 1000).toFixed(0)
                color: tickValue >= root.redlineStart ? "#ff4444" : "#888888"
                font.pixelSize: 18
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 55
                transform: Rotation {
                    origin.x: width / 2
                    origin.y: parent.height / 2 - 55
                    angle: tickAngle + 90
                }
            }
        }
    }

    // Center display
    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 30

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Math.round(root.value)
            font.pixelSize: 48
            font.bold: true
            color: root.value >= root.redlineStart ? "#ff4444" : "#ffffff"

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.pixelSize: 16
            color: "#888888"
        }
    }

    // Needle
    Rectangle {
        id: needle
        width: 4
        height: parent.height * 0.35
        color: "#ff6600"
        radius: 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.verticalCenter
        transformOrigin: Item.Bottom
        rotation: root.valueAngle + 90

        Behavior on rotation {
            NumberAnimation {
                duration: 100
                easing.type: Easing.OutQuad
            }
        }
    }

    // Center cap
    Rectangle {
        anchors.centerIn: parent
        width: 20
        height: 20
        radius: 10
        color: "#444444"
        border.color: "#666666"
        border.width: 2
    }
}
