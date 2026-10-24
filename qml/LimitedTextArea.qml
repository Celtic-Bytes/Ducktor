import QtQuick
import QtQuick.Controls
import Ducktor
import Ducktor.Core

// Text input that refuses insertions beyond maxLength before they reach the document.
ScrollView {
    id: root

    property int maxLength: 5000
    property alias text: area.text
    property alias length: area.length
    property alias placeholderText: area.placeholderText
    property alias readOnly: area.readOnly
    readonly property bool overLimit: area.length > maxLength

    signal limitRejected()

    function clear() {
        setTextUnchecked("")
    }

    // For text produced by Ducktor itself (transcriptions, swapped translations); the counter flags any excess.
    function setTextUnchecked(value) {
        internal.bypass = true
        area.text = value
        internal.lastAccepted = value
        internal.bypass = false
    }

    function pasteGuarded() {
        if (area.readOnly)
            return
        const clip = App.clipboardText()
        const selected = area.selectionEnd - area.selectionStart
        if (!App.fitsLimit(area.length, selected, clip.length, root.maxLength)) {
            root.limitRejected()
            return
        }
        area.remove(area.selectionStart, area.selectionEnd)
        area.insert(area.cursorPosition, clip)
    }

    QtObject {
        id: internal
        property bool bypass: false
        property string lastAccepted: ""
    }

    TextArea {
        id: area
        wrapMode: TextEdit.Wrap
        textFormat: TextEdit.PlainText
        selectByMouse: true
        persistentSelection: true
        font.pixelSize: Theme.textSize
        background: null
        Accessible.name: placeholderText

        Keys.onPressed: event => {
            if (event.matches(StandardKey.Paste)) {
                root.pasteGuarded()
                event.accepted = true
            }
        }

        onTextChanged: {
            if (internal.bypass)
                return
            if (length > root.maxLength && length > internal.lastAccepted.length) {
                // Safety net for input paths not intercepted above, such as input methods or drag and drop.
                const position = Math.min(cursorPosition, internal.lastAccepted.length)
                internal.bypass = true
                text = internal.lastAccepted
                cursorPosition = position
                internal.bypass = false
                root.limitRejected()
                return
            }
            internal.lastAccepted = text
        }

        ContextMenu.menu: Menu {
            MenuItem {
                text: qsTr("Cut")
                enabled: !area.readOnly && area.selectedText.length > 0
                onTriggered: area.cut()
            }
            MenuItem {
                text: qsTr("Copy")
                enabled: area.selectedText.length > 0
                onTriggered: area.copy()
            }
            MenuItem {
                text: qsTr("Paste")
                enabled: !area.readOnly
                onTriggered: root.pasteGuarded()
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Select all")
                onTriggered: area.selectAll()
            }
        }
    }
}
