import QtQuick

// A hero strolling along the top edge of a card: runs, stops to rest, hops, turns at the ends.
// A click makes the hero jump with a heart.
Item {
    id: walker
    property int hero: 0
    property real ledge: 0
    property bool animating: true
    property bool leftward: false
    property string mood: "idle"
    property real spot: 0.2
    readonly property int size: 32
    readonly property real room: Math.max(0, ledge - size - 8)
    y: -size + 3
    x: 4 + spot * room
    width: size
    height: size
    visible: ledge > 120

    function decide() {
        if (!animating) return
        if (Math.random() < 0.3 && mood === "run") {
            mood = "idle"
            pause.interval = 900 + Math.random() * 1700
            pause.restart()
            return
        }
        if (leftward ? spot < 0.04 : spot > 0.96) leftward = !leftward
        const goal = Math.max(0, Math.min(1, spot + (leftward ? -1 : 1) * (0.2 + Math.random() * 0.5)))
        mood = "run"
        stroll.from = spot
        stroll.to = goal
        stroll.duration = Math.max(120, Math.abs(goal - spot) * room / (64 + hero * 10) * 1000)
        stroll.restart()
        if (Math.random() < 0.4) leap.restart()
    }
    onAnimatingChanged: {
        if (animating) decide()
        else { stroll.stop(); pause.stop(); leap.stop(); body.y = 0; mood = "idle" }
    }
    Component.onCompleted: { spot = 0.15 + hero * 0.25; if (animating) decide() }

    NumberAnimation { id: stroll; target: walker; property: "spot"; onFinished: walker.decide() }
    Timer { id: pause; onTriggered: { walker.mood = "run"; walker.decide() } }
    SequentialAnimation {
        id: leap
        NumberAnimation { target: body; property: "y"; from: 0; to: -22; duration: 210; easing.type: Easing.OutQuad }
        NumberAnimation { target: body; property: "y"; to: 0; duration: 210; easing.type: Easing.InQuad }
    }
    HeroSprite {
        id: body
        factor: 2
        hero: walker.hero
        mirrored: walker.leftward
        animating: walker.animating
        pose: leap.running ? "jump" : walker.mood
    }
    PixelSparks { id: hearts; width: walker.size; height: walker.size; colours: ["#f4476f", "#ffb3c4"] }
    TapHandler {
        enabled: walker.animating
        onTapped: { leap.restart(); hearts.burst(16, 2) }
    }
}
