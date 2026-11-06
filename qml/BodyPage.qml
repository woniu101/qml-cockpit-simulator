import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    readonly property var vehicle: Driver.state
    readonly property bool parked: vehicle.gear === "P"
    readonly property bool showPlan: width >= 760
    implicitHeight: 610
    spacing: 16

    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label { text: "车身控制"; font.pixelSize: 27; color: DriverTheme.ink; font.weight: Font.DemiBold }
            Label { text: "长按升降 · 松手即停"; color: DriverTheme.muted; font.pixelSize: 13 }
        }
        Item { Layout.fillWidth: true }
        DriverButton {
            text: page.vehicle.locked ? "解锁车辆" : "锁定车辆"
            selected: page.vehicle.locked
            enabled: Driver.running && page.parked
            onClicked: Driver.command("setLocked", { value: !page.vehicle.locked })
        }
    }

    GridLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        columns: page.showPlan ? 3 : 2
        rowSpacing: 14
        columnSpacing: 18
        VehiclePlan {
            visible: page.showPlan
            Layout.row: 0
            Layout.column: 1
            Layout.rowSpan: 2
            Layout.fillWidth: true
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            vehicle: page.vehicle
        }
        Repeater {
            model: 4
            Rectangle {
                id: card
                required property int index
                readonly property real actual: page.vehicle.windows[index]
                readonly property bool moving: page.vehicle.windowMoving[index]
                Layout.row: Math.floor(index / 2)
                Layout.column: page.showPlan && index % 2 ? 2 : index % 2
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 300
                Layout.minimumHeight: 180
                radius: 16
                color: DriverTheme.surface
                border.color: moving ? DriverTheme.green : DriverTheme.line
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 9
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: ["左前窗", "右前窗", "左后窗", "右后窗"][card.index]
                            color: DriverTheme.ink
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }
                        Label {
                            text: Math.round(card.actual) + "%"
                            color: DriverTheme.green
                            font.pixelSize: 28
                            font.family: "Segoe UI"
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 5
                        radius: 2.5
                        color: DriverTheme.line
                        Rectangle {
                            width: parent.width * card.actual / 100
                            height: parent.height
                            radius: parent.radius
                            color: DriverTheme.green
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: card.moving ? "执行中 · 目标 " + page.vehicle.targets[card.index] + "%"
                            : card.actual === 0 ? "车窗全关" : card.actual === 100 ? "车窗全开" : "部分开启"
                        color: DriverTheme.muted
                        font.pixelSize: 13
                    }
                    Item { Layout.fillHeight: true }
                    RowLayout {
                        Layout.fillWidth: true
                        WindowRocker { index: card.index; opening: false; Layout.fillWidth: true }
                        WindowRocker { index: card.index; opening: true; Layout.fillWidth: true }
                        DriverButton {
                            Layout.fillWidth: true
                            text: page.vehicle.doors[card.index] ? "关门" : "开门"
                            enabled: Driver.running && page.parked
                            onClicked: Driver.command("setDoor", { index: card.index, open: !page.vehicle.doors[card.index] })
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: [{ state: "hood", action: "setHood", open: "打开前备厢", close: "关闭前备厢" },
                { state: "hatch", action: "setHatch", open: "打开后备厢", close: "关闭后备厢" },
                { state: "mirrors", action: "setMirrors", open: "折叠后视镜", close: "展开后视镜" }]
            DriverButton {
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: 48
                selected: page.vehicle[modelData.state]
                text: page.vehicle[modelData.state] ? modelData.close : modelData.open
                enabled: Driver.running && page.parked
                onClicked: Driver.command(modelData.action, { value: !page.vehicle[modelData.state] })
            }
        }
    }
    Label {
        text: page.parked ? "车门、备厢与后视镜可操作" : "当前 " + page.vehicle.gear + " 挡 · 挂 P 挡后可操作车门、备厢与后视镜"
        color: page.parked ? DriverTheme.muted : DriverTheme.warning
        font.pixelSize: 13
    }
}
