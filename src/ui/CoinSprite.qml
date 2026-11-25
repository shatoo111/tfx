import QtQuick

Item {
    id: coin
    property int factor: 2
    property bool spinning: false
    property int frame: 0
    width: 10 * factor
    height: 10 * factor
    onSpinningChanged: if (!spinning) frame = 0
    Timer {
        interval: 80
        repeat: true
        running: coin.spinning && coin.visible
        onTriggered: coin.frame = (coin.frame + 1) % 4
    }
    Image {
        anchors.fill: parent
        smooth: false
        source: "qrc:/assets/pixel/coin-" + coin.frame + ".png"
    }
}
