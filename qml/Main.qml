import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

ApplicationWindow {
    id: window
    width: 1080
    height: 700
    minimumWidth: 760
    minimumHeight: 500
    visible: true
    title: "Ducktor"
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

    // Settings and About always open on top of the translator, never on top of each other.
    function openPage(component, name, properties) {
        if (stack.currentItem && stack.currentItem.objectName === name)
            return
        stack.pop(null, StackView.Immediate)
        stack.push(component, properties || {})
    }

    component HeaderButton: ToolButton {
        display: AbstractButton.TextBesideIcon
        icon.width: 18
        icon.height: 18
        icon.color: checked ? Theme.accent : Theme.text
        Accessible.name: text
    }

    header: ToolBar {
        padding: 0
        implicitHeight: 56

        background: Rectangle {
            color: Theme.surface

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.border
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 8

            Image {
                source: "qrc:/resources/icons/ducktor.svg"
                sourceSize: Qt.size(64, 64)
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Accessible.ignored: true
            }
            Label {
                text: "Ducktor"
                font.bold: true
                font.pixelSize: 20
                color: Theme.text
            }
            ToolSeparator {
                visible: stack.depth > 1
                Layout.leftMargin: 6
            }
            HeaderButton {
                visible: stack.depth > 1
                text: qsTr("Back")
                icon.source: "qrc:/resources/icons/back.svg"
                onClicked: stack.pop()
            }
            Label {
                visible: stack.depth > 1
                text: stack.currentItem ? stack.currentItem.title : ""
                font.pixelSize: 16
                color: Theme.mutedText
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Item {
                visible: stack.depth === 1
                Layout.fillWidth: true
            }
            HeaderButton {
                text: qsTr("About")
                icon.source: "qrc:/resources/icons/info.svg"
                checkable: false
                checked: stack.currentItem !== null && stack.currentItem.objectName === "about"
                onClicked: window.openPage(aboutComponent, "about")
            }
            LanguageMenu {
                Layout.preferredHeight: 34
            }
            HeaderButton {
                text: qsTr("Settings")
                icon.source: "qrc:/resources/icons/settings.svg"
                checkable: false
                checked: stack.currentItem !== null && stack.currentItem.objectName === "settings"
                onClicked: window.openPage(settingsComponent, "settings")
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: TranslatorPage {}
    }

    Component {
        id: settingsComponent
        SettingsPage {
            objectName: "settings"
            onPrivacyRequested: window.openPage(aboutComponent, "about", { initialTab: 1 })
        }
    }

    Component {
        id: aboutComponent
        AboutPage {
            objectName: "about"
        }
    }

    Connections {
        target: App
        function onNotice(message) { toast.show(message) }
    }
    Connections {
        target: Models
        function onMessage(text) { toast.show(text) }
    }

    Rectangle {
        id: toast
        property alias text: toastLabel.text

        function show(message) {
            text = message
            visible = true
            hideTimer.restart()
        }

        visible: false
        z: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: Math.min(toastLabel.implicitWidth + 32, window.width - 48)
        height: toastLabel.implicitHeight + 20
        radius: 8
        color: window.palette.toolTipBase
        border.color: window.palette.mid
        Accessible.role: Accessible.AlertMessage
        Accessible.name: text

        Label {
            id: toastLabel
            anchors.fill: parent
            anchors.margins: 10
            wrapMode: Text.Wrap
            color: window.palette.toolTipText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        MouseArea {
            anchors.fill: parent
            onClicked: toast.visible = false
        }
        Timer {
            id: hideTimer
            interval: 6000
            onTriggered: toast.visible = false
        }
    }
}
