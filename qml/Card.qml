import QtQuick
import QtQuick.Controls
import Ducktor

// Rounded panel used for the translator panes and settings entries.
Frame {
    padding: 12

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radius
    }
}
