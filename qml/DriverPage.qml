import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    required property bool fullScreen
    signal fullScreenRequested()
    signal bodyRequested()
    signal keysRequested()
    readonly property var state: Driver.state
    readonly property bool driving: state.speed > 0
    readonly property color bright: "#f1f7ef"
    readonly property color dim: "#b2c6ba"
    readonly property color accent: "#bde6cf"

    RoadView {
        objectName: "driverRoad"
        anchors.fill: parent
        anchors.bottomMargin: dashboard.height - 18
        distanceKm: 2 * page.state.forwardKm - page.state.tripKm
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 88
        gradient: Gradient {
            GradientStop { position: 0; color: "#a911231e" }
            GradientStop { position: 1; color: "#0011231e" }
        }
    }

    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 18
        spacing: 8
        Rectangle {
            implicitWidth: link.implicitWidth + 24
            implicitHeight: 34
            radius: 9
            color: "#d82a4136"
            Label {
                id: link
                objectName: "driverLinkStatus"
                anchors.centerIn: parent
                text: !Driver.running ? "服务离线" : Driver.telemetryPaused ? "遥测暂停"
                    : Driver.cockpitCount > 0 ? "●  车机已连接" : "○  等待车机"
                color: Driver.cockpitCount > 0 && !Driver.telemetryPaused ? page.accent : page.bright
                font.pixelSize: 13
            }
        }
        Item { Layout.fillWidth: true }
        DashKey {
            objectName: "driverBodyDrawer"
            text: "车辆操作"
            onClicked: page.bodyRequested()
        }
        DashKey {
            objectName: "driverKeys"
            text: "按键  F1"
            onClicked: page.keysRequested()
        }
        DashKey {
            objectName: "driverFullscreen"
            text: page.fullScreen ? "退出全屏" : "全屏  F11"
            onClicked: page.fullScreenRequested()
        }
    }

    Rectangle {
        id: dashboard
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.max(218, Math.min(252, page.height * 0.34))
        radius: 26
        gradient: Gradient {
            GradientStop { position: 0; color: "#263c34" }
            GradientStop { position: 1; color: "#101d19" }
        }
        border.color: "#536d60"

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 14
            width: Math.min(parent.width - 48, 1170)
            spacing: 9

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 95
                RowLayout {
                    anchors.fill: parent
                    spacing: 12
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        Column {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 5
                            Label { text: "动力电池"; color: page.dim; font.pixelSize: 13 }
                            Label {
                                text: page.state.soc.toFixed(1) + " %"
                                color: page.state.soc <= 20 ? "#f4c093" : page.accent
                                font.pixelSize: 26; font.weight: Font.DemiBold
                            }
                            Rectangle {
                                width: 98; height: 4; radius: 2; color: "#486157"
                                Rectangle {
                                    width: parent.width * page.state.soc / 100
                                    height: parent.height
                                    radius: 2
                                    color: page.state.soc <= 20 ? "#f4c093" : page.accent
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 14
                        Label {
                            objectName: "driverSpeed"
                            text: page.state.speed
                            color: page.bright
                            font.family: "Segoe UI"
                            font.pixelSize: dashboard.height < 240 ? 66 : 82
                            font.weight: Font.DemiBold
                        }
                        Column {
                            spacing: 6
                            Label { text: "km/h"; color: page.dim; font.pixelSize: 15 }
                            Label {
                                text: page.state.gear + " · " + ({ P: "驻车", R: "倒车", N: "空挡", D: "前进" })[page.state.gear]
                                color: page.accent; font.pixelSize: 15; font.weight: Font.DemiBold
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        Column {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 5
                            Label { anchors.right: parent.right; text: "本次里程"; color: page.dim; font.pixelSize: 13 }
                            Label {
                                anchors.right: parent.right
                                text: page.state.tripKm.toFixed(2) + " km"
                                color: page.bright; font.pixelSize: 24
                            }
                            Label {
                                anchors.right: parent.right
                                text: page.state.charging ? "正在充电" : page.state.brake ? "制动中"
                                      : page.driving ? (page.state.throttle ? "驱动中" : "滑行中") : "已停稳"
                                color: page.accent; font.pixelSize: 12
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#486157" }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 3
                radius: 1.5
                color: "#2f4a40"
                Rectangle {
                    width: parent.width * page.state.speed / (page.state.gear === "R" ? 15 : 120)
                    height: parent.height
                    radius: parent.radius
                    color: page.state.brake ? "#dfb789" : "#8cd7b8"
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Repeater {
                    model: ["P", "R", "N", "D"]
                    DashKey {
                        required property string modelData
                        objectName: "driverGear" + modelData
                        Layout.preferredWidth: modelData === "P" ? 76 : 57
                        text: modelData + "  ·  " + Driver.keyLabels[modelData]
                        selected: page.state.gear === modelData
                        enabled: Driver.running
                        onClicked: Driver.shift(modelData)
                    }
                }
                Item { Layout.fillWidth: true }
                Pedal { brake: true; Layout.preferredWidth: 126 }
                Pedal { brake: false; Layout.preferredWidth: 102 }
            }

            Label {
                Layout.fillWidth: true
                text: !Driver.running ? "本机服务未启动"
                    : Driver.warning ? Driver.message
                    : page.state.driveBlock ? page.state.driveBlock
                    : page.state.gear === "P" && !page.state.brake ? "踩住刹车后可挂 D 或 R"
                    : "方向盘视角 · 直线驾驶"
                color: Driver.warning || page.state.driveBlock ? "#f2c49c" : page.dim
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    component DashKey: DriverButton {
        id: key
        implicitHeight: 40
        contentItem: Text {
            text: key.text
            font: key.font
            color: !key.enabled ? "#8ba297" : key.selected ? "#1b3026" : page.bright
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 9
            color: !key.enabled ? "#2a3c33"
                : key.selected || key.down ? page.accent : key.hovered ? "#4c6859" : "#d82a4136"
            border.width: 1
            border.color: key.visualFocus ? page.accent : "#637d6f"
        }
    }

    component Pedal: DashKey {
        id: pedal
        required property bool brake
        objectName: brake ? "driverBrake" : "driverThrottle"
        text: brake ? "刹车" : "油门"
        selected: brake ? page.state.brake : page.state.throttle
        enabled: Driver.running
        implicitHeight: 60
        onPressed: Driver.pedal(brake, false, true)
        onReleased: Driver.pedal(brake, false, false)
        onCanceled: Driver.pedal(brake, false, false)
        contentItem: Row {
            spacing: 8
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: pedal.brake ? 32 : 17
                height: pedal.brake ? 21 : 31
                radius: 4
                color: pedal.selected ? "#426b58" : "#111d19"
                border.color: "#708b7b"
                Column {
                    anchors.centerIn: parent
                    spacing: 3
                    Repeater {
                        model: pedal.brake ? 3 : 5
                        Rectangle { width: pedal.brake ? 20 : 9; height: 1; color: "#91a698" }
                    }
                }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: pedal.text + "  " + Driver.keyLabels[pedal.brake ? "brake" : "throttle"]
                color: pedal.selected ? "#1b3026" : page.bright
                font.pixelSize: 13
            }
        }
    }
}
