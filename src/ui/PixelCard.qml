import QtQuick
import QtQuick.Layouts

// A dark card. Its top edge is a ledge, and a hero may live on it.
Rectangle {
    id: card
    readonly property TfxColors colors: TfxColors {}
    default property alias content: body.data
    property int padding: 22
    property int spacing: 18
    property int resident: -1
    property bool animating: false
    implicitHeight: body.implicitHeight + padding * 2
    color: colors.panel
    data: [
        Rectangle { width: parent.width; height: 4; color: card.colors.line },
        ColumnLayout {
            id: body
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: card.padding }
            anchors.topMargin: card.padding + 4
            spacing: card.spacing
        },
        Loader {
            active: card.resident >= 0
            sourceComponent: Component {
                LedgeWalker { hero: card.resident; ledge: card.width; animating: card.animating }
            }
        }
    ]
}
