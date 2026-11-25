import QtQuick

// Prints its text letter by letter, with a block cursor, whenever it changes.
Text {
    id: typed
    property string full
    property bool animating: true
    property int shown: 0
    text: animating ? full.slice(0, shown) + (shown < full.length || blink.on ? "█" : " ") : full
    onFullChanged: shown = animating ? 0 : full.length
    Timer {
        interval: 28
        repeat: true
        running: typed.animating && typed.visible && typed.shown < typed.full.length
        onTriggered: ++typed.shown
    }
    Timer {
        id: blink
        property bool on: true
        interval: 560
        repeat: true
        running: typed.animating && typed.visible && typed.shown >= typed.full.length
        onTriggered: on = !on
    }
}
