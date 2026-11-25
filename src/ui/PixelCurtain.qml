import QtQuick

// Uncovers whatever lies beneath it, block by block. Call play().
ShaderEffect {
    id: curtain
    property real open: 1
    property real cell: 12
    property size size: Qt.size(width, height)
    property color color: "#151516"
    property color spark: "#b8ff5a"
    property int duration: 520
    visible: open < 1
    fragmentShader: "qrc:/tfx/shaders/curtain.frag.qsb"
    function play() { opening.restart() }
    NumberAnimation { id: opening; target: curtain; property: "open"; from: 0; to: 1; duration: curtain.duration }
}
