import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: row
    readonly property TfxColors colors: TfxColors {}
    property string title
    property string detail
    property Component control
    spacing: 14
    Layout.fillWidth: true
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 5
        Label { text: row.title; color: row.colors.text; font.pixelSize: 14; font.bold: true; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Label {
            text: row.detail; visible: text.length > 0
            color: row.colors.muted; font.pixelSize: 12
            Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
    }
    Loader { sourceComponent: row.control; Layout.alignment: Qt.AlignVCenter | Qt.AlignRight }
}
