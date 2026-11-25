pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var preferences
    required property var controller
    property bool initiallyHidden: false
    readonly property var themeNames: ["Снегопад", "Дождь", "Сакура", "Осенние листья", "The United States", "Звёздная пыль", "8 бит", "Чёрная дыра", "Котик", "Аквариум"]
    readonly property bool parade: preferences.effect === 4
    readonly property bool flightTheme: parade || preferences.effect === 5
    readonly property bool ambient: preferences.effect >= 3
    readonly property bool retro: preferences.effect === 6
    readonly property bool blackHole: preferences.effect === 7
    readonly property bool cat: preferences.effect === 8
    readonly property bool aquarium: preferences.effect === 9
    readonly property var coatNames: ["Рыжий", "Чёрный", "Серый", "Белый"]
    readonly property var intensityNames: cat ? ["Один", "Два", "Три"] : aquarium ? ["Немного", "Обычно", "Много"] : parade ? ["Редкий", "Обычный", "Частый"]
        : (sakura || ambient) ? ["Редкие", "Обычные", "Густые"] : raining ? ["Лёгкий", "Обычный", "Ливень"] : ["Лёгкий", "Обычный", "Метель"]
    readonly property string intensityTitle: cat ? "Клубки" : aquarium ? "Обитатели" : blackHole ? "Звёзды" : parade ? "Салют" : "Интенсивность"
    readonly property bool sakura: preferences.effect === 2
    readonly property string angleLabel: Math.abs(preferences.sakuraAngle) < 0.5 ? "↓ 0°" : (preferences.sakuraAngle < 0 ? "↙ " : "↘ ") + Math.round(Math.abs(preferences.sakuraAngle)) + "°"
    readonly property bool raining: preferences.effect === 1
    readonly property bool compact: width < 860
    readonly property color ink: colors.text
    readonly property color muted: colors.muted
    // Decorations rest when the window has been in the background for a while.
    readonly property bool awake: active || drowsy.running
    readonly property bool shown: visible && visibility !== Window.Minimized
    readonly property bool animating: shown && awake && !controller.reducedMotion && !preferences.economy
    property int shownEffect: preferences.effect
    readonly property int gutter: compact ? 22 : 32
    readonly property string mood: ["Немного зимы", "Дождливое настроение", "Время цветения", "Золотая пора", "Парад 4 июля", "Загадай желание", "Нажми старт", "Горизонт событий", "Мур-мур", "Тихая глубина"][preferences.effect]
    readonly property string windLabel: Math.abs(preferences.wind) < 0.02 ? "Штиль"
        : (preferences.wind < 0 ? "← " : "→ ") + Math.round(Math.abs(preferences.wind) * 100) + "%"
    width: Math.max(minimumWidth, Math.min(1120, Screen.desktopAvailableWidth - 32))
    height: Math.max(minimumHeight, Math.min(820, Screen.desktopAvailableHeight - 32))
    minimumWidth: 400
    minimumHeight: 520
    title: "TFX"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint
    visible: !initiallyHidden
    color: "transparent"
    font.family: mono.status === FontLoader.Ready ? mono.name : Qt.platform.os === "osx" ? "Menlo" : "Consolas"
    font.weight: Font.Normal
    onActiveChanged: {
        if (active) controller.release.refresh()
        else drowsy.restart()
    }
    Timer { id: drowsy; interval: 12000; running: true }
    TfxColors { id: colors }

    // Straight into Telegram; the web page is for computers where the app did not register its links.
    function openChannel() {
        if (!Qt.openUrlExternally("tg://resolve?domain=tfxsoft")) Qt.openUrlExternally("https://t.me/tfxsoft")
    }
    FontLoader { id: mono; source: "qrc:/assets/fonts/JetBrainsMono-Regular.ttf" }
    FontLoader { source: "qrc:/assets/fonts/JetBrainsMono-Bold.ttf" }
    FontLoader { source: "qrc:/assets/fonts/TFXPixel-Regular.ttf" }
    palette {
        window: colors.panel
        windowText: root.ink
        text: root.ink
        button: colors.raised
        buttonText: root.ink
        highlight: colors.acid
        highlightedText: colors.acidInk
        base: colors.panel
        mid: colors.line
    }

    component PresetMenuItem: MenuItem {
        id: option
        hoverEnabled: true
        contentItem: Text {
            text: option.text
            color: option.highlighted || option.hovered ? colors.acidInk : root.ink
            font.family: root.font.family
            font.pixelSize: 13
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            leftPadding: 10
        }
        background: Rectangle { color: option.highlighted || option.hovered ? colors.acid : "transparent" }
    }
    component PixelTitle: Label {
        color: root.ink
        font.family: colors.pixelFont
        font.pixelSize: 16
        renderType: Text.NativeRendering
    }
    component Divider: Rectangle {
        Layout.fillWidth: true
        implicitHeight: 2
        color: colors.line
    }
    component Metric: ColumnLayout {
        id: metric
        property string caption
        property string value
        spacing: 6
        Label { text: metric.caption; color: root.muted; font.pixelSize: 11; font.capitalization: Font.AllUppercase }
        Label { text: metric.value; color: colors.acid; font.family: colors.pixelFont; font.pixelSize: 16; renderType: Text.NativeRendering }
    }

    // The grid of the landing page. The Canvas is a static fallback for graphics backends
    // without shader support; the shader covers it otherwise.
    background: Item {
        id: backdrop
        Canvas {
            anchors.fill: parent
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d"), px = 4
                ctx.reset()
                ctx.clearRect(0, 0, width, height)
                for (const [inset, colour] of [[0, colors.line], [px, colors.ground]]) {
                    ctx.fillStyle = colour
                    for (let row = 0; row < 3; ++row)
                        ctx.fillRect(inset + (2 - row) * px, inset + row * px, width - 2 * inset - 2 * (2 - row) * px, height - 2 * inset - 2 * row * px)
                }
                ctx.fillStyle = "#202021"
                for (let x = 48; x < width - 12; x += 48) ctx.fillRect(x, px, 1, height - 2 * px)
                for (let y = 48; y < height - 12; y += 48) ctx.fillRect(px, y, width - 2 * px, 1)
            }
        }
        // The grid breathes slowly, so it is painted eight times a second and reused in between.
        ShaderEffect {
            id: grid
            anchors.fill: parent
            property real time: 0
            property real px: 4
            property size size: Qt.size(width, height)
            fragmentShader: "qrc:/tfx/shaders/backdrop.frag.qsb"
        }
        ShaderEffectSource {
            id: gridPicture
            anchors.fill: parent
            sourceItem: grid
            hideSource: true
            live: false
            smooth: false
            onWidthChanged: scheduleUpdate()
            onHeightChanged: scheduleUpdate()
        }
        Timer {
            interval: 125
            repeat: true
            running: root.animating
            onTriggered: {
                grid.time = (grid.time + 0.125) % 4096
                gridPicture.scheduleUpdate()
            }
        }
    }

    header: Item {
        implicitHeight: 78

        MouseArea {
            anchors.fill: parent
            onPressed: root.startSystemMove()
        }
        Rectangle { x: root.gutter; y: 14; width: parent.width - 2 * root.gutter; height: 2; color: colors.acid }

        RowLayout {
            anchors { left: parent.left; right: parent.right; leftMargin: root.gutter; rightMargin: root.gutter }
            y: 16
            height: 62
            spacing: root.compact ? 8 : 14
            Image {
                source: "qrc:/assets/pixel/logo.png"
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                smooth: false
                Accessible.role: Accessible.Graphic
                Accessible.name: "TFX"
            }
            Label {
                text: root.compact ? "TFX" : Qt.platform.os === "osx" ? "TFX / Telegram" : "TFX / Telegram Desktop"
                font.capitalization: Font.AllUppercase
                font.pixelSize: 13
                font.bold: true
                color: root.ink
            }
            Label { visible: !root.compact; text: "×××"; color: colors.acid; font.pixelSize: 13; font.bold: true; font.letterSpacing: 5 }
            Item { Layout.fillWidth: true }
            PixelButton {
                objectName: "channelButton"
                visible: !root.compact
                text: "@tfxsoft"
                onClicked: root.openChannel()
                Accessible.name: "Открыть канал TFX в Telegram"
            }
            Label {
                text: root.preferences.enabled ? "Включено" : "Выключено"
                font.capitalization: Font.AllUppercase
                color: root.preferences.enabled ? colors.acid : root.muted
                font.pixelSize: 12
                font.bold: true
            }
            PixelSwitch {
                objectName: "effectSwitch"
                checked: root.preferences.enabled
                onToggled: root.preferences.enabled = checked
                Accessible.name: "Включить эффект"
            }
            PixelButton {
                id: minimizeButton
                objectName: "minimizeWindowButton"
                padding: 0
                horizontalPadding: 0
                Layout.preferredWidth: 40
                Accessible.name: "Свернуть TFX"
                Accessible.description: "Эффект в Telegram продолжит работать"
                onClicked: root.showMinimized()
                ToolTip {
                    visible: minimizeButton.hovered
                    delay: 600
                    x: minimizeButton.width - width
                    y: minimizeButton.height + 6
                    padding: 10
                    contentItem: Text {
                        text: "Свернуть — эффект в Telegram продолжит работать"
                        color: root.ink
                        font.family: root.font.family
                        font.pixelSize: 12
                    }
                    background: PixelFrame { fill: colors.panel; edge: colors.line }
                }
                contentItem: Item {
                    Rectangle { anchors.centerIn: parent; width: 15; height: 3; color: minimizeButton.lit ? colors.acid : root.ink }
                }
            }
            PixelButton {
                id: closeButton
                objectName: "closeWindowButton"
                padding: 0
                horizontalPadding: 0
                Layout.preferredWidth: 40
                Accessible.name: "Закрыть TFX"
                onClicked: quitDialog.open()
                contentItem: Item {
                    Repeater {
                        model: [[0, 0], [1, 1], [2, 2], [3, 3], [4, 4], [4, 0], [3, 1], [1, 3], [0, 4]]
                        delegate: Rectangle {
                            required property var modelData
                            x: parent.width / 2 - 7.5 + modelData[0] * 3
                            y: parent.height / 2 - 7.5 + modelData[1] * 3
                            width: 3
                            height: 3
                            color: closeButton.lit ? colors.danger : root.ink
                        }
                    }
                }
            }
        }
    }

    // A newer version on the site shows up by itself at the bottom of the window.
    footer: Item {
        id: releaseBar
        objectName: "releaseBar"
        readonly property bool shown: root.controller.release.version.length > 0
        property string version
        implicitHeight: shown ? 86 : 0
        visible: implicitHeight > 1
        clip: true
        // The text stays while the bar folds away.
        Binding { target: releaseBar; property: "version"; value: root.controller.release.version; when: releaseBar.shown; restoreMode: Binding.RestoreNone }
        Behavior on implicitHeight { enabled: root.animating; NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
        PixelFrame {
            x: 14
            y: 4
            width: parent.width - 28
            height: 68
            fill: colors.panel
            edge: colors.acid
            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 12 }
                spacing: root.compact ? 8 : 14
                HeroSprite { visible: !root.compact; factor: 2; hero: 1; pose: "run"; animating: root.animating && releaseBar.visible }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    PixelTitle {
                        objectName: "releaseTitle"
                        Layout.fillWidth: true
                        text: root.compact ? "TFX " + releaseBar.version : "Доступна TFX " + releaseBar.version
                        color: colors.acid
                        font.pixelSize: root.compact ? 12 : 16
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.compact ? "Есть обновление" : "Сейчас установлена " + root.controller.release.current
                        color: root.muted
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
                PixelButton {
                    objectName: "releaseButton"
                    text: "Обновить"
                    primary: true
                    onClicked: Qt.openUrlExternally(root.controller.release.page)
                    Accessible.name: "Скачать TFX " + releaseBar.version + " на сайте"
                }
                PixelButton {
                    objectName: "releaseLaterButton"
                    text: root.compact ? "×" : "Позже"
                    horizontalPadding: root.compact ? 12 : 18
                    onClicked: root.controller.release.dismiss()
                    Accessible.name: "Напомнить об обновлении позже"
                }
            }
        }
    }

    ScrollView {
        id: scroll
        objectName: "settingsScroll"
        anchors { fill: parent; leftMargin: 4; rightMargin: 4; bottomMargin: 4 }
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            objectName: "settingsScrollBar"
            parent: scroll
            anchors {
                right: scroll.right
                top: scroll.top
                bottom: scroll.bottom
                rightMargin: 8
                topMargin: 10
                bottomMargin: 12
            }
            contentItem: Rectangle {
                implicitWidth: 8
                implicitHeight: 40
                color: scrollBar.pressed ? colors.acid : scrollBar.hovered ? colors.muted : colors.line
            }
        }
        ColumnLayout {
            x: root.gutter - 4
            width: scroll.availableWidth - x * 2
            spacing: 40

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 22
                PixelWorld {
                    id: world
                    objectName: "world"
                    Layout.fillWidth: true
                    Layout.leftMargin: 4 - root.gutter
                    Layout.rightMargin: 4 - root.gutter
                    implicitHeight: root.compact ? 150 : 200
                    theme: root.preferences.effect
                    phrases: [root.themeNames[root.preferences.effect], root.mood]
                    animating: root.animating && scroll.contentItem.contentY < height
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compact ? 1 : 2
                    rowSpacing: 18
                    columnSpacing: 30
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 420
                        spacing: 12
                        Rectangle { width: 12; height: 12; color: root.preferences.enabled ? colors.acid : colors.line; Layout.alignment: Qt.AlignVCenter }
                        TypedText {
                            objectName: "statusText"
                            Layout.fillWidth: true
                            full: root.controller.status
                            animating: root.animating
                            color: root.ink
                            font.family: root.font.family
                            font.pixelSize: 13
                            font.bold: true
                            font.capitalization: Font.AllUppercase
                            wrapMode: Text.WordWrap
                            Accessible.role: Accessible.StaticText
                            Accessible.name: full
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 530
                        spacing: root.compact ? 20 : 40
                        Metric { caption: root.intensityTitle; value: root.intensityNames[root.preferences.intensity] }
                        Metric { caption: root.sakura ? "Угол полёта" : root.cat ? "Окрас" : root.aquarium ? "Течение" : root.blackHole ? "Вращение" : "Ветер"; value: root.sakura ? root.angleLabel : root.cat ? root.coatNames[root.preferences.catCoat] : root.windLabel }
                        Metric { caption: "Плавность"; value: (root.preferences.economy ? 30 : root.preferences.fps) + " fps" }
                        Item { Layout.fillWidth: true }
                    }
                }
                ThemeGrid {
                    objectName: "themePicker"
                    Layout.fillWidth: true
                    names: root.themeNames
                    current: root.preferences.effect
                    hero: root.preferences.retroHero
                    animating: root.animating
                    onActivated: index => root.preferences.effect = index
                    Accessible.role: Accessible.Grouping
                    Accessible.name: "Тема эффекта"
                }
                // The way to the channel, where new themes are announced.
                Rectangle {
                    objectName: "channelRow"
                    Layout.fillWidth: true
                    implicitHeight: channelLine.implicitHeight + 28
                    color: colors.panel
                    Rectangle { width: 4; height: parent.height; color: colors.acid }
                    GridLayout {
                        id: channelLine
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 22; rightMargin: 14 }
                        columns: root.compact ? 1 : 2
                        rowSpacing: 12
                        columnSpacing: 16
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            PixelTitle { text: "Канал TFX" }
                            Label {
                                Layout.fillWidth: true
                                text: "Новые темы, обновления и опросы — раньше всех"
                                color: root.muted
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                        PixelButton {
                            objectName: "channelLinkButton"
                            text: "Открыть в Telegram"
                            primary: true
                            onClicked: root.openChannel()
                            Accessible.name: "Открыть канал TFX в Telegram"
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.compact ? 1 : 2
                columnSpacing: 24
                rowSpacing: 40
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 650
                    Layout.alignment: Qt.AlignTop
                    spacing: 40
                    PixelCard {
                        objectName: "weatherCard"
                        Layout.fillWidth: true
                        resident: 0
                        animating: root.animating
                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                spacing: 4
                                PixelTitle { text: root.themeNames[root.preferences.effect]; font.pixelSize: root.compact ? 16 : 24 }
                                Label { text: "Образец эффекта"; color: root.muted; font.pixelSize: 12 }
                            }
                            Item { Layout.fillWidth: true }
                        }
                        PixelFrame {
                            Layout.fillWidth: true
                            implicitHeight: root.compact ? 220 : 280
                            px: 4
                            fill: colors.sunken
                            edge: colors.line
                            WeatherPreview {
                                objectName: "weatherPreview"
                                anchors { fill: parent; margins: 8 }
                                viewport: scroll.contentItem
                                windowVisible: root.shown && root.awake
                                theme: root.preferences.effect
                                animate: !root.controller.reducedMotion && !root.preferences.economy
                                branchSide: root.preferences.branchSide
                                branchWidth: root.preferences.branchWidth
                                branchOpacity: root.preferences.branchOpacity
                                sakura: root.sakura
                                sakuraAngle: root.preferences.sakuraAngle
                                sakuraBranches: root.preferences.sakuraBranches
                                interaction: root.preferences.interaction
                                raining: root.raining
                                frameRate: root.preferences.fps
                                effectEnabled: root.preferences.enabled
                                flightLimit: root.preferences.flightLimit
                                flightPause: root.preferences.flightPause
                                hero: root.preferences.retroHero
                                coat: root.preferences.catCoat
                                festive: root.preferences.festive
                                intensity: root.preferences.intensity
                                wind: root.preferences.wind
                                surface: root.raining ? root.preferences.grass : root.preferences.accumulation
                                surfaceHeight: root.raining ? root.preferences.grassHeight : root.preferences.maxDepth
                                lightning: root.preferences.lightning
                                flashInterval: root.preferences.flashInterval
                                Accessible.role: Accessible.Graphic
                                Accessible.name: "Образец выбранного эффекта"
                            }
                            PixelCurtain { id: previewCurtain; anchors { fill: parent; margins: 8 } color: colors.sunken }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            objectName: "intensityControls"
                            spacing: 10
                            Label { text: root.intensityTitle; color: root.muted; font.pixelSize: 12 }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Repeater {
                                    model: root.intensityNames
                                    delegate: PixelButton {
                                        required property int index
                                        required property string modelData
                                        objectName: "intensityPreset" + index
                                        text: modelData
                                        Layout.fillWidth: true
                                        selected: root.preferences.intensity === index
                                        onClicked: root.preferences.intensity = index
                                        Accessible.name: modelData
                                    }
                                }
                            }
                        }
                        ColumnLayout {
                            objectName: "retroControls"
                            visible: root.retro
                            Layout.fillWidth: true
                            spacing: 8
                            Label { text: "Герой"; color: root.muted; font.pixelSize: 12 }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Repeater {
                                    model: ["Котик", "Рыцарь", "Робот"]
                                    delegate: PixelButton {
                                        required property int index
                                        required property string modelData
                                        objectName: "retroHero" + index
                                        text: modelData
                                        Layout.fillWidth: true
                                        selected: root.preferences.retroHero === index
                                        onClicked: root.preferences.retroHero = index
                                        Accessible.name: "Герой: " + modelData
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "Пауза между вылазками"; color: root.muted; font.pixelSize: 12; Layout.fillWidth: true }
                                Label { text: root.preferences.flightPause + " с"; color: root.ink; font.pixelSize: 12 }
                            }
                            PixelSlider {
                                objectName: "retroPauseSlider"
                                hero: 1
                                Layout.fillWidth: true
                                from: 2; to: 60; stepSize: 1
                                value: root.preferences.flightPause
                                onMoved: root.preferences.flightPause = Math.round(value)
                                Accessible.name: "Пауза между вылазками героя в секундах"
                            }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: "Герой выходит из домика в правом нижнем углу, пробегает по всему нижнему краю окна два-три раза и собирает монеты. Наведите на него курсор — он подпрыгнет, а монеты под курсором соберутся сами."
                                color: root.muted
                                font.pixelSize: 12
                            }
                        }
                        ColumnLayout {
                            objectName: "catControls"
                            visible: root.cat
                            Layout.fillWidth: true
                            spacing: 8
                            Label { text: "Окрас"; color: root.muted; font.pixelSize: 12 }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Repeater {
                                    model: root.coatNames
                                    delegate: PixelButton {
                                        required property int index
                                        required property string modelData
                                        objectName: "catCoat" + index
                                        text: modelData
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 1
                                        selected: root.preferences.catCoat === index
                                        onClicked: root.preferences.catCoat = index
                                        Accessible.name: "Окрас: " + modelData
                                    }
                                }
                            }
                        }
                        ColumnLayout {
                            objectName: "blackHoleControls"
                            visible: root.blackHole
                            Layout.fillWidth: true
                            spacing: 8
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: "Пауза между гостями"; color: root.muted; font.pixelSize: 12; Layout.fillWidth: true }
                                Label { text: root.preferences.flightPause + " с"; color: root.ink; font.pixelSize: 12 }
                            }
                            PixelSlider {
                                objectName: "planetPauseSlider"
                                hero: 2
                                Layout.fillWidth: true
                                from: 2; to: 60; stepSize: 1
                                value: root.preferences.flightPause
                                onMoved: root.preferences.flightPause = Math.round(value)
                                Accessible.name: "Пауза между гостями в секундах"
                            }
                        }
                        Label {
                            objectName: "sceneHint"
                            visible: root.blackHole || root.cat || root.aquarium
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: root.blackHole ? "Маленькая дыра живёт в правом нижнем углу. Задержите на ней курсор: она вырастет и рассыплется искрами."
                                : root.cat ? "Кот гуляет вдоль нижнего края. Погладьте его курсором, и он замурлычет. Толкните клубок, и кот побежит за ним."
                                : "Рыбки плавают вдоль нижнего края и пугаются курсора, а иглобрюх надувается. Сундук в углу открывается при наведении."
                            color: root.muted
                            font.pixelSize: 12
                        }
                        ColumnLayout {
                            visible: root.flightTheme
                            Layout.fillWidth: true
                            spacing: 8
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: root.parade ? "Пауза между гостями" : "Пауза между пролётами"; color: root.muted; font.pixelSize: 12; Layout.fillWidth: true }
                                Label { text: root.preferences.flightPause + " с"; color: root.ink; font.pixelSize: 12 }
                            }
                            PixelSlider {
                                objectName: "flightPauseSlider"
                                hero: 1
                                Layout.fillWidth: true
                                from: 2; to: 60; stepSize: 1
                                value: root.preferences.flightPause
                                onMoved: root.preferences.flightPause = Math.round(value)
                                Accessible.name: root.parade ? "Пауза между гостями парада в секундах" : "Пауза между пролётами в секундах"
                            }
                            Label { text: root.parade ? "Гостей одновременно" : "Максимум комет на экране"; color: root.muted; font.pixelSize: 12 }
                            RowLayout {
                                Layout.fillWidth: true
                                Repeater {
                                    model: 3
                                    delegate: PixelButton {
                                        required property int index
                                        text: String(index + 1)
                                        Layout.fillWidth: true
                                        selected: root.preferences.flightLimit === index + 1
                                        onClicked: root.preferences.flightLimit = index + 1
                                        Accessible.name: (root.parade ? "Гостей парада одновременно: " : "Максимум комет: ") + text
                                    }
                                }
                            }
                            Label {
                                visible: root.parade
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: "По нижнему краю идёт парад: орлан, пикап, ковбой, астронавт и такса в булочке. Наведите на гостя — он поздоровается, нажмите — запустит салют."
                                color: root.muted
                                font.pixelSize: 12
                            }
                        }
                        ColumnLayout {
                            visible: !root.cat
                            Layout.fillWidth: true
                            spacing: 5
                            RowLayout {
                                Label { text: root.sakura ? "Угол полёта лепестков" : root.aquarium ? "Течение" : root.blackHole ? "Вращение" : root.parade ? "Направление парада" : "Направление ветра"; color: root.muted; font.pixelSize: 12; Layout.fillWidth: true }
                                Label { text: root.sakura ? root.angleLabel : root.windLabel; color: root.ink; font.pixelSize: 12 }
                            }
                            PixelSlider {
                                objectName: "windSlider"
                                hero: 0
                                centered: true
                                Layout.fillWidth: true
                                from: root.sakura ? -65 : -1; to: root.sakura ? 65 : 1; stepSize: root.sakura ? 1 : 0.01
                                value: root.sakura ? root.preferences.sakuraAngle : root.preferences.wind
                                onMoved: {
                                    if (root.sakura) root.preferences.sakuraAngle = value
                                    else root.preferences.wind = value
                                }
                                Accessible.name: root.sakura ? "Угол полёта лепестков от вертикали" : "Скорость и направление ветра"
                            }
                        }
                    }
                    PixelCard {
                        objectName: "stormCard"
                        resident: 2
                        animating: root.animating
                        visible: root.raining
                        Layout.fillWidth: true
                        SettingRow {
                            title: "Гроза"
                            detail: "Мягкие вспышки поверх дождя"
                            control: PixelSwitch {
                                objectName: "lightningSwitch"
                                checked: root.preferences.lightning
                                onToggled: root.preferences.lightning = checked
                                Accessible.name: "Вспышки грозы"
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            enabled: root.preferences.lightning
                            opacity: enabled ? 1 : 0.45
                            RowLayout {
                                Label { text: "Интервал вспышек"; color: root.muted; font.pixelSize: 12; Layout.fillWidth: true }
                                Label { text: "≈ " + Math.round(root.preferences.flashInterval) + " сек"; color: root.ink; font.pixelSize: 12 }
                            }
                            PixelSlider {
                                objectName: "flashIntervalSlider"
                                hero: 2
                                Layout.fillWidth: true
                                from: 5; to: 60; stepSize: 1
                                value: root.preferences.flashInterval
                                onMoved: root.preferences.flashInterval = value
                                Accessible.name: "Средний интервал вспышек грозы"
                            }
                        }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 330
                    Layout.alignment: Qt.AlignTop
                    spacing: 40
                    PixelCard {
                        objectName: "surfaceCard"
                        resident: 1
                        animating: root.animating
                        visible: !root.ambient
                        Layout.fillWidth: true
                        spacing: 16
                        SettingRow {
                            objectName: "surfaceRow"
                            title: root.sakura ? "Ветки сакуры" : root.raining ? "Трава" : "Сугробы"
                            detail: root.sakura ? "Цветущие ветки в верхних углах" : "Вдоль нижней границы окна"
                            control: PixelSwitch {
                                objectName: "surfaceSwitch"
                                checked: root.sakura ? root.preferences.sakuraBranches : root.raining ? root.preferences.grass : root.preferences.accumulation
                                onToggled: {
                                    if (root.sakura) root.preferences.sakuraBranches = checked
                                    else if (root.raining) root.preferences.grass = checked
                                    else root.preferences.accumulation = checked
                                }
                                Accessible.name: root.sakura ? "Показать ветки сакуры" : root.raining ? "Показать траву" : "Накопление снега"
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            visible: !root.sakura
                            enabled: root.raining ? root.preferences.grass : root.preferences.accumulation
                            opacity: enabled ? 1 : 0.45
                            RowLayout {
                                spacing: 6
                                Label { text: Math.round(root.raining ? root.preferences.grassHeight : root.preferences.maxDepth); color: colors.acid; font.family: colors.pixelFont; font.pixelSize: 40; renderType: Text.NativeRendering }
                                Label { text: "pt"; color: root.muted; font.pixelSize: 14; Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 6 }
                                Item { Layout.fillWidth: true }
                                Label { text: "Максимальная\nвысота"; color: root.muted; font.pixelSize: 11; horizontalAlignment: Text.AlignRight }
                            }
                            PixelSlider {
                                objectName: "surfaceHeightSlider"
                                hero: 1
                                Layout.fillWidth: true
                                from: 0; to: 160; stepSize: 1
                                value: root.raining ? root.preferences.grassHeight : root.preferences.maxDepth
                                onMoved: {
                                    if (root.raining) root.preferences.grassHeight = value
                                    else root.preferences.maxDepth = value
                                }
                                Accessible.name: root.raining ? "Высота травы" : "Максимальная высота сугробов"
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: root.sakura && root.preferences.sakuraBranches
                            Label { text: "Сторона веток"; color: root.ink }
                            PixelPicker {
                                model: ["Слева", "Справа", "С обеих сторон"]
                                currentIndex: root.preferences.branchSide
                                onActivated: index => root.preferences.branchSide = index
                            }
                            Label { text: "Размер веток · " + Math.round(root.preferences.branchWidth); color: root.ink }
                            PixelSlider { objectName: "branchWidthSlider"; hero: 0; Layout.fillWidth: true; stepSize: 1; from: 24; to: 160; value: root.preferences.branchWidth; onMoved: root.preferences.branchWidth = value; Accessible.name: "Ширина веток" }
                            Label { text: "Непрозрачность · " + Math.round(root.preferences.branchOpacity*100) + "%"; color: root.ink }
                            PixelSlider { objectName: "branchOpacitySlider"; hero: 2; Layout.fillWidth: true; stepSize: 0.01; from: root.preferences.minBranchOpacity; to: 1; value: root.preferences.branchOpacity; onMoved: root.preferences.branchOpacity = value; Accessible.name: "Непрозрачность веток" }
                        }
                        Divider {}
                        SettingRow {
                            objectName: "festiveRow"
                            visible: root.preferences.effect === 0
                            title: "Новогоднее оформление"
                            detail: "Ёлки, снеговик и огоньки на снегу"
                            control: PixelSwitch {
                                objectName: "festiveSwitch"
                                checked: root.preferences.festive
                                onToggled: root.preferences.festive = checked
                                Accessible.name: "Новогоднее оформление"
                            }
                        }
                        SettingRow {
                            title: "Реакция на курсор"
                            detail: root.sakura ? "Лепестки мягко разлетаются от мыши" : root.raining ? "Касайтесь травы движением мыши" : "Смахивайте снег движением мыши"
                            control: PixelSwitch {
                                objectName: "interactionSwitch"
                                checked: root.preferences.interaction
                                onToggled: root.preferences.interaction = checked
                                Accessible.name: "Взаимодействие с курсором"
                            }
                        }
                        PixelButton {
                            objectName: "clearSnowButton"
                            visible: !root.raining && !root.sakura
                            text: "Очистить снег"
                            Layout.fillWidth: true
                            onClicked: root.controller.clearSnow()
                        }
                    }
                    PixelCard {
                        Layout.fillWidth: true
                        spacing: 16
                        SettingRow {
                            title: "Плавность"
                            detail: root.preferences.economy ? "Экономичный режим ограничивает частоту до 30 кадров/с" : ""
                            control: RowLayout {
                                spacing: 4
                                PixelButton { objectName: "fps30Button"; text: "30"; enabled: !root.preferences.economy; selected: root.preferences.economy || root.preferences.fps === 30; onClicked: root.preferences.fps = 30; Accessible.name: "30 кадров в секунду" }
                                PixelButton { objectName: "fps60Button"; text: "60"; enabled: !root.preferences.economy; selected: !root.preferences.economy && root.preferences.fps === 60; onClicked: root.preferences.fps = 60; Accessible.name: "60 кадров в секунду" }
                            }
                        }
                        Divider {}
                        SettingRow {
                            title: "Автозапуск"
                            detail: "Вместе с системой"
                            control: PixelSwitch {
                                objectName: "autoStartSwitch"
                                checked: root.controller.autoStart
                                onToggled: root.controller.autoStart = checked
                                Accessible.name: "Автозапуск"
                            }
                        }
                    }
                }
            }

            PixelCard {
                Layout.fillWidth: true
                SettingRow {
                    title: "Экономичный режим"
                    detail: "30 кадров/с и меньше частиц"
                    control: PixelSwitch { checked: root.preferences.economy; onToggled: root.preferences.economy = checked }
                }
                SettingRow {
                    title: "Настроить тему"
                    control: PixelButton {
                        text: "Выбрать"
                        onClicked: presetMenu.open()
                        Menu {
                            id: presetMenu
                            y: parent.height + 4
                            width: 158
                            padding: 6
                            background: PixelFrame { fill: colors.panel; edge: colors.line }
                            PresetMenuItem { text: "Ненавязчиво"; onTriggered: root.preferences.applyPreset(0) }
                            PresetMenuItem { text: "Уютно"; onTriggered: root.preferences.applyPreset(1) }
                            PresetMenuItem { text: "Празднично"; onTriggered: root.preferences.applyPreset(2) }
                        }
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                visible: root.controller.error.length > 0
                implicitHeight: errorText.implicitHeight + 28
                color: colors.panel
                border.width: 3
                border.color: colors.danger
                Label {
                    id: errorText
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
                    text: root.controller.error
                    color: root.ink
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                }
            }
            // The ground of the level.
            Item {
                Layout.fillWidth: true
                implicitHeight: 128
                Rectangle { y: 34; width: parent.width; height: 4; color: "#7be05a" }
                Image {
                    y: 38
                    width: parent.width / 4
                    height: 8
                    scale: 4
                    transformOrigin: Item.TopLeft
                    smooth: false
                    fillMode: Image.TileHorizontally
                    horizontalAlignment: Image.AlignLeft
                    source: "qrc:/assets/pixel/turf.png"
                }
                LedgeWalker { hero: 2; ledge: parent.width; y: 5; animating: root.animating }
                PixelButton {
                    objectName: "quitButton"
                    anchors { right: parent.right; bottom: parent.bottom }
                    text: "Выйти"
                    onClicked: quitDialog.open()
                    Accessible.name: "Завершить приложение"
                }
            }
            Item { Layout.preferredHeight: 4 }
        }
    }
    Connections {
        target: root.preferences
        function onChanged() {
            if (root.shownEffect === root.preferences.effect) return
            root.shownEffect = root.preferences.effect
            if (root.animating) previewCurtain.play()
        }
    }
    PixelCurtain { id: entrance; parent: Overlay.overlay; anchors { fill: parent; margins: 4 } cell: 20; duration: 900 }
    Component.onCompleted: if (animating) entrance.play()
    MouseArea {
        anchors { right: parent.right; bottom: parent.bottom }
        width: 14
        height: 14
        cursorShape: Qt.SizeFDiagCursor
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }

    Dialog {
        id: quitDialog
        objectName: "quitDialog"
        anchors.centerIn: Overlay.overlay
        width: Math.min(380, root.width - 32)
        padding: 24
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        onOpened: cancelExit.forceActiveFocus()
        onAccepted: root.controller.quit()

        Overlay.modal: Rectangle { color: "#d90b0b0c" }
        background: PixelFrame { px: 4; fill: colors.panel; edge: colors.acid }
        contentItem: ColumnLayout {
            spacing: 14
            Row {
                spacing: 6
                Repeater {
                    model: 3
                    delegate: HeroSprite {
                        id: waver
                        required property int index
                        factor: 2
                        hero: index
                        // They jump for joy when you change your mind and look away when you leave.
                        pose: cancelExit.hovered ? "jump" : "idle"
                        mirrored: confirmExit.hovered
                        y: cancelExit.hovered ? -hopping.lift * (index === 1 ? 10 : 6) : 0
                        animating: root.animating
                        SequentialAnimation {
                            id: hopping
                            property real lift: 0
                            running: cancelExit.hovered && root.animating
                            loops: Animation.Infinite
                            PauseAnimation { duration: waver.index * 70 }
                            NumberAnimation { target: hopping; property: "lift"; to: 1; duration: 180; easing.type: Easing.OutQuad }
                            NumberAnimation { target: hopping; property: "lift"; to: 0; duration: 180; easing.type: Easing.InQuad }
                        }
                    }
                }
            }
            PixelTitle { text: "Закрыть TFX?" }
            Label {
                Layout.fillWidth: true
                text: "Вы уверены, что хотите выйти? Погодный эффект остановится."
                color: root.muted
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.topMargin: 8
                Layout.alignment: Qt.AlignRight
                spacing: 8
                PixelButton {
                    id: cancelExit
                    objectName: "cancelExitButton"
                    text: "Отмена"
                    onClicked: quitDialog.reject()
                }
                PixelButton {
                    id: confirmExit
                    objectName: "confirmExitButton"
                    text: "Выйти"
                    primary: true
                    onClicked: quitDialog.accept()
                }
            }
        }
    }
}
