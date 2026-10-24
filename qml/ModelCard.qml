import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

Frame {
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
    required property string unavailableReason
    required property string voiceLanguage

    readonly property bool installed: status === "installed" || status === "bundled"
    readonly property bool transferring: status === "downloading" || status === "verifying"

    width: ListView.view ? ListView.view.width : implicitWidth

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            spacing: 8

            Label {
                text: card.name
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Label {
                visible: card.active
                text: qsTr("In use")
                color: palette.highlight
                font.bold: true
            }
            Label {
                visible: card.recommended
                text: qsTr("Recommended")
                opacity: 0.8
            }
            Label {
                visible: card.experimental
                text: qsTr("Experimental")
                color: "#b26a00"
            }
            Label {
                visible: card.status === "bundled"
                text: qsTr("Included")
                opacity: 0.8
            }
            Label {
                visible: card.sizeText !== ""
                text: card.sizeText
                opacity: 0.8
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
            opacity: 0.8
            text: qsTr("Languages: %1").arg(card.languagesText)
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            opacity: 0.8
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
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: card.status === "unavailable"
            color: "#b26a00"
            text: card.unavailableReason !== "" ? card.unavailableReason : qsTr("Not integrated in Ducktor yet.")
        }

        RowLayout {
            spacing: 8
            visible: card.status !== "unavailable"

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
                onClicked: Models.selectModel(card.capability, card.modelId, card.voiceLanguage)
            }
            Button {
                visible: card.status === "installed"
                text: qsTr("Delete")
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
