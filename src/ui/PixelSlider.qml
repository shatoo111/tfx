import QtQuick
import QtQuick.Controls

// The handle is a hero who runs along the track while you drag.
Slider {
    id: slider
    readonly property TfxColors colors: TfxColors {}
    property bool centered: false
    property int hero: 0
    property bool leftward: false
    property real previous: visualPosition
    implicitHeight: 44
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    onVisualPositionChanged: {
        if (Math.abs(visualPosition - previous) > 0.0005) leftward = visualPosition < previous
        previous = visualPosition
        if (pressed) running.restart()
    }
    Timer { id: running; interval: 160 }
    background: Item {
        readonly property real from: slider.centered ? Math.min(0.5, slider.visualPosition) : 0
        readonly property real span: slider.centered ? Math.abs(slider.visualPosition - 0.5) : slider.visualPosition
        Image {
            y: slider.height - 10
            width: parent.width
            height: 6
            fillMode: Image.TileHorizontally
            horizontalAlignment: Image.AlignLeft
            smooth: false
            source: "qrc:/assets/pixel/track.png"
        }
        Item {
            x: parent.width * parent.from
            y: slider.height - 10
            width: parent.width * parent.span
            height: 6
            clip: true
            Image {
                x: -parent.x
                width: slider.width
                height: 6
                fillMode: Image.TileHorizontally
                horizontalAlignment: Image.AlignLeft
                smooth: false
                source: "qrc:/assets/pixel/track-on.png"
            }
        }
    }
    handle: Item {
        x: slider.visualPosition * (slider.availableWidth - width)
        y: slider.height - 10 - height + 2
        implicitWidth: 32
        implicitHeight: 32
        HeroSprite {
            factor: 2
            hero: slider.hero
            mirrored: slider.leftward
            pose: running.running ? "run" : "idle"
        }
        Rectangle { visible: slider.visualFocus; x: -2; y: parent.height + 4; width: 36; height: 3; color: slider.colors.cyan }
    }
}
