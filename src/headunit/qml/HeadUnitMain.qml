import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root

    width: 1024
    height: 600
    visible: true
    title: qsTr("Head Unit")
    color: "#1a1a1a"

    // Fullscreen on embedded
    visibility: Qt.platform.os === "linux" ? Window.FullScreen : Window.Windowed

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Header bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2a2a2a"
            radius: 10

            RowLayout {
                anchors.fill: parent
                anchors.margins: 15

                Text {
                    text: "/dev/dash"
                    font.pixelSize: 24
                    font.bold: true
                    color: "#ffffff"
                }

                Item { Layout.fillWidth: true }

                // Connection status
                Row {
                    spacing: 10

                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: dataBroker && dataBroker.isConnected ? "#00ff00" : "#ff0000"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: dataBroker && dataBroker.isConnected ? "Connected" : "Disconnected"
                        font.pixelSize: 14
                        color: "#888888"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        // Main content area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Left panel - vehicle data
            Rectangle {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                color: "#2a2a2a"
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 15

                    Text {
                        text: "Vehicle Data"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#ffffff"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444444"
                    }

                    // Data grid
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 10

                        Text { text: "RPM"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.rpm.toFixed(0) : "--"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Speed"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.vehicleSpeed.toFixed(0) + " km/h" : "--"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Throttle"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.throttlePosition.toFixed(0) + "%" : "--"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Coolant"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.coolantTemperature.toFixed(0) + "°C" : "--"
                            color: {
                                var temp = dataBroker ? dataBroker.coolantTemperature : 0;
                                if (temp > 105) return "#ff4444";
                                if (temp > 95) return "#ffaa00";
                                return "#ffffff";
                            }
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Oil Temp"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.oilTemperature.toFixed(0) + "°C" : "--"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Oil Pressure"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.oilPressure.toFixed(0) + " kPa" : "--"
                            color: {
                                var pressure = dataBroker ? dataBroker.oilPressure : 0;
                                if (pressure < 100) return "#ff4444";
                                return "#ffffff";
                            }
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "MAP"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.manifoldPressure.toFixed(0) + " kPa" : "--"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "IAT"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.intakeAirTemperature.toFixed(0) + "°C" : "--"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Battery"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: dataBroker ? dataBroker.batteryVoltage.toFixed(1) + "V" : "--"
                            color: {
                                var voltage = dataBroker ? dataBroker.batteryVoltage : 0;
                                if (voltage < 12.0) return "#ff4444";
                                if (voltage < 12.5) return "#ffaa00";
                                return "#ffffff";
                            }
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text { text: "Gear"; color: "#888888"; font.pixelSize: 14 }
                        Text {
                            text: {
                                var gear = dataBroker ? dataBroker.gear : 0;
                                return gear === 0 ? "N" : gear.toString();
                            }
                            color: dataBroker && dataBroker.gear === 0 ? "#00ff00" : "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // Center panel - placeholder for future features
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"
                radius: 10

                Column {
                    anchors.centerIn: parent
                    spacing: 10

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Head Unit"
                        font.pixelSize: 24
                        color: "#555555"
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Media, Navigation, Settings"
                        font.pixelSize: 14
                        color: "#444444"
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "(Coming Soon)"
                        font.pixelSize: 12
                        color: "#333333"
                    }
                }
            }
        }

        // Bottom navigation bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2a2a2a"
            radius: 10

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Repeater {
                    model: ["Home", "Media", "Navigation", "Settings"]

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: index === 0 ? "#444444" : "transparent"
                        radius: 5

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 14
                            color: index === 0 ? "#ffffff" : "#888888"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: console.log("Tapped:", modelData)
                        }
                    }
                }
            }
        }
    }
}
