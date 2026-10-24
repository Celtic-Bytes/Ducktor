import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

// Installs a model file the user already has, without an internet connection.
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
        color: Theme.mutedText
        text: Models.importing
              ? qsTr("Checking the file…")
              : qsTr("Use this to install a model file you already downloaded, without an internet connection.")
    }

    FileDialog {
        id: importDialog
        title: qsTr("Import model file")
        nameFilters: [qsTr("Model files (*.gguf *.bin *.onnx *.json MODEL_CARD)"), qsTr("All files (*)")]
        onAccepted: Models.importFile(selectedFile)
    }
}
