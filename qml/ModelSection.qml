import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Ducktor.Core

ColumnLayout {
    id: section

    required property string capability
    required property var listModel
    property string intro
    readonly property var selectorLanguages: capability === "text-to-speech" ? Models.voiceLanguages() : [""]

    spacing: 12

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: section.intro
    }

    Repeater {
        model: section.selectorLanguages

        RowLayout {
            id: selector
            required property string modelData
            spacing: 12

            function sync() {
                const index = combo.indexOfValue(Models.selectedModel(section.capability, modelData))
                combo.currentIndex = index
            }

            Label {
                text: selector.modelData === ""
                      ? qsTr("Model in use")
                      : qsTr("Voice for %1").arg(Models.languageName(selector.modelData))
            }
            ComboBox {
                id: combo
                Layout.preferredWidth: 360
                model: Models.installedModels(section.capability, selector.modelData)
                textRole: "name"
                valueRole: "id"
                enabled: count > 0
                displayText: count > 0 ? currentText : qsTr("None installed")
                onModelChanged: Qt.callLater(selector.sync)
                onActivated: Models.selectModel(section.capability, currentValue, selector.modelData)
            }
            Connections {
                target: Models
                function onRevisionChanged() {
                    combo.model = Models.installedModels(section.capability, selector.modelData)
                }
            }
        }
    }

    RowLayout {
        spacing: 12

        Button {
            text: qsTr("Import file…")
            enabled: !Models.downloading && !Models.importing
            onClicked: importDialog.open()
        }
        BusyIndicator {
            running: Models.importing
            visible: running
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            opacity: 0.75
            text: Models.importing
                  ? qsTr("Checking the file…")
                  : qsTr("Use this to install a model file you already downloaded, without an internet connection.")
        }
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 8
        model: section.listModel
        delegate: ModelCard {
            capability: section.capability
        }
        ScrollBar.vertical: ScrollBar {}
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import model file")
        nameFilters: [qsTr("Model files (*.gguf *.bin *.onnx *.json MODEL_CARD)"), qsTr("All files (*)")]
        onAccepted: Models.importFile(selectedFile)
    }
}
