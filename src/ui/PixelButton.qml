import QtQuick
import QtQuick.Controls

Button {
    id: button
    readonly property TfxColors colors: TfxColors {}
    property bool selected: checked
    property bool primary: false
    readonly property bool lit: hovered || visualFocus
    hoverEnabled: true
    padding: 10
    horizontalPadding: 18
    implicitHeight: 40
    opacity: enabled ? 1 : 0.45
    background: Item {
        PixelFrame {
            y: button.down ? 2 : 0
            width: parent.width
            height: parent.height
            fill: button.selected ? button.colors.acid : button.primary ? button.colors.acid : button.colors.raised
            edge: button.visualFocus ? button.colors.cyan : button.selected || button.primary ? button.colors.acid : button.lit ? button.colors.acid : button.colors.line
        }
    }
    contentItem: Item {
        implicitWidth: label.implicitWidth
        implicitHeight: label.implicitHeight
        readonly property color tone: button.selected || button.primary ? button.colors.acidInk : button.lit ? button.colors.acid : button.colors.text
        Text {
            id: label
            y: (parent.height - height) / 2 + (button.down ? 2 : 0)
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(implicitWidth, parent.width)
            text: button.text
            elide: Text.ElideRight
            color: parent.tone
            font.family: button.colors.monoFont
            font.pixelSize: 13
            font.bold: true
        }
        // The braces step apart on hover.
        Text {
            visible: button.text.length > 0
            text: "{"
            color: parent.tone
            opacity: button.hovered ? 1 : 0
            x: label.x - (button.hovered ? 13 : 7)
            y: label.y
            font: label.font
            Behavior on x { NumberAnimation { duration: 120 } }
        }
        Text {
            visible: button.text.length > 0
            text: "}"
            color: parent.tone
            opacity: button.hovered ? 1 : 0
            x: label.x + label.width + (button.hovered ? 5 : -1)
            y: label.y
            font: label.font
            Behavior on x { NumberAnimation { duration: 120 } }
        }
    }
}
