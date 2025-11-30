import QtQuick
import QtQuick.Window
import DevDash.Gauges

/**
 * @brief Test harness for gauge primitives visualization.
 *
 * Displays all primitive components with various configurations
 * to verify rendering and animations work correctly.
 *
 * Run this by temporarily replacing ClusterMain.qml or loading
 * it in a separate test window.
 */
Window {
    id: root
    width: 1920
    height: 720
    visible: true
    title: "Primitives Test"
    color: "#1a1a1a"

    Text {
        id: title
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        text: "Gauge Primitives Test Harness"
        font.pixelSize: 32
        font.bold: true
        color: "#ffffff"
    }

    // Grid layout for primitives
    Grid {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 40
        columns: 4
        spacing: 80

        // === Test 1: GaugeFace ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeFace\n(solid + gradient)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10

                GaugeFace {
                    diameter: 90
                    color: "#222222"
                    borderWidth: 2
                    borderColor: "#444444"
                }

                GaugeFace {
                    diameter: 90
                    useGradient: true
                    gradientCenter: "#333333"
                    gradientEdge: "#111111"
                }
            }
        }

        // === Test 2: GaugeBezel ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeBezel\n(flat + chrome)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10

                GaugeBezel {
                    outerRadius: 50
                    innerRadius: 42
                    color: "#333333"
                    style: "flat"
                }

                GaugeBezel {
                    outerRadius: 50
                    innerRadius: 42
                    style: "chrome"
                    chromeHighlight: "#888888"
                    chromeShadow: "#111111"
                }
            }
        }

        // === Test 3: GaugeArc ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeArc\n(animated)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 180
                height: 180

                // Background arc
                GaugeArc {
                    centerX: 90
                    centerY: 90
                    radius: 70
                    strokeWidth: 8
                    startAngle: 135
                    sweepAngle: 270
                    strokeColor: "#333333"
                    capStyle: ShapePath.RoundCap
                }

                // Animated value arc
                GaugeArc {
                    id: testArc
                    centerX: 90
                    centerY: 90
                    radius: 70
                    strokeWidth: 8
                    startAngle: 135
                    sweepAngle: arcAnimation.running ? 200 : 50
                    strokeColor: "#00aaff"
                    capStyle: ShapePath.RoundCap
                    animated: true
                    animationDuration: 1000

                    SequentialAnimation on sweepAngle {
                        id: arcAnimation
                        running: true
                        loops: Animation.Infinite
                        NumberAnimation { to: 270; duration: 2000; easing.type: Easing.InOutQuad }
                        PauseAnimation { duration: 500 }
                        NumberAnimation { to: 50; duration: 2000; easing.type: Easing.InOutQuad }
                        PauseAnimation { duration: 500 }
                    }
                }
            }
        }

        // === Test 4: GaugeNeedle ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeNeedle\n(SpringAnimation)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 180
                height: 180

                // Background circle for reference
                Rectangle {
                    anchors.centerIn: parent
                    width: 140
                    height: 140
                    radius: 70
                    color: "#222222"
                    border.width: 2
                    border.color: "#444444"
                }

                GaugeNeedle {
                    anchors.centerIn: parent
                    width: 180
                    height: 180
                    length: 60
                    needleWidth: 4
                    color: "#ff3333"
                    angle: needleAnimation.running ? 225 : 135
                    animated: true
                    spring: 3.5
                    damping: 0.25

                    SequentialAnimation on angle {
                        id: needleAnimation
                        running: true
                        loops: Animation.Infinite
                        NumberAnimation { to: 405; duration: 0 } // Jump to end
                        PauseAnimation { duration: 1000 }
                        NumberAnimation { to: 135; duration: 0 } // Jump to start
                        PauseAnimation { duration: 1000 }
                    }
                }

                GaugeCenterCap {
                    anchors.centerIn: parent
                    diameter: 16
                    borderWidth: 2
                    color: "#444444"
                    borderColor: "#666666"
                }
            }
        }

        // === Test 5: GaugeTick ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeTick\n(rotated)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 180
                height: 180

                Rectangle {
                    anchors.centerIn: parent
                    width: 120
                    height: 120
                    radius: 60
                    color: "#222222"
                }

                // Major ticks
                Repeater {
                    model: 9
                    GaugeTick {
                        centerX: 90
                        centerY: 90
                        angle: 135 + index * 33.75
                        distanceFromCenter: 50
                        length: 12
                        width: 3
                        color: "#ffffff"
                        roundedEnds: true
                    }
                }

                // Minor ticks
                Repeater {
                    model: 27
                    GaugeTick {
                        centerX: 90
                        centerY: 90
                        angle: 135 + index * 10
                        distanceFromCenter: 54
                        length: 6
                        width: 1.5
                        color: "#888888"
                        roundedEnds: true
                    }
                }
            }
        }

        // === Test 6: GaugeTickLabel ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeTickLabel\n(auto-rotate)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 180
                height: 180

                Rectangle {
                    anchors.centerIn: parent
                    width: 140
                    height: 140
                    radius: 70
                    color: "#222222"
                }

                Repeater {
                    model: 9
                    GaugeTickLabel {
                        centerX: 90
                        centerY: 90
                        angle: 135 + index * 33.75
                        distanceFromCenter: 55
                        value: index
                        fontSize: 14
                        color: "#ffffff"
                        keepUpright: true
                    }
                }
            }
        }

        // === Test 7: GaugeCenterCap ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "GaugeCenterCap\n(flat + gradient)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 20

                Rectangle {
                    width: 90
                    height: 90
                    color: "#222222"
                    radius: 45

                    GaugeCenterCap {
                        anchors.centerIn: parent
                        diameter: 20
                        borderWidth: 2
                        color: "#444444"
                        borderColor: "#666666"
                    }
                }

                Rectangle {
                    width: 90
                    height: 90
                    color: "#222222"
                    radius: 45

                    GaugeCenterCap {
                        anchors.centerIn: parent
                        diameter: 20
                        borderWidth: 2
                        borderColor: "#888888"
                        hasGradient: true
                        gradientTop: "#666666"
                        gradientBottom: "#222222"
                    }
                }
            }
        }

        // === Test 8: Composite Example ===
        Item {
            width: 200
            height: 240

            Text {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Composite\n(all primitives)"
                font.pixelSize: 14
                color: "#aaaaaa"
                horizontalAlignment: Text.AlignHCenter
            }

            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 180
                height: 180

                // Face
                GaugeFace {
                    anchors.centerIn: parent
                    diameter: 140
                    useGradient: true
                    gradientCenter: "#2a2a2a"
                    gradientEdge: "#1a1a1a"
                    borderWidth: 1
                    borderColor: "#333333"
                }

                // Background arc
                GaugeArc {
                    centerX: 90
                    centerY: 90
                    radius: 60
                    strokeWidth: 6
                    startAngle: 135
                    sweepAngle: 270
                    strokeColor: "#333333"
                    capStyle: ShapePath.RoundCap
                }

                // Value arc
                GaugeArc {
                    centerX: 90
                    centerY: 90
                    radius: 60
                    strokeWidth: 6
                    startAngle: 135
                    sweepAngle: 180
                    strokeColor: "#00ff88"
                    capStyle: ShapePath.RoundCap
                }

                // Ticks
                Repeater {
                    model: 7
                    GaugeTick {
                        centerX: 90
                        centerY: 90
                        angle: 135 + index * 45
                        distanceFromCenter: 50
                        length: 8
                        width: 2
                        color: "#ffffff"
                        roundedEnds: true
                    }
                }

                // Labels
                Repeater {
                    model: 7
                    GaugeTickLabel {
                        centerX: 90
                        centerY: 90
                        angle: 135 + index * 45
                        distanceFromCenter: 40
                        value: index * 2
                        fontSize: 11
                        color: "#ffffff"
                        keepUpright: true
                    }
                }

                // Needle
                GaugeNeedle {
                    anchors.centerIn: parent
                    width: 180
                    height: 180
                    length: 50
                    needleWidth: 3
                    color: "#ff3333"
                    angle: 135 + 180 // Pointing at value 4
                }

                // Center cap
                GaugeCenterCap {
                    anchors.centerIn: parent
                    diameter: 14
                    borderWidth: 2
                    borderColor: "#666666"
                    hasGradient: true
                    gradientTop: "#555555"
                    gradientBottom: "#222222"
                }

                // Bezel
                GaugeBezel {
                    anchors.centerIn: parent
                    outerRadius: 75
                    innerRadius: 70
                    style: "chrome"
                    chromeHighlight: "#666666"
                    chromeShadow: "#111111"
                }
            }
        }
    }
}
