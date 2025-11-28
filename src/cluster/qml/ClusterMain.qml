import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import DevDash.Cluster

Window {
    id: root

    width: 1920
    height: 720
    visible: true
    title: qsTr("Instrument Cluster")
    color: "#1a1a1a"

    // Fullscreen on embedded
    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 40

        // Left tachometer
        Tachometer {
            id: tachometer
            Layout.preferredWidth: 400
            Layout.preferredHeight: 400
            Layout.alignment: Qt.AlignVCenter

            value: dataBroker ? dataBroker.rpm : 0
            maxValue: 8000
            redlineStart: 6500
            label: "RPM"
        }

        // Center info panel
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                anchors.centerIn: parent
                spacing: 20

                // Speed display
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: dataBroker ? Math.round(dataBroker.vehicleSpeed) : "0"
                    font.pixelSize: 120
                    font.bold: true
                    color: "#ffffff"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "km/h"
                    font.pixelSize: 24
                    color: "#888888"
                }

                // Gear indicator
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 80
                    height: 80
                    radius: 10
                    color: "#333333"
                    border.color: "#555555"
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: {
                            var gear = dataBroker ? dataBroker.gear : 0;
                            return gear === 0 ? "N" : gear.toString();
                        }
                        font.pixelSize: 48
                        font.bold: true
                        color: dataBroker && dataBroker.gear === 0 ? "#00ff00" : "#ffffff"
                    }
                }

                // Temperature and pressure row
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 40

                    Column {
                        Text {
                            text: dataBroker ? dataBroker.coolantTemperature.toFixed(0) + "°C" : "--"
                            font.pixelSize: 28
                            color: {
                                var temp = dataBroker ? dataBroker.coolantTemperature : 0;
                                if (temp > 105) return "#ff4444";
                                if (temp > 95) return "#ffaa00";
                                return "#ffffff";
                            }
                        }
                        Text {
                            text: "COOLANT"
                            font.pixelSize: 12
                            color: "#888888"
                        }
                    }

                    Column {
                        Text {
                            text: dataBroker ? dataBroker.oilPressure.toFixed(0) + " kPa" : "--"
                            font.pixelSize: 28
                            color: {
                                var pressure = dataBroker ? dataBroker.oilPressure : 0;
                                if (pressure < 100) return "#ff4444";
                                return "#ffffff";
                            }
                        }
                        Text {
                            text: "OIL"
                            font.pixelSize: 12
                            color: "#888888"
                        }
                    }

                    Column {
                        Text {
                            text: dataBroker ? dataBroker.batteryVoltage.toFixed(1) + "V" : "--"
                            font.pixelSize: 28
                            color: {
                                var voltage = dataBroker ? dataBroker.batteryVoltage : 0;
                                if (voltage < 12.0) return "#ff4444";
                                if (voltage < 12.5) return "#ffaa00";
                                return "#ffffff";
                            }
                        }
                        Text {
                            text: "BATTERY"
                            font.pixelSize: 12
                            color: "#888888"
                        }
                    }
                }
            }
        }

        // Right - throttle/boost gauge (placeholder)
        Tachometer {
            id: throttleGauge
            Layout.preferredWidth: 400
            Layout.preferredHeight: 400
            Layout.alignment: Qt.AlignVCenter

            value: dataBroker ? dataBroker.manifoldPressure : 0
            maxValue: 250
            redlineStart: 200
            label: "MAP kPa"
        }
    }

    // Connection indicator
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        width: 12
        height: 12
        radius: 6
        color: dataBroker && dataBroker.isConnected ? "#00ff00" : "#ff0000"
    }
}
