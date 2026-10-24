import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

Card {
    id: card

    required property string capability
    required property string modelId
    required property string name
    required property string description
    required property string license
    required property url licenseUrl
    required property url homepage
    required property string released
    required property string sizeText
    required property string languagesText
    required property string status
    required property real progress
    required property bool active
    required property bool recommended
    required property bool experimental
    required property string voiceLanguage

    readonly property bool installed: status === "installed" || status === "bundled"
    readonly property bool transferring: status === "downloading" || status === "verifying"

    width: ListView.view ? ListView.view.width - 12 : implicitWidth

    component Tag: Label {
        property color tint: Theme.accent

        leftPadding: 8
        rightPadding: 8
        topPadding: 2
        bottomPadding: 2
        font.pixelSize: 12
        font.bold: true
        color: tint
        background: Rectangle {
            radius: height / 2
            color: "transparent"
            border.color: parent.tint
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            spacing: 8

            Label {
                text: card.name
                font.bold: true
                font.pixelSize: 15
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Tag {
                visible: card.active
                text: qsTr("In use")
                tint: Theme.success
            }
            Tag {
                visible: card.recommended
                text: qsTr("Recommended")
            }
            Tag {
                visible: card.experimental
                text: qsTr("Experimental")
                tint: Theme.warning
            }
            Tag {
                visible: card.status === "bundled"
                text: qsTr("Included")
                tint: Theme.mutedText
            }
            Label {
                visible: card.sizeText !== ""
                text: card.sizeText
                color: Theme.mutedText
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: card.description
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: card.languagesText !== ""
            color: Theme.mutedText
            text: qsTr("Languages: %1").arg(card.languagesText)
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.mutedText
            textFormat: Text.StyledText
            text: {
                const parts = []
                if (card.license !== "")
                    parts.push(qsTr("License: %1").arg("<a href=\"" + card.licenseUrl + "\">" + card.license + "</a>"))
                if (card.released !== "")
                    parts.push(qsTr("Released: %1").arg(card.released))
                if (card.homepage.toString() !== "")
                    parts.push("<a href=\"" + card.homepage + "\">" + qsTr("Model page") + "</a>")
                return parts.join(" · ")
            }
            onLinkActivated: link => Qt.openUrlExternally(link)
        }

        RowLayout {
            spacing: 8

            ProgressBar {
                Layout.fillWidth: true
                visible: card.transferring
                indeterminate: card.status === "verifying"
                value: card.progress
            }
            Label {
                visible: card.transferring
                text: card.status === "verifying" ? qsTr("Checking partial download…") : Math.floor(card.progress * 100) + " %"
            }
            Item {
                Layout.fillWidth: true
                visible: !card.transferring
            }
            Button {
                visible: card.status === "notInstalled"
                text: qsTr("Download (%1)").arg(card.sizeText)
                icon.source: "image://svg/icons/download.svg"
                icon.color: palette.buttonText
                enabled: !Models.downloading && !Models.importing
                onClicked: Models.download(card.modelId)
            }
            Button {
                visible: card.transferring
                text: qsTr("Cancel")
                onClicked: Models.cancelDownload()
            }
            Button {
                visible: card.installed && !card.active
                text: qsTr("Use")
                highlighted: true
                onClicked: Models.selectModel(card.capability, card.modelId, card.voiceLanguage)
            }
            Button {
                visible: card.status === "installed"
                text: qsTr("Delete")
                icon.source: "image://svg/icons/delete.svg"
                icon.color: palette.buttonText
                onClicked: deleteDialog.open()
            }
        }
    }

    Dialog {
        id: deleteDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Delete model")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: Models.remove(card.modelId)

        Label {
            text: qsTr("Delete %1 from this computer? You can download it again later.").arg(card.name)
            wrapMode: Text.Wrap
        }
    }
}
