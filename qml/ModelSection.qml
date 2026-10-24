import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

ColumnLayout {
    id: section

    required property string capability
    required property var listModel
    property string intro

    function syncSelector() {
        selector.currentIndex = selector.indexOfValue(Models.selectedModel(section.capability))
    }

    spacing: 12

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.mutedText
        text: section.intro
    }

    RowLayout {
        spacing: 12

        Label {
            text: qsTr("Model in use")
            font.bold: true
        }
        ComboBox {
            id: selector
            Layout.preferredWidth: 360
            model: Models.installedModels(section.capability)
            textRole: "name"
            valueRole: "id"
            enabled: count > 0
            displayText: count > 0 ? currentText : qsTr("None installed")
            onModelChanged: Qt.callLater(section.syncSelector)
            onActivated: Models.selectModel(section.capability, currentValue)
        }
        Connections {
            target: Models
            function onRevisionChanged() {
                selector.model = Models.installedModels(section.capability)
            }
        }
    }

    ImportRow {
        Layout.fillWidth: true
    }

    ListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 10
        model: section.listModel
        delegate: ModelCard {
            capability: section.capability
        }
        ScrollBar.vertical: ScrollBar {}
    }
}
