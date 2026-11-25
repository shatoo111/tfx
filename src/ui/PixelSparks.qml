import QtQuick

// A short burst of square sparks: call burst() with the point they leave.
Item {
    id: sparks
    property var colours: ["#fff4a8", "#ffcf3f"]
    property int count: 7
    property int size: 4
    property real reach: 26
    property int serial: 0
    function burst(x, y) {
        originX = x
        originY = y
        ++serial
    }
    property real originX: 0
    property real originY: 0
    Repeater {
        model: sparks.count
        delegate: Rectangle {
            id: bit
            required property int index
            property real travel: 1
            readonly property real angle: index / sparks.count * 2 * Math.PI + (sparks.serial * 0.7)
            visible: travel < 1
            width: sparks.size
            height: sparks.size
            color: sparks.colours[index % sparks.colours.length]
            x: sparks.originX - width / 2 + Math.cos(angle) * sparks.reach * travel
            y: sparks.originY - height / 2 + Math.sin(angle) * sparks.reach * travel + 18 * travel * travel
            opacity: Math.ceil((1 - travel) * 4) / 4
            NumberAnimation { id: flight; target: bit; property: "travel"; from: 0; to: 1; duration: 420; easing.type: Easing.OutQuad }
            Connections {
                target: sparks
                function onSerialChanged() { flight.restart() }
            }
        }
    }
}
