import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool selected: false
    property bool quiet: false
    implicitHeight: 44
    padding: 10
    hoverEnabled: true
    readonly property bool strong: enabled && (selected || highlighted || down)

    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? DriverTheme.buttonDisabledText
            : control.strong ? DriverTheme.buttonStrongText : DriverTheme.ink
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 12
        color: !control.enabled ? DriverTheme.buttonDisabled
            : control.strong ? DriverTheme.buttonStrong
            : control.hovered ? DriverTheme.buttonHover
            : control.quiet ? DriverTheme.surface : DriverTheme.buttonFill
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus || control.strong ? DriverTheme.green
            : !control.enabled ? (DriverTheme.night ? "#526d5e" : "#aebfb4") : DriverTheme.buttonBorder
        Behavior on color { ColorAnimation { duration: 100 } }
    }
}
