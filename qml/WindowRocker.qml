import QtQuick

DriverButton {
    required property int index
    required property bool opening
    property var hold: 0
    objectName: (opening ? "windowDown" : "windowUp") + index
    text: opening ? "↓ 降窗" : "↑ 升窗"
    selected: down && hold !== 0
    enabled: Driver.running
    onPressed: hold = Driver.beginWindow(index, opening)
    onReleased: stop()
    onCanceled: stop()

    function stop() {
        Driver.endWindow(index, hold)
        hold = 0
    }
}
