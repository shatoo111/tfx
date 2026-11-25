import QtQuick
import QtQuick.Controls

// The knob is a coin: it rolls across and sparkles when the option turns on.
Switch {
    id: toggle
    readonly property TfxColors colors: TfxColors {}
    padding: 0
    implicitWidth: 58
    implicitHeight: 32
    onToggled: if (checked) sparks.burst(width - 17, height / 2)
    indicator: PixelFrame {
        implicitWidth: 58
        implicitHeight: 32
        fill: toggle.checked ? "#2c3d16" : toggle.colors.sunken
        edge: toggle.visualFocus ? toggle.colors.cyan : toggle.checked ? toggle.colors.acid : toggle.hovered ? toggle.colors.muted : toggle.colors.line
        Rectangle { x: 10; y: 14; width: 4; height: 4; color: toggle.checked ? toggle.colors.acid : "transparent" }
        Rectangle { x: 18; y: 14; width: 4; height: 4; color: toggle.checked ? toggle.colors.acid : "transparent" }
        Rectangle { x: 36; y: 14; width: 4; height: 4; color: toggle.checked ? "transparent" : toggle.colors.line }
        Rectangle { x: 44; y: 14; width: 4; height: 4; color: toggle.checked ? "transparent" : toggle.colors.line }
        CoinSprite {
            x: toggle.checked ? 32 : 6
            y: 6
            opacity: toggle.checked ? 1 : 0.4
            spinning: roll.running
            Behavior on x { NumberAnimation { id: roll; duration: 320; easing.type: Easing.OutCubic } }
        }
        PixelSparks { id: sparks; anchors.fill: parent }
    }
}
