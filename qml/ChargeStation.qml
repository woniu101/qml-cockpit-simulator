import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: station
    required property var vehicle
    color: DriverTheme.night ? "#162e2c" : "#e4eee7"
    radius: 18
    implicitWidth: 370
    implicitHeight: 380

    VehiclePlan {
        x: 8
        y: 20
        width: station.width * 0.52
        height: station.height - 40
        vehicle: station.vehicle
    }

    Rectangle {
        id: charger
        x: station.width * 0.65
        y: station.height * 0.16
        width: station.width * 0.27
        height: station.height * 0.58
        radius: 16
        color: "#294b42"
        border.color: "#709e88"
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 10
            height: parent.height * 0.55
            radius: 8
            color: "#112b27"
            Column {
                anchors.centerIn: parent
                spacing: 8
                Label { anchors.horizontalCenter: parent.horizontalCenter; text: "DC"; font.pixelSize: 20; color: "#a3d9bf" }
                Label { anchors.horizontalCenter: parent.horizontalCenter; text: station.vehicle.chargePower + " kW"; font.pixelSize: 15; color: "#eef5ec" }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: station.vehicle.charging ? "充电中" : station.vehicle.plugged ? "已连接" : "待连接"
                    color: "#a3d9bf"; font.pixelSize: 12
                }
            }
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            width: 22; height: 8; radius: 4
            color: station.vehicle.charging ? "#91dec2" : "#6b8879"
        }
    }

    Shape {
        anchors.fill: parent
        visible: station.vehicle.plugged
        ShapePath {
            strokeWidth: 5
            strokeColor: station.vehicle.charging ? "#478f7c" : "#76968c"
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            startX: station.width * 0.77
            startY: station.height * 0.72
            PathCubic {
                control1X: station.width * 0.76
                control1Y: station.height * 0.94
                control2X: station.width * 0.43
                control2Y: station.height * 0.94
                x: station.width * 0.43
                y: station.height * 0.68
            }
        }
    }

    Rectangle {
        id: energy
        visible: station.vehicle.charging
        width: 9; height: 9; radius: 5
        color: "#a9f5d6"
    }
    PathAnimation {
        target: energy
        duration: 1500
        loops: Animation.Infinite
        running: station.visible && station.vehicle.charging
        path: Path {
            startX: station.width * 0.77
            startY: station.height * 0.72
            PathCubic {
                control1X: station.width * 0.76
                control1Y: station.height * 0.94
                control2X: station.width * 0.43
                control2Y: station.height * 0.94
                x: station.width * 0.43
                y: station.height * 0.68
            }
        }
    }
}
