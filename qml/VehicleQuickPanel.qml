import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel
    readonly property var vehicle: Driver.state
    readonly property bool parked: vehicle.gear === "P"
    readonly property bool stationaryControls: Driver.running && parked

    parent: Overlay.overlay
    width: Math.min(484, parent.width - 32)
    x: parent.width - width - 16
    y: (parent.height - height) / 2
    height: Math.min(parent.height - 32, implicitContentHeight + 2 * padding)
    padding: 20
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onAboutToHide: Driver.releaseInputs()

    Overlay.modal: Rectangle { color: "#5520322b" }
    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 } }

    background: Rectangle {
        radius: 21
        color: DriverTheme.surface
        border.color: DriverTheme.buttonBorder
    }

    contentItem: ColumnLayout {
        spacing: 14
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 4
                Label {
                    text: "车辆操作"
                    color: DriverTheme.ink
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }
                Label {
                    text: panel.parked ? "P 挡 · 驻车" : panel.vehicle.gear + " 挡"
                    color: panel.parked ? DriverTheme.green : DriverTheme.muted
                    font.pixelSize: 13
                }
            }
            Item { Layout.fillWidth: true }
            DriverButton {
                objectName: "driverCloseDrawer"
                text: "关闭"
                quiet: true
                implicitHeight: 40
                onClicked: panel.close()
            }
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: sections.implicitHeight
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            clip: true

            ColumnLayout {
                id: sections
                width: scroll.availableWidth
                spacing: 12
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: "车窗"
                        color: DriverTheme.ink
                        font.pixelSize: 17
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: "长按升降，松手即停"
                        color: DriverTheme.muted
                        font.pixelSize: 12
                    }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 10
                    Repeater {
                        model: ["左前窗", "右前窗", "左后窗", "右后窗"]
                        Rectangle {
                            id: windowCard
                            required property int index
                            required property string modelData
                            readonly property real opening: panel.vehicle.windows[index]
                            readonly property bool moving: panel.vehicle.windowMoving[index]
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            implicitHeight: controls.implicitHeight + 22
                            radius: 12
                            color: DriverTheme.top
                            border.color: moving ? DriverTheme.green : DriverTheme.line
                            ColumnLayout {
                                id: controls
                                anchors.fill: parent
                                anchors.margins: 11
                                spacing: 8
                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        Layout.fillWidth: true
                                        text: windowCard.modelData
                                        color: DriverTheme.ink
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }
                                    Label {
                                        text: Math.round(windowCard.opening) + "%"
                                        color: DriverTheme.green
                                        font.family: "Segoe UI"
                                        font.pixelSize: 18
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: 4
                                    radius: 2
                                    color: DriverTheme.line
                                    Rectangle {
                                        width: parent.width * windowCard.opening / 100
                                        height: parent.height
                                        radius: parent.radius
                                        color: DriverTheme.green
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    WindowRocker {
                                        index: windowCard.index
                                        opening: false
                                        Layout.fillWidth: true
                                        implicitHeight: 40
                                        padding: 6
                                    }
                                    WindowRocker {
                                        index: windowCard.index
                                        opening: true
                                        Layout.fillWidth: true
                                        implicitHeight: 40
                                        padding: 6
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: DriverTheme.line
                }
                Label {
                    text: "门锁与后视镜"
                    color: DriverTheme.ink
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    StateControl {
                        actionName: "quickLockAction"
                        label: "车门锁"
                        stateText: panel.vehicle.locked ? "已锁定" : "已解锁"
                        actionText: panel.vehicle.locked ? "解锁车辆" : "锁定车辆"
                        onActivated: Driver.command("setLocked", { value: !panel.vehicle.locked })
                    }
                    StateControl {
                        actionName: "quickMirrorAction"
                        label: "外后视镜"
                        stateText: panel.vehicle.mirrors ? "已折叠" : "已展开"
                        actionText: panel.vehicle.mirrors ? "展开后视镜" : "折叠后视镜"
                        onActivated: Driver.command("setMirrors", { value: !panel.vehicle.mirrors })
                    }
                }
                Label {
                    visible: !panel.parked
                    Layout.fillWidth: true
                    text: "挂入 P 挡后可操作门锁和后视镜"
                    wrapMode: Text.Wrap
                    color: DriverTheme.warning
                    font.pixelSize: 12
                }
            }
        }
    }

    component StateControl: ColumnLayout {
        property alias actionName: action.objectName
        required property string label
        required property string stateText
        required property string actionText
        signal activated()
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        spacing: 7
        RowLayout {
            Layout.fillWidth: true
            Label { Layout.fillWidth: true; text: label; color: DriverTheme.ink; font.pixelSize: 14 }
            Label { text: stateText; color: DriverTheme.green; font.pixelSize: 12 }
        }
        DriverButton {
            id: action
            Layout.fillWidth: true
            implicitHeight: 40
            text: actionText
            enabled: panel.stationaryControls
            onClicked: activated()
        }
    }
}
