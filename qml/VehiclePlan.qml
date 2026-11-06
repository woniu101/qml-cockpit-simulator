import QtQuick
import QtQuick.Controls

Item {
    id: plan
    required property var vehicle
    implicitWidth: 240
    implicitHeight: 400
    Item {
        anchors.centerIn: parent
        width: 240
        height: 400
        scale: Math.min(plan.width / width, plan.height / height)
        Rectangle {
            anchors.centerIn: parent
            width: 224
            height: 330
            radius: 90
            color: DriverTheme.night ? "#203d38" : "#e5eee7"
        }
        Label {
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            text: "↑  车头"
            color: DriverTheme.muted
            font.pixelSize: 12
        }
        Repeater {
            model: 4
            Rectangle {
                required property int index
                x: index % 2 ? 176 : 48
                y: index < 2 ? 92 : 274
                width: 16
                height: 46
                radius: 5
                color: "#233c36"
            }
        }
        Rectangle {
            x: 62; y: 44; width: 116; height: 306; radius: 38
            gradient: Gradient {
                GradientStop { position: 0; color: DriverTheme.night ? "#647e72" : "#f8fbf6" }
                GradientStop { position: 0.5; color: DriverTheme.night ? "#425f52" : "#d4e0d5" }
                GradientStop { position: 1; color: DriverTheme.night ? "#334d42" : "#eef3e9" }
            }
            border.width: 2
            border.color: DriverTheme.night ? "#8db5a4" : "#90ab9e"
            Rectangle { x: 15; y: 67; width: 86; height: 42; radius: 12; color: "#4f6a65" }
            Rectangle { x: 18; y: 113; width: 80; height: 89; radius: 12; color: DriverTheme.night ? "#283f36" : "#a8bdb0" }
            Rectangle { x: 15; y: 207; width: 86; height: 38; radius: 12; color: "#4f6a65" }
            Repeater {
                model: 4
                Rectangle {
                    required property int index
                    x: index % 2 ? 103 : 5
                    y: index < 2 ? 110 : 161
                    width: 8
                    height: 45
                    radius: 3
                    color: "#2e766b"
                    opacity: 0.9 - plan.vehicle.windows[index] * 0.008
                }
            }
            Repeater {
                model: 2
                Rectangle {
                    required property int index
                    x: index ? 85 : 11
                    y: 22
                    width: 20
                    height: 6
                    radius: 3
                    color: "#d6f2e6"
                    border.color: "#8db5a4"
                }
            }
            Rectangle { x: 16; y: 284; width: 84; height: 4; radius: 2; color: "#bf7870" }
        }
        Repeater {
            model: 4
            Rectangle {
                required property int index
                x: index % 2 ? 175 : 56
                y: index < 2 ? 151 : 207
                width: 9
                height: 50
                radius: 4
                transformOrigin: Item.Top
                rotation: plan.vehicle.doors[index] ? (index % 2 ? -58 : 58) : 0
                color: plan.vehicle.doors[index] ? DriverTheme.warning : "#8caf9c"
                Behavior on rotation { NumberAnimation { duration: 220 } }
            }
        }
        Label {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            text: plan.vehicle.locked ? "车辆已锁定" : "车辆已解锁"
            color: DriverTheme.green
            font.pixelSize: 13
        }
    }
}
