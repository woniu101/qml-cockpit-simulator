import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: page
    implicitHeight: width < 850 ? 680 : 610
    spacing: 16
    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label { text: "联调工作台"; font.pixelSize: 27; font.weight: Font.DemiBold; color: DriverTheme.ink }
            Label { text: Driver.serviceName; font.pixelSize: 13; color: DriverTheme.muted }
        }
        Item { Layout.fillWidth: true }
        DriverButton { text: "高级控制台"; onClicked: Driver.openConsole() }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: [{ title: "车机连接", value: Driver.cockpitCount + " 个", warning: Driver.cockpitCount === 0 },
                { title: "遥测", value: Driver.telemetryPaused ? "暂停" : "运行中", warning: Driver.telemetryPaused },
                { title: "指令处理", value: Driver.faults.reject ? "拒绝请求" : Driver.faults.drop ? "丢弃请求" : Driver.faults.delayMs + " ms 延迟",
                  warning: Driver.faults.reject || Driver.faults.drop || Driver.faults.delayMs > 0 }]
            Rectangle {
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                implicitHeight: 83
                radius: 13
                color: modelData.warning ? DriverTheme.warningFill : DriverTheme.secondary
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 13
                    spacing: 4
                    Label { text: modelData.title; font.pixelSize: 12; color: DriverTheme.muted }
                    Label { text: modelData.value; font.pixelSize: 20; color: modelData.warning ? DriverTheme.warning : DriverTheme.green }
                }
            }
        }
    }

    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 8
        DriverButton {
            objectName: "driverPauseTelemetry"
            text: Driver.telemetryPaused ? "恢复遥测" : "暂停遥测"
            selected: Driver.telemetryPaused
            enabled: Driver.running
            onClicked: Driver.command("faults", { pauseTelemetry: !Driver.telemetryPaused })
        }
        DriverButton {
            text: "拒绝请求"
            selected: Driver.faults.reject
            onClicked: Driver.command("faults", { reject: !Driver.faults.reject })
        }
        DriverButton {
            text: "丢弃请求"
            selected: Driver.faults.drop
            onClicked: Driver.command("faults", { drop: !Driver.faults.drop })
        }
        DriverButton {
            text: "延迟 1 秒"
            selected: Driver.faults.delayMs > 0
            onClicked: Driver.command("faults", { delayMs: Driver.faults.delayMs > 0 ? 0 : 1000 })
        }
        DriverButton {
            text: "清除故障"
            enabled: Driver.telemetryPaused || Driver.faults.reject || Driver.faults.drop || Driver.faults.delayMs > 0
            onClicked: Driver.command("faults", { pauseTelemetry: false, reject: false, drop: false, delayMs: 0 })
        }
        DriverButton {
            text: "断开车机"
            enabled: Driver.running && Driver.cockpitCount > 0
            onClicked: Driver.command("disconnect")
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label { Layout.preferredWidth: 110; text: "时间"; color: DriverTheme.muted; font.pixelSize: 12 }
        Label { Layout.preferredWidth: 86; text: "类别"; color: DriverTheme.muted; font.pixelSize: 12 }
        Label { Layout.fillWidth: true; text: "事件详情 · 最新在前"; color: DriverTheme.muted; font.pixelSize: 12 }
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 240
        radius: 14
        color: DriverTheme.surface
        border.color: DriverTheme.line
        ListView {
            id: log
            anchors.fill: parent
            anchors.margins: 10
            clip: true
            model: Driver.eventRows
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property var modelData
                required property int index
                width: log.width - 12
                height: Math.max(42, detail.implicitHeight + 18)
                radius: 7
                color: index % 2 ? "transparent" : DriverTheme.secondary
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label { Layout.preferredWidth: 105; Layout.alignment: Qt.AlignTop; text: modelData.time; color: DriverTheme.muted; font.pixelSize: 12 }
                    Label { Layout.preferredWidth: 82; Layout.alignment: Qt.AlignTop; text: modelData.category; color: DriverTheme.green; font.pixelSize: 12 }
                    Label {
                        id: detail
                        Layout.fillWidth: true
                        text: modelData.detail
                        wrapMode: Text.Wrap
                        textFormat: Text.PlainText
                        color: DriverTheme.ink
                        font.pixelSize: 13
                    }
                }
            }
        }
        Label {
            anchors.centerIn: parent
            visible: log.count === 0
            text: "操作车辆后，指令与连接事件将在这里显示"
            color: DriverTheme.muted
        }
    }
    Label {
        Layout.fillWidth: true
        text: Driver.message
        wrapMode: Text.Wrap
        color: Driver.warning ? DriverTheme.warning : DriverTheme.muted
        font.pixelSize: 13
    }
}
