import QtQuick
import "LandData.js" as LandData

// One part of a land: "sky" is the light above the horizon, "hills" the ridges, the road and the weather.
Item {
    id: land
    property int theme: 0
    property string part: "hills"
    property int factor: 3
    property bool animating: true
    property real roadX: 0
    property real speed: 44
    readonly property real horizon: height - LandData.ground * factor
    readonly property int columns: Math.ceil(width / factor)
    readonly property var info: LandData.lands[theme]

    component Strip: Image {
        smooth: false
        transformOrigin: Item.TopLeft
        scale: land.factor
        fillMode: Image.TileHorizontally
        horizontalAlignment: Image.AlignLeft
    }
    Strip {
        visible: land.part === "sky"
        y: land.horizon - 60 * land.factor
        width: land.columns
        height: 60
        source: "qrc:/assets/pixel/glow-" + land.theme + ".png"
    }
    Repeater {
        model: land.part === "hills" ? [["far", 0.12], ["near", 0.3]] : []
        delegate: Strip {
            id: ridge
            required property var modelData
            y: land.horizon - 64 * land.factor
            width: land.columns + LandData.tile
            height: 64
            source: "qrc:/assets/pixel/" + modelData[0] + "-" + land.theme + ".png"
            NumberAnimation on x {
                from: 0; to: -LandData.tile * land.factor
                duration: LandData.tile / (land.speed * ridge.modelData[1]) * 1000
                loops: Animation.Infinite
                running: land.animating
            }
        }
    }
    Strip {
        visible: land.part === "hills"
        x: -land.roadX * land.factor
        y: land.height - LandData.height * land.factor
        width: land.columns + LandData.length
        height: LandData.height
        source: "qrc:/assets/pixel/road-" + land.theme + ".png"
    }
    Item {
        visible: land.part === "hills"
        width: land.width
        height: land.horizon
        clip: true
        Repeater {
            model: land.part === "hills" ? land.info.weather : []
            delegate: Image {
                id: fall
                required property int index
                required property var modelData
                smooth: false
                transformOrigin: Item.TopLeft
                scale: land.factor
                fillMode: Image.Tile
                horizontalAlignment: Image.AlignLeft
                verticalAlignment: Image.AlignTop
                width: land.columns + modelData.w
                height: Math.ceil(land.horizon / land.factor) + modelData.h
                source: "qrc:/assets/pixel/weather-" + land.theme + "-" + index + ".png"
                NumberAnimation on x {
                    from: 0; to: -fall.modelData.w * land.factor
                    duration: fall.modelData.seconds * 1000
                    loops: Animation.Infinite
                    running: land.animating
                }
                NumberAnimation on y {
                    from: fall.modelData.dy > 0 ? -fall.modelData.h * land.factor : 0
                    to: fall.modelData.dy < 0 ? -fall.modelData.h * land.factor : 0
                    duration: fall.modelData.seconds * 1000
                    loops: Animation.Infinite
                    running: land.animating && fall.modelData.dy !== 0
                }
            }
        }
    }
}
