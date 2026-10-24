import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

ApplicationWindow {
    id: window
    width: 1040
    height: 660
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: "Ducktor"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 8
            spacing: 10

            ToolButton {
                visible: stack.depth > 1
                text: qsTr("Back")
                onClicked: stack.pop()
            }
            Image {
                source: "qrc:/resources/icons/ducktor.svg"
                sourceSize: Qt.size(28, 28)
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                Accessible.ignored: true
            }
            Label {
                text: "Ducktor"
                font.bold: true
                font.pixelSize: 18
            }
            Label {
                visible: stack.depth > 1
                text: qsTr("Settings")
                opacity: 0.7
            }
            Item { Layout.fillWidth: true }
            ToolButton {
                visible: stack.depth === 1
                text: qsTr("Settings")
                onClicked: stack.push(settingsComponent)
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
        SettingsPage {}
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
