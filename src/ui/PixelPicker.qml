import QtQuick
import QtQuick.Controls

ComboBox {
    id: picker
    readonly property TfxColors colors: TfxColors {}
    implicitHeight: 40
    leftPadding: 16
    rightPadding: 32
    font.family: colors.monoFont
    font.pixelSize: 13
    font.bold: true
    contentItem: Text {
        text: picker.displayText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font: picker.font
        color: picker.colors.text
    }
    indicator: Item {
        x: picker.width - 28
        y: picker.height / 2 - 3
        Repeater {
            model: 5
            delegate: Rectangle {
                required property int index
                x: index * 3
                y: (picker.down ? Math.abs(index - 2) : 2 - Math.abs(index - 2)) * 3
                width: 3
                height: 3
                color: picker.colors.acid
            }
        }
    }
    background: PixelFrame {
        implicitWidth: 160
        fill: picker.colors.raised
        edge: picker.visualFocus ? picker.colors.cyan : picker.hovered || picker.down ? picker.colors.acid : picker.colors.line
    }
    popup: Popup {
        y: picker.height + 4
        width: Math.max(160, picker.width)
        padding: 6
        implicitHeight: contentItem.implicitHeight + 12
        background: PixelFrame { fill: picker.colors.panel; edge: picker.colors.line }
        contentItem: ListView {
            implicitHeight: contentHeight
            clip: true
            model: picker.popup.visible ? picker.delegateModel : null
            currentIndex: picker.highlightedIndex
        }
    }
    delegate: ItemDelegate {
        id: option
        required property int index
        required property string modelData
        readonly property bool selected: picker.currentIndex === index
        width: picker.popup.availableWidth
        text: modelData
        highlighted: picker.highlightedIndex === index
        contentItem: Text {
            text: (option.selected ? "× " : "  ") + option.modelData
            color: option.selected ? picker.colors.acidInk : option.highlighted || option.hovered ? picker.colors.acid : picker.colors.text
            font: picker.font
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle { color: option.selected ? picker.colors.acid : option.highlighted || option.hovered ? picker.colors.raised : "transparent" }
    }
}
