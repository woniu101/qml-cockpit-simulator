import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    readonly property var vehicle: Driver.state
    readonly property bool parked: vehicle.gear === "P"
    readonly property bool reached: vehicle.soc >= vehicle.chargeTarget
    readonly property string status: vehicle.charging ? "正在充电"
        : vehicle.plugged ? (reached ? "已达目标" : "已连接 · 待充电")
        : vehicle.port ? "充电口已打开" : "充电口已关闭"
    implicitHeight: width < 850 ? 940 : 630
    spacing: 18

    RowLayout {
        Layout.fillWidth: true
        Label { Layout.fillWidth: true; text: "充电与补能"; font.pixelSize: 27; color: DriverTheme.ink; font.weight: Font.DemiBold }
        Label { text: page.status; color: DriverTheme.green }
    }

    GridLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        columns: page.width >= 850 ? 2 : 1
        columnSpacing: 22
        rowSpacing: 16

        ChargeStation {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 430
            Layout.minimumHeight: page.width >= 850 ? 400 : 270
            vehicle: page.vehicle
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 540
            spacing: 16
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 4
                    Label { text: "当前电量"; color: DriverTheme.muted; font.pixelSize: 13 }
                    Label {
                        text: page.vehicle.soc.toFixed(1) + "%"
                        font.pixelSize: 46; font.family: "Segoe UI"; color: DriverTheme.green
                    }
                }
                Item { Layout.fillWidth: true }
                ColumnLayout {
                    spacing: 5
                    Label { text: "实时功率"; color: DriverTheme.muted; font.pixelSize: 13 }
                    Label { text: page.vehicle.chargePower + " kW"; color: DriverTheme.ink; font.pixelSize: 25 }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 8
                radius: 4
                color: DriverTheme.line
                Rectangle {
                    width: parent.width * page.vehicle.soc / 100
                    height: parent.height; radius: parent.radius; color: DriverTheme.green
                }
                Rectangle {
                    x: Math.min(parent.width - width, parent.width * page.vehicle.chargeTarget / 100)
                    anchors.verticalCenter: parent.verticalCenter
                    width: 2; height: 14; radius: 1; color: DriverTheme.ink
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { Layout.fillWidth: true; text: "目标电量"; color: DriverTheme.ink }
                Label { text: Math.round(target.value) + "%"; color: DriverTheme.green; font.pixelSize: 22 }
            }
            Slider {
                id: target
                objectName: "driverChargeTarget"
                Layout.fillWidth: true
                implicitHeight: 34
                from: 50; to: 100; stepSize: 1
                enabled: Driver.running
                property int proposed: -1
                background: Rectangle {
                    x: target.leftPadding
                    y: target.topPadding + (target.availableHeight - height) / 2
                    width: target.availableWidth
                    height: 4
                    radius: 2
                    color: DriverTheme.line
                    Rectangle {
                        width: parent.width * target.position
                        height: parent.height; radius: parent.radius; color: DriverTheme.green
                    }
                }
                handle: Rectangle {
                    x: target.leftPadding + target.visualPosition * (target.availableWidth - width)
                    y: target.topPadding + (target.availableHeight - height) / 2
                    width: 20; height: 20; radius: 10
                    color: target.pressed ? DriverTheme.active : DriverTheme.surface
                    border.width: target.visualFocus ? 3 : 2
                    border.color: DriverTheme.green
                }
                Binding on value {
                    value: page.vehicle.chargeTarget
                    when: !target.pressed
                    restoreMode: Binding.RestoreNone
                }
                onMoved: {
                    proposed = Math.round(value)
                    if (!pressed) commit()
                }
                onPressedChanged: if (!pressed) commit()
                function commit() {
                    if (proposed >= 0 && proposed !== page.vehicle.chargeTarget)
                        Driver.command("setChargeTarget", { value: proposed })
                    proposed = -1
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: guidance.implicitHeight + 24
                radius: 12
                color: page.parked ? DriverTheme.secondary : DriverTheme.warningFill
                Label {
                    id: guidance
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    wrapMode: Text.Wrap
                    color: page.parked ? DriverTheme.muted : DriverTheme.warning
                    text: !page.parked ? "挂 P 挡后可操作充电接口"
                        : page.vehicle.charging ? "预计 " + Math.ceil((page.vehicle.chargeTarget - page.vehicle.soc) * 36 / page.vehicle.chargePower) + " 分钟达标，暂停后可拔枪"
                        : !page.vehicle.port ? "先打开充电口，再连接充电枪"
                        : !page.vehicle.plugged ? "充电口已打开，请连接充电枪"
                        : page.reached ? "已达到目标，可拔枪或提高目标电量" : "连接就绪，可以开始充电"
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 76
                radius: 12
                color: DriverTheme.secondary
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    ColumnLayout {
                        spacing: 4
                        Label { text: "补足至目标"; color: DriverTheme.muted; font.pixelSize: 12 }
                        Label {
                            text: Math.max(0, (page.vehicle.chargeTarget - page.vehicle.soc) * 0.6).toFixed(1) + " kWh"
                            color: DriverTheme.green; font.pixelSize: 24
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: "达标后自动停止"
                        color: DriverTheme.muted
                        font.pixelSize: 13
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                DriverButton {
                    objectName: "driverChargePort"
                    Layout.fillWidth: true
                    text: page.vehicle.port ? "关闭充电口" : "打开充电口"
                    enabled: Driver.running && page.parked && !page.vehicle.plugged
                    onClicked: Driver.command("setPort", { value: !page.vehicle.port })
                }
                DriverButton {
                    objectName: "driverChargePlug"
                    Layout.fillWidth: true
                    text: page.vehicle.plugged ? "拔出充电枪" : "插入充电枪"
                    enabled: Driver.running && page.parked && page.vehicle.port && !page.vehicle.charging
                    onClicked: Driver.command("setPlugged", { value: !page.vehicle.plugged })
                }
            }
            DriverButton {
                objectName: "driverChargeStart"
                Layout.fillWidth: true
                implicitHeight: 52
                text: page.vehicle.charging ? "暂停充电" : "开始充电"
                highlighted: true
                selected: page.vehicle.charging
                enabled: Driver.running && page.parked && page.vehicle.plugged && (page.vehicle.charging || !page.reached)
                onClicked: Driver.command(page.vehicle.charging ? "stopCharging" : "startCharging")
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label { Layout.fillWidth: true; text: "虚拟补能 · 60 kWh 电池 / 55 kW 充电"; font.pixelSize: 13; color: DriverTheme.muted }
        DriverButton {
            objectName: "driverChargeAdvance"
            text: "快进 5 分钟"
            enabled: Driver.running && page.vehicle.charging
            onClicked: Driver.command("advance", { seconds: 300 })
        }
    }
}
