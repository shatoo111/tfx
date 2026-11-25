import QtQuick

// One frame of a hero: the cat, the knight or the robot.
Item {
    id: sprite
    property int hero: 0
    property string pose: "idle" // idle, run, jump
    property bool mirrored: false
    property bool animating: true
    property int factor: 3
    property int stride: 0
    readonly property int frame: pose === "jump" ? 3 : pose === "run" ? 1 + stride % 2 : 0
    width: 16 * factor
    height: 16 * factor
    Timer {
        interval: 100
        repeat: true
        running: sprite.animating && sprite.pose === "run" && sprite.visible
        onTriggered: ++sprite.stride
    }
    // Separate pictures instead of a clipped sheet, so that sprites share one draw call.
    Image {
        anchors.fill: parent
        smooth: false
        mirror: sprite.mirrored
        source: "qrc:/assets/pixel/hero-" + sprite.hero + "-" + sprite.frame + ".png"
    }
}
