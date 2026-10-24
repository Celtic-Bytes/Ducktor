import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

// Interface language selector: flag followed by the language's own name.
ComboBox {
    id: box

    function flag(code) {
        return "qrc:/resources/flags/" + code + ".svg"
    }

    model: Settings.uiLanguageOptions
    textRole: "name"
    valueRole: "code"
    implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
    Accessible.name: qsTr("Interface language")
    ToolTip.visible: hovered && !popup.visible
    ToolTip.delay: 600
    ToolTip.text: qsTr("Interface language")

    Component.onCompleted: currentIndex = indexOfValue(Settings.uiLanguage)
    onActivated: Settings.uiLanguage = currentValue

    contentItem: RowLayout {
        spacing: 8

        Item { Layout.preferredWidth: 2 }
        Image {
            source: box.currentValue ? box.flag(box.currentValue) : ""
            sourceSize: Qt.size(42, 28)
            Layout.preferredWidth: 21
            Layout.preferredHeight: 14
            fillMode: Image.PreserveAspectCrop
            Accessible.ignored: true
        }
        Label {
            text: box.displayText
            Layout.fillWidth: true
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    delegate: ItemDelegate {
        id: entry

        required property var modelData
        required property int index

        width: ListView.view ? ListView.view.width : implicitWidth
        highlighted: box.highlightedIndex === index
        Accessible.name: modelData.name

        contentItem: RowLayout {
            spacing: 8

            Image {
                source: box.flag(entry.modelData.code)
                sourceSize: Qt.size(42, 28)
                Layout.preferredWidth: 21
                Layout.preferredHeight: 14
                fillMode: Image.PreserveAspectCrop
                Accessible.ignored: true
            }
            Label {
                text: entry.modelData.name
                Layout.fillWidth: true
                font.bold: entry.modelData.code === Settings.uiLanguage
            }
        }
    }
}
