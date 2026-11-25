import QtQuick
import QtQuick.Controls

// Level select: every theme is a tile, and the chosen hero hops to the tile you pick.
Item {
    id: grid
    readonly property TfxColors colors: TfxColors {}
    property var names: []
    property int current: 0
    property int hero: 0
    property bool animating: true
    readonly property int columns: width < 560 ? 2 : 5
    readonly property int gap: 10
    readonly property int tileHeight: 60
    readonly property real tileWidth: (width - gap * (columns - 1)) / columns
    signal activated(int index)
    implicitHeight: Math.ceil(names.length / columns) * (tileHeight + gap) - gap

    function place(index) {
        return Qt.point((index % columns) * (tileWidth + gap) + tileWidth - 42, Math.floor(index / columns) * (tileHeight + gap) + tileHeight - 32 - 3)
    }
    Repeater {
        model: grid.names
        delegate: AbstractButton {
            id: tile
            required property int index
            required property string modelData
            readonly property bool chosen: grid.current === index
            objectName: "theme" + index
            x: (index % grid.columns) * (grid.tileWidth + grid.gap)
            y: Math.floor(index / grid.columns) * (grid.tileHeight + grid.gap)
            width: grid.tileWidth
            height: grid.tileHeight
            hoverEnabled: true
            checkable: false
            onClicked: grid.activated(index)
            Accessible.role: Accessible.RadioButton
            Accessible.name: modelData
            Accessible.checked: chosen
            background: PixelFrame {
                fill: tile.chosen ? "#26301b" : tile.hovered ? grid.colors.raised : grid.colors.panel
                edge: tile.visualFocus ? grid.colors.cyan : tile.chosen ? grid.colors.acid : tile.hovered ? grid.colors.muted : grid.colors.line
            }
            contentItem: Item {
                Image {
                    id: icon
                    x: 12
                    y: (parent.height - height) / 2 + (tile.hovered && grid.animating ? bounce.shift : 0)
                    width: 32
                    height: 32
                    smooth: false
                    source: "qrc:/assets/pixel/theme-" + tile.index + ".png"
                    SequentialAnimation {
                        id: bounce
                        property real shift: 0
                        loops: Animation.Infinite
                        running: tile.hovered && grid.animating
                        NumberAnimation { target: bounce; property: "shift"; to: -5; duration: 160; easing.type: Easing.OutQuad }
                        NumberAnimation { target: bounce; property: "shift"; to: 0; duration: 160; easing.type: Easing.InQuad }
                    }
                }
                Text {
                    anchors { left: icon.right; leftMargin: 12; right: parent.right; rightMargin: tile.chosen ? 46 : 10; verticalCenter: parent.verticalCenter }
                    text: tile.modelData
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: tile.chosen ? grid.colors.acid : tile.hovered ? grid.colors.text : grid.colors.muted
                    font.family: grid.colors.monoFont
                    font.pixelSize: 12
                    font.bold: true
                }
            }
        }
    }

    // The hero: a straight way between the tiles plus a parabola makes the hop.
    Item {
        id: jumper
        property point from
        property point to
        property real way: 1
        readonly property real hop: 4 * way * (1 - way)
        x: from.x + (to.x - from.x) * way
        y: from.y + (to.y - from.y) * way - hop * 46
        HeroSprite {
            factor: 2
            hero: grid.hero
            animating: grid.animating
            pose: travel.running ? "jump" : "idle"
            mirrored: jumper.to.x < jumper.from.x
        }
        NumberAnimation {
            id: travel
            target: jumper
            property: "way"
            from: 0; to: 1
            duration: 460
            onFinished: dust.burst(16, 30)
        }
        PixelSparks { id: dust; width: 32; height: 32; size: 3; reach: 18; colours: ["#e9eef5", "#b9c1cb"] }
    }
    function settle(hopping) {
        const target = place(current)
        if (hopping && animating) {
            jumper.from = Qt.point(jumper.x, jumper.y)
            jumper.to = target
            travel.restart()
        } else {
            travel.stop()
            jumper.from = jumper.to = target
            jumper.way = 1
        }
    }
    onCurrentChanged: settle(true)
    onWidthChanged: settle(false)
    Component.onCompleted: settle(false)
}
