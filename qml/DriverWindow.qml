import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    title: "QML Cockpit Simulator · 整车模拟器"
    width: 1160
    height: 820
    minimumWidth: 720
    minimumHeight: 560
    visible: true
    font.family: "Microsoft YaHei UI"
    font.pixelSize: 15
    property int page: 0
    readonly property bool immersive: visibility === Window.FullScreen
    readonly property bool driveKeysEnabled: page === 0 && !vehicleDrawer.visible && !keySettings.visible
    onDriveKeysEnabledChanged: if (!driveKeysEnabled) Driver.releaseInputs()
    onPageChanged: scroll.contentItem.contentY = 0
    onClosing: Driver.releaseInputs()

    function toggleFullscreen() {
        if (immersive)
            showNormal()
        else if (page === 0)
            showFullScreen()
    }

    Shortcut { sequence: "F1"; enabled: !keySettings.visible && !vehicleDrawer.visible; onActivated: keySettings.open() }
    Shortcut { sequence: "F11"; enabled: window.driveKeysEnabled; onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "Esc"; enabled: window.immersive && window.driveKeysEnabled; onActivated: window.showNormal() }

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0; color: DriverTheme.top }
            GradientStop { position: 1; color: DriverTheme.bottom }
        }
    }

    header: Rectangle {
        visible: !window.immersive
        implicitHeight: visible ? 62 : 0
        color: DriverTheme.surface
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 8
            Image {
                source: "qrc:/branding/app.ico"
                sourceSize.width: 32
                sourceSize.height: 32
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
            }
            Label {
                text: "整车模拟器"
                font.pixelSize: 21
                font.weight: Font.DemiBold
                color: DriverTheme.ink
                Layout.rightMargin: 10
            }
            Repeater {
                model: ["驾驶", "车身", "补能", "联调"]
                DriverButton {
                    required property string modelData
                    required property int index
                    objectName: "driverTab" + index
                    Layout.preferredWidth: 68
                    text: modelData
                    quiet: true
                    selected: window.page === index
                    onClicked: window.page = index
                }
            }
            Item { Layout.fillWidth: true }
            DriverButton {
                objectName: "driverThemeToggle"
                text: DriverTheme.night ? "切换日间" : "切换夜间"
                onClicked: DriverTheme.night = !DriverTheme.night
            }
            DriverButton {
                text: "设置"
                onClicked: keySettings.open()
            }
        }
    }

    DriverPage {
        anchors.fill: parent
        visible: window.page === 0
        fullScreen: window.immersive
        onFullScreenRequested: window.toggleFullscreen()
        onBodyRequested: vehicleDrawer.open()
        onKeysRequested: keySettings.open()
    }

    ScrollView {
        id: scroll
        objectName: "driverPageScroll"
        visible: window.page !== 0
        anchors.fill: parent
        anchors.margins: window.width < 960 ? 16 : 24
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        clip: true
        Loader {
            width: scroll.availableWidth
            height: Math.max(scroll.availableHeight, implicitHeight)
            sourceComponent: [null, body, charge, lab][window.page]
        }
    }
    VehicleQuickPanel { id: vehicleDrawer }

    KeySettings {
        id: keySettings
        anchors.centerIn: Overlay.overlay
    }

    Component { id: body; BodyPage {} }
    Component { id: charge; ChargePage {} }
    Component { id: lab; DiagnosticsPage {} }
}
