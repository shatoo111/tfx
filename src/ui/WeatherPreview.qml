import QtQuick
import Tfx.Scene 1.0
import "WeatherPixels.js" as WeatherPixels

Item {
    id: preview
    required property bool raining
    property int flightPause: 17
    property int flightLimit: 2
    property int hero: 0
    property int coat: 0
    property bool festive: true
    // Live snow and rain need the accelerated scene graph; without it a plain sketch is painted.
    readonly property bool liveWeather: theme <= 1 && GraphicsInfo.api !== GraphicsInfo.Software
    readonly property bool liveScene: theme >= 3 || liveWeather
    readonly property bool hoverable: interaction && (liveWeather || theme === 4 || theme >= 6)
    onLiveSceneChanged: repaintWeather()
    property int theme: 0
    property bool animate: true
    property int frameRate: 60
    property Item viewport
    property int branchSide: 2
    property real branchWidth: 78
    property real branchOpacity: 1
    onThemeChanged: repaintWeather()
    property bool sakura: false
    property real sakuraAngle: 15
    property bool sakuraBranches: true
    property bool interaction: true
    required property bool effectEnabled
    required property int intensity
    required property real wind
    required property bool surface
    required property real surfaceHeight
    required property bool lightning
    required property real flashInterval
    required property bool windowVisible
    property real animationTime: 0
    property real nextFlash: 0.4
    implicitHeight: 190

    readonly property bool inViewport: {
        if (!viewport)
            return true
        // Accumulate the scroll content's translation and watch layouts that
        // move the preview even when its own item geometry does not change.
        let top = 0
        let item = preview
        while (item && item !== viewport) {
            top += item.y
            item = item.parent
        }
        return !item || (top < viewport.height && top + height > 0)
    }
    readonly property bool animationActive: animate && effectEnabled && visible
        && windowVisible && inViewport && width > 0 && height > 0

    function wrap(value, span) {
        return value - Math.floor(value / span) * span
    }

    function repaintWeather() {
        if (weather)
            weather.requestPaint()
    }

    Canvas {
        id: backdrop
        objectName: "weatherPreviewBackdrop"
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: preview.paintBackdrop(getContext("2d"))
    }

    Canvas {
        id: weather
        anchors.fill: parent
        smooth: false
        visible: preview.effectEnabled && !preview.liveScene && !preview.sakura
        onVisibleChanged: if (visible) requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: preview.paintWeather(getContext("2d"))
    }

    Loader {
        id: sakuraScene
        anchors.fill: parent
        active: preview.sakura && preview.effectEnabled
        sourceComponent: Component {
            SakuraPreview {
                id: cherry
                software: GraphicsInfo.api === GraphicsInfo.Software
                readonly property var options: [preview.intensity, preview.sakuraAngle, preview.sakuraBranches,
                    preview.branchSide, preview.branchWidth, preview.branchOpacity, preview.interaction]
                function updateScene() {
                    configure(options[0], options[1], options[2], options[3], options[4], options[5], options[6])
                }
                function frame(seconds) {
                    advance(seconds, petalsHover.hovered ? petalsHover.point.position : Qt.point(-1, -1))
                }
                HoverHandler {
                    id: petalsHover
                    enabled: preview.interaction && preview.animationActive
                }
                onOptionsChanged: updateScene()
                Component.onCompleted: updateScene()
            }
        }
    }

    Loader {
        id: illustrated
        anchors.fill: parent
        active: preview.liveScene && preview.effectEnabled
        sourceComponent: Component {
            IllustratedPreview {
                id: illustration
                function updateScene() {
                    configure(preview.theme, preview.intensity, preview.wind, preview.flightPause, preview.flightLimit, preview.hero,
                              preview.coat, preview.festive, preview.surface, preview.surfaceHeight,
                              preview.lightning, preview.flashInterval)
                }
                function frame(dt) {
                    const cursor = pointerHover.enabled && pointerHover.hovered ? pointerHover.point.position : Qt.point(-1, -1)
                    advance(dt, cursor)
                }
                HoverHandler {
                    id: pointerHover
                    enabled: preview.hoverable && preview.animationActive
                }
                TapHandler {
                    enabled: pointerHover.enabled
                    acceptedButtons: Qt.LeftButton
                    onPressedChanged: if (pressed) illustration.pressAt(point.position)
                }
                Component.onCompleted: updateScene()
                onWidthChanged: updateScene()
                onHeightChanged: updateScene()
                Connections {
                    target: preview
                    function onThemeChanged() { if (preview.liveScene) illustration.updateScene() }
                    function onIntensityChanged() { illustration.updateScene() }
                    function onFlightPauseChanged() { illustration.updateScene() }
                    function onFlightLimitChanged() { illustration.updateScene() }
                    function onWindChanged() { illustration.updateScene() }
                    function onHeroChanged() { illustration.updateScene() }
                    function onCoatChanged() { illustration.updateScene() }
                    function onFestiveChanged() { illustration.updateScene() }
                    function onSurfaceChanged() { illustration.updateScene() }
                    function onSurfaceHeightChanged() { illustration.updateScene() }
                    function onLightningChanged() { illustration.updateScene() }
                    function onFlashIntervalChanged() { illustration.updateScene() }
                }
            }
        }
    }

    FrameAnimation {
        property real elapsedSinceUpdate: 0
        property real untilNextUpdate: 0
        running: preview.animationActive
        onRunningChanged: {
            elapsedSinceUpdate = 0
            untilNextUpdate = 0
        }
        onTriggered: {
            const elapsed = Math.min(frameTime, 0.05)
            elapsedSinceUpdate += elapsed
            untilNextUpdate -= elapsed
            // Allow for timestamp jitter so a nominal 60 Hz display does not
            // skip a frame when its callback arrives fractionally early.
            if (untilNextUpdate > 0.001)
                return
            untilNextUpdate = Math.max(0, untilNextUpdate + 1 / preview.frameRate)
            if (sakuraScene.item) {
                sakuraScene.item.frame(elapsedSinceUpdate)
            } else if (!preview.liveScene) {
                preview.animationTime += elapsedSinceUpdate
                preview.repaintWeather()
            } else if (illustrated.item) {
                illustrated.item.frame(elapsedSinceUpdate)
            }
            elapsedSinceUpdate = 0
        }
    }

    onRainingChanged: {
        nextFlash = animationTime + 0.4
        if (backdrop)
            backdrop.requestPaint()
        repaintWeather()
    }
    onEffectEnabledChanged: {
        nextFlash = animationTime + 0.4
        repaintWeather()
    }
    onIntensityChanged: repaintWeather()
    onWindChanged: repaintWeather()
    onSurfaceChanged: repaintWeather()
    onSurfaceHeightChanged: repaintWeather()
    onLightningChanged: {
        nextFlash = animationTime + 0.4
        repaintWeather()
    }
    onFlashIntervalChanged: {
        nextFlash = animationTime + 0.4
        repaintWeather()
    }

    // A wordless chat window, so the sample shows where the effect will live.
    function paintBackdrop(ctx) {
        if (width <= 0 || height <= 0) return
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        const snap = v => Math.round(v / 2) * 2
        const box = (x, y, w, h, colour) => { ctx.fillStyle = colour; ctx.fillRect(snap(x), snap(y), snap(w), snap(h)) }
        const narrow = width < 420, side = narrow ? 44 : width * 0.3, head = 40, foot = 34
        box(0, 0, width, height, "#131820")
        box(0, 0, side, height, "#1a212a")
        box(side, 0, width - side, head, "#1a212a")
        box(side, height - foot, width - side, foot, "#1a212a")
        box(side - 2, 0, 2, height, "#0e1217")
        box(side, head - 2, width - side, 2, "#0e1217")
        box(8, 10, side - 18, 20, "#252e39")
        const faces = ["#ff9fc0", "#f2a54a", "#86b9dc", "#8cf05a"]
        for (let i = 0; i < 4 && head + i * 44 + 40 < height; ++i) {
            const y = head + i * 44
            if (i === 0) box(0, y, side - 2, 44, "#2b5278")
            box(10, y + 10, 24, 24, faces[i])
            box(16, y + 18, 4, 4, "#1a212a")
            box(24, y + 18, 4, 4, "#1a212a")
            if (narrow) continue
            box(44, y + 12, side * (0.34 + 0.08 * (i % 2)), 6, "#c9d6ea")
            box(44, y + 26, side * (0.5 - 0.07 * i), 4, i === 0 ? "#a9c0d6" : "#55636f")
        }
        box(side + 12, 8, 22, 22, faces[0])
        box(side + 44, 10, 90, 6, "#c9d6ea")
        box(side + 44, 22, 46, 4, "#55636f")
        const left = side + 14, room = width - side - 28, middle = (head + height - foot) / 2
        const bubble = (x, y, w, colour) => { box(x, y, w, 34, colour); box(x + 12, y + 11, w - 60, 5, "#c9d6ea"); box(x + 12, y + 21, (w - 24) * 0.55, 4, "#8ea6bd") }
        bubble(left, middle - 62, room * 0.5, "#27313d")
        bubble(left + room * 0.42, middle - 18, room * 0.56, "#2b5278")
        bubble(left, middle + 26, room * 0.42, "#27313d")
        box(side + 16, height - foot + 14, 10, 4, "#b8ff5a")
        box(side + 34, height - foot + 14, room * 0.35, 4, "#55636f")
    }

    function paintWeather(ctx) {
        if (width <= 0 || height <= 0 || !effectEnabled || sakura || liveScene) return
        ctx.reset()
        ctx.clearRect(0, 0, width, height)
        const time = animationTime
        const count = (raining ? [14, 28, 52] : [32, 64, 110])[intensity]
        const margin = raining ? 22 : 5
        const span = height + 2 * margin
        const slope = wind * (raining ? 0.65 : 0.9)
        const depth = surface ? Math.min(height * 0.3, surfaceHeight * 0.35) : 0
        const impacts = []
        const bladeSpacing = 7
        function bladeHeight(blade) {
            return depth * (0.35 + 0.65 * Math.abs(Math.sin(blade * 1.7)))
        }
        for (let i = 0; i < count; ++i) {
            const speed = raining ? 190 + (i % 7) * 23 : 22 + (i % 7) * 5
            const y = wrap(wrap(i * i * 0.754877666 + 0.13, 1) * span + time * speed, span) - margin
            const sway = raining ? 0 : Math.sin(time * 1.25 + i) * 6
            const x = wrap(wrap(i * 0.61803398875 + 0.07, 1) * width + y * slope + sway,
                           width + 2 * margin) - margin
            const size = (0.9 + (i % 5) * 0.45) * (raining ? 1.6 : 1)
            if (raining) {
                const blade = Math.round(x / bladeSpacing)
                const ground = height - bladeHeight(blade) * 0.72
                const age = (y - ground) / speed
                if (age >= 0) {
                    if (age < 0.14 && depth > 0) {
                        impacts[blade] = (impacts[blade] || 0) + (1-age/0.14) * Math.sin(age*30)
                        WeatherPixels.splash(ctx, x - age * 22, ground - age * 28)
                    }
                    continue
                }
                WeatherPixels.drop(ctx, x, y, size, slope, i % 3 === 0)
            } else if (i % 9 === 0) {
                WeatherPixels.crystal(ctx, x, y, i + time * 0.4)
            } else {
                WeatherPixels.snow(ctx, x, y, size, i % 3 === 0)
            }
        }
        if (surface && surfaceHeight > 0) {
            if (raining) {
                for (let i = 0; i * bladeSpacing < width; ++i) {
                    const x = i * bladeSpacing + Math.sin(i*8.3)*2, tip = bladeHeight(i)
                    const hit = (impacts[i] || 0) + 0.35*((impacts[i-1] || 0) + (impacts[i+1] || 0))
                    const lean = wind*12 + Math.sin(x*0.08 + time*2.4)*4 + Math.sin(i*2.4)*6 + hit*5
                    WeatherPixels.grass(ctx, x, height, tip, lean, i % 3)
                }
            } else {
                ctx.fillStyle = "#a6e9e9d7"
                ctx.beginPath()
                ctx.moveTo(0, height)
                ctx.lineTo(0, height - depth * 0.6)
                ctx.bezierCurveTo(width * 0.25, height - depth * 1.4,
                                  width * 0.3, height, width * 0.6, height - depth * 0.65)
                ctx.quadraticCurveTo(width * 0.85, height - depth * 1.1, width, height - depth * 0.4)
                ctx.lineTo(width, height)
                ctx.closePath()
                ctx.fill()
            }
        }
        const flashAge = time >= nextFlash ? (time - nextFlash) % flashInterval : 1
        if (raining && lightning && flashAge < 0.7) {
            const strength = Math.pow(Math.sin(Math.PI * flashAge / 0.7), 2)
            WeatherPixels.lightning(ctx, width, height, strength)
        }
    }
}
