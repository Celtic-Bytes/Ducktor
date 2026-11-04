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

    // Centers and shows the announcement window on top of the main one. Window positions are
    // absolute in the virtual desktop, so the center is computed from the main window's own
    // position.
    function showAnnouncement() {
        announcementWindow.x = window.x + (window.width - announcementWindow.width) / 2
        announcementWindow.y = window.y + (window.height - announcementWindow.height) / 2
        announcementWindow.show()
        announcementWindow.raise()
    }

    // Shown once per bundled announcement and app version; the user closes it manually.
    // The window manager places the main window only after this component completes, so the
    // announcement waits a moment for that to happen; shown immediately it would be centered
    // on the main window's initial position (0,0) and could end up on another screen than
    // the main one.
    Timer {
        interval: 500
        repeat: false
        running: Announcement.available && Announcement.unseen
        onTriggered:
            if (Announcement.available && Announcement.unseen)
                showAnnouncement()
    }



    // Hides the announcement without acknowledging it: the user only marks it as seen by
    // closing it on purpose, so it is shown again if the main window was closed first.
    onClosing:
        if (announcementWindow.visible)
            announcementWindow.hide()

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
                source: "image://svg/icons/ducktor.svg"
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
                display: AbstractButton.IconOnly
                icon.source: "image://svg/icons/back.svg"
                icon.width: 22
                icon.height: 22
                ToolTip.visible: hovered
                ToolTip.delay: 600
                ToolTip.text: text
                onClicked: stack.pop()
            }
            Item {
                Layout.fillWidth: true
            }
            HeaderButton {
                text: qsTr("About")
                icon.source: "image://svg/icons/info.svg"
                checkable: false
                checked: stack.currentItem !== null && stack.currentItem.objectName === "about"
                onClicked: window.openPage(aboutComponent, "about")
            }
            HeaderButton {
                text: qsTr("Settings")
                icon.source: "image://svg/icons/settings.svg"
                checkable: false
                checked: stack.currentItem !== null && stack.currentItem.objectName === "settings"
                onClicked: window.openPage(settingsComponent, "settings")
            }
            LanguageMenu {
                Layout.preferredHeight: 34
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

    // Declared inside the main window, so it is transient for it and stays grouped with it.
    AnnouncementWindow {
        id: announcementWindow
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
