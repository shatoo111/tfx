import QtQuick

// A box with one-pixel notches at the corners.
Item {
    id: frame
    property color fill: "#242426"
    property color edge: "#414145"
    property int px: 3
    Rectangle { x: frame.px; width: frame.width - 2 * frame.px; height: frame.height; color: frame.edge }
    Rectangle { y: frame.px; width: frame.width; height: frame.height - 2 * frame.px; color: frame.edge }
    Rectangle { x: frame.px; y: frame.px; width: frame.width - 2 * frame.px; height: frame.height - 2 * frame.px; color: frame.fill }
}
