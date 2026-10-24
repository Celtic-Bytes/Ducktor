import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

// Compact table: one row per language with its female and male voice.
ColumnLayout {
    id: section

    property string pendingId
    property string pendingName

    function confirmDelete(id, name) {
        pendingId = id
        pendingName = name
        deleteDialog.open()
    }

    function qualityName(quality) {
        switch (quality) {
        case "high": return qsTr("high")
        case "medium": return qsTr("medium")
        case "low": return qsTr("low")
        case "x_low": return qsTr("very low")
        default: return quality || ""
        }
    }

    spacing: 10

    component VoiceCell: RowLayout {
        id: cell

        property var voice: ({})
        property string language
        property string qualityText
        readonly property bool present: voice !== undefined && voice.id !== undefined
        readonly property bool transferring: present && (voice.status === "downloading" || voice.status === "verifying")
        readonly property bool installed: present && (voice.status === "installed" || voice.status === "bundled")

        signal deleteRequested(string id, string name)

        spacing: 4

        Label {
            Layout.fillWidth: true
            text: cell.present ? cell.voice.name : qsTr("Not available")
            color: cell.present ? Theme.text : Theme.mutedText
            elide: Text.ElideRight

            HoverHandler { id: nameHover }
            ToolTip.visible: cell.present && nameHover.hovered
            ToolTip.delay: 400
            ToolTip.text: {
                if (!cell.present)
                    return ""
                const lines = [cell.voice.locale.replace("_", "-") + " · " + qsTr("Quality: %1").arg(cell.qualityText),
                               qsTr("License: %1").arg(cell.voice.license)]
                if (cell.voice.shared)
                    lines.push(qsTr("One download includes both voices."))
                return lines.join("\n")
            }
        }
        ToolButton {
            visible: cell.present
            icon.source: "image://svg/icons/info.svg"
            icon.width: 16
            icon.height: 16
            icon.color: Theme.mutedText
            ToolTip.visible: hovered
            ToolTip.text: qsTr("License and details")
            Accessible.name: qsTr("License and details")
            onClicked: Qt.openUrlExternally(cell.voice.licenseUrl)
        }
        ProgressBar {
            visible: cell.transferring
            Layout.preferredWidth: 80
            indeterminate: cell.present && cell.voice.status === "verifying"
            value: cell.present ? cell.voice.progress : 0
        }
        ToolButton {
            visible: cell.transferring
            icon.source: "image://svg/icons/clear.svg"
            icon.width: 16
            icon.height: 16
            icon.color: Theme.text
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Cancel")
            Accessible.name: qsTr("Cancel")
            onClicked: Models.cancelDownload()
        }
        Button {
            visible: cell.present && cell.voice.status === "notInstalled"
            text: cell.present ? cell.voice.sizeText : ""
            icon.source: "image://svg/icons/download.svg"
            icon.width: 16
            icon.height: 16
            icon.color: palette.buttonText
            enabled: !Models.downloading && !Models.importing
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Download")
            Accessible.name: qsTr("Download")
            onClicked: Models.download(cell.voice.id)
        }
        Button {
            visible: cell.installed && !cell.voice.active
            text: qsTr("Use")
            onClicked: Models.selectVoice(cell.language, cell.voice.id, cell.voice.gender)
        }
        Label {
            visible: cell.installed && cell.voice.active
            text: "✓ " + qsTr("In use")
            color: Theme.success
            font.bold: true
            rightPadding: 6
        }
        ToolButton {
            visible: cell.present && cell.voice.status === "installed"
            icon.source: "image://svg/icons/delete.svg"
            icon.width: 16
            icon.height: 16
            icon.color: Theme.mutedText
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Delete")
            Accessible.name: qsTr("Delete")
            onClicked: cell.deleteRequested(cell.voice.id, cell.voice.name)
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.mutedText
        text: qsTr("Voices that read translations aloud. Each language can have a female and a male voice; text translation works without them.")
    }

    ImportRow {
        Layout.fillWidth: true
    }

    Card {
        Layout.fillWidth: true
        Layout.fillHeight: true
        padding: 6

        ColumnLayout {
            anchors.fill: parent
            spacing: 2

            RowLayout {
                Layout.leftMargin: 12
                Layout.rightMargin: 20
                Layout.topMargin: 4
                Layout.bottomMargin: 4
                spacing: 16

                Label {
                    text: qsTr("Language")
                    font.bold: true
                    color: Theme.mutedText
                    Layout.preferredWidth: 160
                }
                Label {
                    text: qsTr("Female voice")
                    font.bold: true
                    color: Theme.mutedText
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                }
                Label {
                    text: qsTr("Male voice")
                    font.bold: true
                    color: Theme.mutedText
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: Models.voiceTable
                ScrollBar.vertical: ScrollBar {}

                delegate: Rectangle {
                    id: row

                    required property int index
                    required property string language
                    required property string languageName
                    required property var female
                    required property var male

                    width: ListView.view.width - 12
                    height: 40
                    radius: Theme.smallRadius
                    color: index % 2 === 0 ? Theme.surfaceAlt : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        spacing: 16

                        Label {
                            text: row.languageName
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.preferredWidth: 160
                        }
                        VoiceCell {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            voice: row.female
                            language: row.language
                            qualityText: section.qualityName(row.female.quality)
                            onDeleteRequested: (id, name) => section.confirmDelete(id, name)
                        }
                        VoiceCell {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            voice: row.male
                            language: row.language
                            qualityText: section.qualityName(row.male.quality)
                            onDeleteRequested: (id, name) => section.confirmDelete(id, name)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: deleteDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: qsTr("Delete voice")
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: Models.remove(section.pendingId)

        Label {
            text: qsTr("Delete %1 from this computer? You can download it again later.").arg(section.pendingName)
            wrapMode: Text.Wrap
        }
    }
}
