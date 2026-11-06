import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: settings
    width: Math.min(540, Overlay.overlay.width - 36)
    padding: 20
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onAboutToHide: Driver.captureKey("")
    background: Rectangle {
        radius: 18
        color: DriverTheme.surface
        border.color: DriverTheme.line
    }

    contentItem: ColumnLayout {
        spacing: 11
        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: "驾驶按键"
                color: DriverTheme.ink
                font.pixelSize: 24
                font.weight: Font.DemiBold
            }
            DriverButton {
                objectName: "driverCloseKeys"
                text: "完成"
                onClicked: settings.close()
            }
        }
        Label {
            Layout.fillWidth: true
            text: "点击键位后按下新按键。设置自动保存，仅在驾驶主屏且模拟器获得焦点时生效。"
            wrapMode: Text.Wrap
            color: DriverTheme.muted
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 7
            columnSpacing: 10
            Repeater {
                model: [{ action: "brake", label: "刹车" }, { action: "throttle", label: "油门" },
                    { action: "P", label: "P 挡" }, { action: "R", label: "R 挡" },
                    { action: "N", label: "N 挡" }, { action: "D", label: "D 挡" },
                    { action: "windowUp", label: "主驾升窗" }, { action: "windowDown", label: "主驾降窗" }]
                Rectangle {
                    id: bindingRow
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    implicitHeight: 48
                    radius: 9
                    color: DriverTheme.secondary
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 5
                        Label { Layout.fillWidth: true; text: bindingRow.modelData.label; color: DriverTheme.ink }
                        DriverButton {
                            objectName: "driverBind" + bindingRow.modelData.action
                            Layout.preferredWidth: 82
                            implicitHeight: 36
                            selected: Driver.captureAction === bindingRow.modelData.action
                            text: selected ? "等待按键" : Driver.keyLabels[bindingRow.modelData.action]
                            onClicked: Driver.captureKey(bindingRow.modelData.action)
                        }
                    }
                }
            }
        }
        Label {
            objectName: "driverKeyFeedback"
            Layout.fillWidth: true
            text: Driver.keyFeedback || (Driver.captureAction ? "按 Esc 取消录入" : "F1 查看按键 · F11 切换全屏")
            color: Driver.keyFeedback ? DriverTheme.warning : DriverTheme.muted
            wrapMode: Text.Wrap
            font.pixelSize: 13
        }
        RowLayout {
            DriverButton {
                objectName: "driverKeyArrows"
                text: "恢复默认"
                onClicked: Driver.applyKeyPreset(false)
            }
            DriverButton {
                objectName: "driverKeyWS"
                text: "W / S 预设"
                onClicked: Driver.applyKeyPreset(true)
            }
        }
    }
}
