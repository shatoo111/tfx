import QtQuick
import "LandData.js" as LandData

// The running world on top of the settings: the land of the chosen theme scrolls by, its name
// runs along the sky, and the cat, the knight and the robot leap over whatever is on the road.
Item {
    id: world
    readonly property TfxColors colors: TfxColors {}
    property int theme: 0
    property var phrases: []
    property bool animating: true
    readonly property int factor: 3
    readonly property int headlineSize: width < 700 ? 32 : 48
    readonly property real speed: 44
    property real roadX: 0
    property int lap: 0
    property var taken: ({})
    property int takenRevision: 0
    property bool frontIsA: true
    property int landA: 0
    property int landB: 0
    property real mix: 1
    clip: true

    Component.onCompleted: landA = landB = theme
    onThemeChanged: {
        if (frontIsA) landB = theme
        else landA = theme
        frontIsA = !frontIsA
        taken = ({})
        ++takenRevision
        if (animating) dissolve.restart()
        else mix = 1
    }
    NumberAnimation { id: dissolve; target: world; property: "mix"; from: 0; to: 1; duration: 620 }
    function share(isA) {
        const amount = isA === frontIsA ? mix : 1 - mix
        return Math.round(amount * 8) / 8
    }
    NumberAnimation on roadX {
        id: rolling
        from: 0; to: LandData.length
        duration: LandData.length / world.speed * 1000
        loops: Animation.Infinite
        running: world.animating
    }
    onRoadXChanged: {
        if (roadX < lastRoad) ++lap
        lastRoad = roadX
    }
    property real lastRoad: 0

    PixelLand { anchors.fill: parent; part: "sky"; theme: world.landA; factor: world.factor; animating: world.animating && visible; opacity: world.share(true); visible: opacity > 0 }
    PixelLand { anchors.fill: parent; part: "sky"; theme: world.landB; factor: world.factor; animating: world.animating && visible; opacity: world.share(false); visible: opacity > 0 }

    // The headline passes behind the hills.
    Item {
        id: ticker
        y: 16
        height: world.headlineSize * 1.4
        width: parent.width
        readonly property string phrase: world.phrases.map(line => line.toUpperCase() + " <font color='#b8ff5a'>/</font>&nbsp;").join("")
        Row {
            id: phrases
            spacing: 0
            Repeater {
                model: Math.max(2, Math.ceil(world.width / Math.max(1, sample.implicitWidth)) + 1)
                delegate: Text {
                    textFormat: Text.StyledText
                    text: ticker.phrase
                    color: world.colors.text
                    font.family: world.colors.pixelFont
                    font.pixelSize: world.headlineSize
                    renderType: Text.CurveRendering
                }
            }
            NumberAnimation on x {
                from: 0; to: -sample.implicitWidth
                duration: Math.max(1000, sample.implicitWidth * 16)
                loops: Animation.Infinite
                running: world.animating && sample.implicitWidth > 0
            }
        }
        Text { id: sample; visible: false; textFormat: Text.StyledText; text: ticker.phrase; font.family: world.colors.pixelFont; font.pixelSize: world.headlineSize }
    }

    PixelLand { anchors.fill: parent; part: "hills"; theme: world.landA; factor: world.factor; animating: world.animating && visible; roadX: visible ? world.roadX : 0; opacity: world.share(true); visible: opacity > 0 }
    PixelLand { anchors.fill: parent; part: "hills"; theme: world.landB; factor: world.factor; animating: world.animating && visible; roadX: visible ? world.roadX : 0; opacity: world.share(false); visible: opacity > 0 }

    readonly property real floor: height - LandData.ground * factor
    readonly property var info: LandData.lands[theme]

    // Coins of the current land, one set per repeat of the road.
    Item {
        x: -world.roadX * world.factor
        Repeater {
            model: Math.ceil(world.width / world.factor / LandData.length) + 1
            delegate: Item {
                id: repeatOfRoad
                required property int index
                Repeater {
                    model: world.info.coins
                    delegate: CoinSprite {
                        required property int index
                        required property var modelData
                        factor: world.factor
                        spinning: world.animating
                        x: (repeatOfRoad.index * LandData.length + modelData[0]) * world.factor
                        y: world.floor - (modelData[1] + 10) * world.factor
                        visible: world.takenRevision >= 0 && !world.taken[(world.lap + repeatOfRoad.index) + ":" + index]
                    }
                }
            }
        }
    }
    PixelSparks { id: sparks; anchors.fill: parent; size: 3; reach: 30 }

    Repeater {
        id: crew
        model: 3
        delegate: Item {
            id: runner
            required property int index
            readonly property real home: world.width < 700 ? world.width * (0.6 - index * 0.2) : world.width * (0.42 - index * 0.085)
            property real drift: 0
            property real lift: 0
            property real apex: 22
            property real air: 1
            readonly property bool flying: leap.running
            x: home + drift
            y: world.floor - 15 * world.factor - lift * world.factor
            Behavior on drift { NumberAnimation { duration: 2000; easing.type: Easing.InOutQuad } }
            function jump(height, seconds) {
                if (leap.running || !world.animating) return
                apex = height
                air = seconds
                leap.restart()
            }
            SequentialAnimation {
                id: leap
                NumberAnimation { target: runner; property: "lift"; from: 0; to: runner.apex; duration: runner.air * 500; easing.type: Easing.OutQuad }
                NumberAnimation { target: runner; property: "lift"; to: 0; duration: runner.air * 500; easing.type: Easing.InQuad }
            }
            Rectangle { x: 2 * world.factor; y: (15 + runner.lift) * world.factor; width: 12 * world.factor; height: world.factor; color: "#60000000" }
            HeroSprite { hero: runner.index; factor: world.factor; animating: world.animating; pose: !world.animating ? "idle" : runner.flying ? "jump" : "run" }
        }
    }
    Timer {
        interval: 40
        repeat: true
        running: world.animating
        property int beat: 0
        onTriggered: {
            ++beat
            const size = LandData.length
            for (let i = 0; i < crew.count; ++i) {
                const runner = crew.itemAt(i)
                if (!runner) continue
                const centre = world.roadX + runner.x / world.factor + 8
                let near = false
                for (const hurdle of world.info.hurdles) {
                    const ahead = ((hurdle[0] - centre) % size + size) % size
                    if (ahead < 80) near = true
                    const wait = ahead / world.speed - hurdle[1] / 2
                    if (wait >= 0 && wait < 0.06) runner.jump(hurdle[2], hurdle[1])
                }
                if ((beat + i * 37) % 90 === 0) {
                    runner.drift = (Math.random() - 0.5) * 36
                    if (!near && Math.random() < 0.5) runner.jump(14 + Math.random() * 8, 0.8)
                }
                for (let k = 0; k < world.info.coins.length; ++k) {
                    const coin = world.info.coins[k], gap = coin[0] + 5 - centre
                    const copy = Math.round(-gap / size), distance = gap + copy * size
                    if (Math.abs(distance) > 9 || Math.abs(coin[1] - 3 - runner.lift) > 12) continue
                    const key = (world.lap + copy) + ":" + k
                    if (copy < 0 || world.taken[key]) continue
                    world.taken[key] = true
                    ++world.takenRevision
                    sparks.burst((coin[0] + 5 + copy * size - world.roadX) * world.factor, world.floor - (coin[1] + 5) * world.factor)
                }
            }
        }
    }
    TapHandler {
        onTapped: {
            for (let i = 0; i < crew.count; ++i) crew.itemAt(i).jump(26, 0.95)
        }
    }
}
