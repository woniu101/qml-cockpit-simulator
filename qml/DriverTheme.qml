pragma Singleton
import QtQuick

QtObject {
    property bool night: false
    readonly property color top: night ? "#122925" : "#f1f5f0"
    readonly property color bottom: night ? "#1c3530" : "#dfe9e1"
    readonly property color surface: night ? "#203a32" : "#ffffff"
    readonly property color secondary: night ? "#29483e" : "#eaf1eb"
    readonly property color active: night ? "#345d4b" : "#dcece2"
    readonly property color ink: night ? "#e5f0e8" : "#223e35"
    readonly property color muted: night ? "#a6beb0" : "#698073"
    readonly property color green: night ? "#9bdac0" : "#347d68"
    readonly property color line: night ? "#3a5949" : "#d2e0d6"
    readonly property color warning: night ? "#f0c18d" : "#88581b"
    readonly property color warningFill: night ? "#493829" : "#f8edda"
    readonly property color buttonFill: night ? "#345647" : "#dce9df"
    readonly property color buttonBorder: night ? "#779f8a" : "#8cae9a"
    readonly property color buttonHover: night ? "#416a56" : "#c7ddce"
    readonly property color buttonStrong: night ? "#79bd9f" : "#347d68"
    readonly property color buttonStrongText: night ? "#102d22" : "#f5faf7"
    readonly property color buttonDisabled: night ? "#293e34" : "#e4eae6"
    readonly property color buttonDisabledText: night ? "#8fa699" : "#738579"
}
