import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

// Top-level window for the announcement bundled with the app; shown once per id, closed by the user.
Window {
    id: root
    width: 480
    height: 460
    minimumWidth: 480
    maximumWidth: 480
    minimumHeight: 460
    maximumHeight: 460
    flags: Qt.Dialog
    visible: false
    title: Announcement.title
    color: Theme.window

    palette.window: Theme.window
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.alternateBase: Theme.surfaceAlt
    palette.text: Theme.text
    palette.button: Theme.surface
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: "#ffffff"
    palette.placeholderText: Theme.mutedText
    palette.mid: Theme.border
    palette.link: Theme.accent
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text

    // Covers the OK button, Escape and the window manager's close button alike. Qt also
    // emits closing during app shutdown for windows that were shown in the session; those
    // are not user actions, so only acknowledge a close while the window is on screen.
    onClosing:
        if (root.visible)
            Announcement.acknowledge()

    Shortcut {
        sequences: [Qt.Key_Escape]
        onActivated: root.close()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        Image {
            visible: Announcement.image !== ""
            source: Announcement.image !== "" ? "image://svg/" + Announcement.image : ""
            sourceSize: Qt.size(96, 96)
            Layout.preferredWidth: 96
            Layout.preferredHeight: 96
            Layout.alignment: Qt.AlignHCenter
            fillMode: Image.PreserveAspectFit
            Accessible.ignored: true
        }

        Label {
            text: Announcement.title
            font.bold: true
            font.pixelSize: 18
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            TextArea {
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.MarkdownText
                font.pixelSize: 16
                text: Announcement.body
                color: root.palette.text
                onLinkActivated: link => Qt.openUrlExternally(link)

                HoverHandler {
                    cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                visible: Announcement.url !== ""
                text: Announcement.urlLabel
                onClicked: Qt.openUrlExternally(Announcement.url)
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("OK")
                onClicked: root.close()
            }
        }
    }
}
