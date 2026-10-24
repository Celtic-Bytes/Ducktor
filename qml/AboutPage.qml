import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

Page {
    id: page

    property int initialTab: 0

    title: qsTr("About Ducktor")
    background: null

    // Relative links inside the documents point to the other bundled documents.
    function openLink(link) {
        const tabsByFile = { "PRIVACY.md": 1, "LICENSE": 2, "THIRD_PARTY_NOTICES.md": 3 }
        if (tabsByFile[link] !== undefined)
            tabs.currentIndex = tabsByFile[link]
        else if (link.startsWith("https://") || link.startsWith("http://"))
            Qt.openUrlExternally(link)
    }

    component DocumentView: ScrollView {
        id: view

        required property string documentName
        property bool markdown: true

        signal linkClicked(string link)

        contentWidth: availableWidth

        TextArea {
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            textFormat: view.markdown ? TextEdit.MarkdownText : TextEdit.PlainText
            text: App.document(view.documentName)
            onLinkActivated: link => view.linkClicked(link)

            HoverHandler {
                cursorShape: parent.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.IBeamCursor
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        TabBar {
            id: tabs
            Layout.fillWidth: true
            currentIndex: page.initialTab
            TabButton { text: qsTr("About") }
            TabButton { text: qsTr("Privacy") }
            TabButton { text: qsTr("License") }
            TabButton { text: qsTr("Third-party software") }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            ScrollView {
                id: aboutScroll
                contentWidth: availableWidth

                ColumnLayout {
                    width: aboutScroll.availableWidth
                    spacing: 10

                    Image {
                        source: "image://svg/icons/ducktor.svg"
                        sourceSize: Qt.size(96, 96)
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 96
                        Accessible.ignored: true
                    }
                    Label {
                        text: "Ducktor"
                        font.bold: true
                        font.pixelSize: 24
                    }
                    Label {
                        text: qsTr("Version %1").arg(Qt.application.version)
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Offline translator for text and voice. Your text and recordings never leave this computer.")
                    }
                    Label {
                        textFormat: Text.StyledText
                        text: "© 2026 celticbytes · <a href=\"https://celticbytes.com\">celticbytes.com</a>"
                        onLinkActivated: link => Qt.openUrlExternally(link)
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        opacity: 0.8
                        text: qsTr("Free software under the GNU General Public License, version 3 or later. It comes with no warranty: machine translations and transcriptions can contain errors.")
                    }
                }
            }

            DocumentView {
                documentName: "privacy"
                onLinkClicked: link => page.openLink(link)
            }
            DocumentView {
                documentName: "license"
                markdown: false
                onLinkClicked: link => page.openLink(link)
            }
            DocumentView {
                documentName: "notices"
                onLinkClicked: link => page.openLink(link)
            }
        }
    }
}
