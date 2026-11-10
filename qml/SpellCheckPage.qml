import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

// The spell-check page: the annotated text on the left, the list of errors with suggestion
// chips on the right. Clicking a word or a list item highlights both; clicking a suggestion
// replaces the word and runs a new check.
Page {
    id: page

    // Language of the current check; defaults to the interface language.
    property string language: ""
    // True once a check has finished; the status line only reports results then.
    property bool hasChecked: false
    // True from the moment a check starts, until it finishes.
    property bool wasChecking: false
    // Index of the issue highlighted by hovering the text or the list; -1 otherwise.
    property int hoveredIssue: -1
    // Index of the issue to flash briefly after a click on the list.
    property int flashIssue: -1

    readonly property bool checking: App.state === "checking-spelling"
    readonly property var issues: App.spellingIssues

    function syncCombo(box, code) {
        const index = box.indexOfValue(code)
        box.currentIndex = index >= 0 ? index : 0
        return box.currentValue
    }

    function checkNow() {
        if (page.language === "")
            return
        App.checkSpelling(input.text, page.language)
    }

    // Index of the issue covering `position` (UTF-16), or -1.
    function issueAt(position) {
        for (let i = 0; i < page.issues.length; ++i) {
            const issue = page.issues[i]
            if (position >= issue.start && position <= issue.end)
                return i
        }
        return -1
    }

    // Replaces the misspelled word with `replacement` and runs a new check.
    function applySuggestion(issueIndex, replacement) {
        const issue = App.spellingIssues[issueIndex]
        if (!issue)
            return
        suggestionsPopup.close()
        input.setTextUnchecked(input.text.slice(0, issue.start) + replacement + input.text.slice(issue.end))
        page.checkNow()
    }

    // Context line of an issue: a few characters before and after the word.
    function contextSnippet(issue) {
        const text = input.text
        const from = Math.max(0, issue.start - 24)
        const to = Math.min(text.length, issue.end + 24)
        return (from > 0 ? "…" : "") + text.slice(from, issue.start) + "«" + issue.word + "»"
             + text.slice(issue.end, to) + (to < text.length ? "…" : "")
    }

    // Positions the popup next to the word of issue `issueIndex`. The popup's x/y are relative
    // to its parent (this page), so the anchor point is mapped into the parent's coordinates.
    function positionPopup(issueIndex) {
        const issue = App.spellingIssues[issueIndex]
        if (!issue || !suggestionsPopup.visible)
            return
        const parent = suggestionsPopup.parent
        if (!parent)
            return
        const rect = input.textInput.positionToRectangle(issue.start)
        const anchor = input.textInput.mapToItem(parent, rect.x + Math.min(rect.width, 60), rect.y + rect.height + 6)
        const x = Math.max(8, Math.min(anchor.x - 10, parent.width - suggestionsPopup.width - 8))
        let y = anchor.y
        if (y + suggestionsPopup.height > parent.height - 8)
            y = anchor.y - suggestionsPopup.height - 12
        suggestionsPopup.x = x
        suggestionsPopup.y = Math.max(8, y)
    }

    // Shows the suggestion popup next to the word. `pinned` keeps it open when the mouse leaves.
    function showSuggestions(issueIndex, pinned) {
        const issue = App.spellingIssues[issueIndex]
        if (!issue)
            return
        popupCloseDelay.stop()
        suggestionsPopup.issueIndex = issueIndex
        suggestionsPopup.pinned = pinned
        // Open off-screen and position once the content has a size.
        suggestionsPopup.x = -10000
        suggestionsPopup.y = -10000
        if (!suggestionsPopup.visible)
            suggestionsPopup.open()
        Qt.callLater(function () { page.positionPopup(issueIndex) })
    }

    function closeSuggestions(force) {
        if (!suggestionsPopup.pinned || force)
            suggestionsPopup.close()
    }

    // The cursor left a flagged word (or the text area): start the grace period so it can
    // still reach the popup. Pinned popups (opened by clicking the word) are left alone.
    function wordHoverLost() {
        if (suggestionsPopup.visible && !suggestionsPopup.pinned)
            popupCloseDelay.start()
    }

    // The cursor is inside the popup (over its background or one of its buttons).
    function popupHoverIn() {
        popupCloseDelay.stop()
    }

    // The cursor left the popup: close an unpinned popup after the same grace period.
    function popupHoverOut() {
        if (!suggestionsPopup.pinned)
            popupCloseDelay.start()
    }

    // Scrolls the text so the word is visible and flashes it briefly.
    function scrollToIssue(issueIndex) {
        const issue = App.spellingIssues[issueIndex]
        if (!issue)
            return
        // Moving the cursor scrolls the text to the word and leaves the caret on it.
        input.cursorPosition = issue.start
        page.flashIssue = issueIndex
        flashTimer.restart()
    }

    background: null

    Component.onCompleted: Qt.callLater(function () {
        page.language = page.syncCombo(langBox, Settings.uiLanguage)
        // Attach the annotation items to the text content so they scroll with it.
        underlines.textInput = input.textInput
        underlines.parent = input.textInput
        hoverArea.parent = input.textInput
    })

    Timer {
        id: flashTimer
        interval: 1200
        repeat: false
        onTriggered: page.flashIssue = -1
    }

    // Grace period between the cursor leaving a flagged word (or the popup itself) and the
    // popup closing, so the cursor can travel from the word to a suggestion without the
    // popup vanishing. Entering the popup cancels the pending close.
    Timer {
        id: popupCloseDelay
        interval: 300
        repeat: false
        onTriggered: page.closeSuggestions(false)
    }

    // Keeps the popup next to the word while the text scrolls under it.
    Timer {
        interval: 100
        repeat: true
        running: suggestionsPopup.visible
        onTriggered: page.positionPopup(suggestionsPopup.issueIndex)
    }

    Connections {
        target: App
        function onStateChanged() {
            if (App.state === "checking-spelling") {
                page.wasChecking = true
            } else if (page.wasChecking) {
                // A check has finished; the results (or their absence) are now final.
                page.wasChecking = false
                page.hasChecked = true
                page.hoveredIssue = -1
                suggestionsPopup.close()
            }
        }
        function onSpellingIssuesChanged() {
            page.hoveredIssue = -1
        }
    }

    Shortcut {
        sequences: ["Ctrl+Return", "Ctrl+Enter"]
        enabled: page.visible && !App.busy
        onActivated: page.checkNow()
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 20
        columns: 2
        columnSpacing: 16
        rowSpacing: 12

        // Language, check button and the result of the last check.
        RowLayout {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                id: langBox
                Layout.preferredWidth: 220
                model: App.spellLanguages
                textRole: "name"
                valueRole: "code"
                enabled: !App.busy
                Accessible.name: qsTr("Text language")
                onModelChanged: Qt.callLater(function () { page.language = page.syncCombo(langBox, page.language) })
                onActivated: {
                    page.language = currentValue
                    // The previous results were for another language; re-check with the new one.
                    if (input.text.trim().length > 0)
                        page.checkNow()
                }
            }

            Button {
                id: checkButton
                text: qsTr("Check")
                enabled: !App.busy && input.text.trim().length > 0 && !input.overLimit
                leftPadding: 22
                rightPadding: 22
                implicitHeight: 40
                ToolTip.visible: hovered
                ToolTip.delay: 800
                ToolTip.text: "Ctrl+Enter"
                Accessible.name: text
                onClicked: page.checkNow()

                contentItem: Label {
                    text: checkButton.text
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: height / 2
                    color: checkButton.pressed ? Theme.successPressed
                                               : checkButton.hovered ? Theme.successHover : Theme.success
                    opacity: checkButton.enabled ? 1.0 : 0.45
                    border.width: checkButton.visualFocus ? 2 : 0
                    border.color: Theme.accent
                }
            }

            Item { Layout.fillWidth: true }

            BusyIndicator {
                visible: page.checking
                running: page.checking
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }
            Button {
                text: qsTr("Cancel")
                visible: page.checking
                onClicked: App.cancel()
            }

            Label {
                Layout.maximumWidth: 320
                text: page.checking ? App.statusText
                     : page.hasChecked
                       ? (page.issues.length === 1 ? qsTr("1 error found")
                          : page.issues.length > 1 ? qsTr("%1 errors found").arg(page.issues.length)
                          : qsTr("No spelling errors found"))
                       : ""
                color: page.checking ? Theme.mutedText
                     : page.hasChecked && page.issues.length > 0 ? Theme.danger
                     : page.hasChecked ? Theme.success
                     : Theme.mutedText
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                Accessible.role: Accessible.StatusBar
            }
        }

        // Left: the text with the misspelled words underlined.
        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                LimitedTextArea {
                    id: input
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    maxLength: Settings.maxInputChars
                    readOnly: App.busy
                    placeholderText: qsTr("Write or paste the text to check")
                    onLimitRejected: App.notice(qsTr("The text cannot exceed %1 characters.").arg(Settings.maxInputChars))
                    onTextChanged: {
                        // Editing invalidates the last check: the underlines would point at the wrong words.
                        if (input.length > 0) {
                            App.clearSpelling()
                            page.hasChecked = false
                        }
                    }
                    onCursorPositionChanged: {
                        // Clicking a flagged word pins its suggestions.
                        const index = page.issueAt(input.cursorPosition)
                        if (index >= 0)
                            page.showSuggestions(index, true)
                    }
                }

                Divider {}

                RowLayout {
                    spacing: 4

                    PaneButton {
                        text: qsTr("Copy")
                        icon.source: "image://svg/icons/copy.svg"
                        enabled: input.length > 0
                        onClicked: App.copyToClipboard(input.text)
                    }
                    PaneButton {
                        text: qsTr("Clear")
                        icon.source: "image://svg/icons/clear.svg"
                        enabled: !App.busy && input.length > 0
                        onClicked: {
                            input.clear()
                            App.clearSpelling()
                            page.hasChecked = false
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: qsTr("%1 / %2").arg(input.length).arg(Settings.maxInputChars)
                        color: input.overLimit ? Theme.danger : Theme.mutedText
                        Accessible.name: qsTr("%1 of %2 characters").arg(input.length).arg(Settings.maxInputChars)
                    }
                }
            }
        }

        // Right: the numbered list of errors with suggestion chips.
        Card {
            Layout.fillHeight: true
            Layout.preferredWidth: 330

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Label {
                        text: qsTr("Errors")
                        font.bold: true
                        color: Theme.text
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        visible: page.issues.length > 0
                        text: String(page.issues.length)
                        font.pixelSize: 12
                        font.bold: true
                        color: "#ffffff"
                        leftPadding: 8
                        rightPadding: 8
                        topPadding: 2
                        bottomPadding: 2
                        background: Rectangle {
                            color: Theme.danger
                            radius: height / 2
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        anchors.centerIn: parent
                        visible: !page.hasChecked && !page.checking
                        width: parent.width - 40
                        topPadding: 8
                        bottomPadding: 8
                        text: qsTr("Paste some text and press Check to see the misspelled words here.")
                        color: Theme.mutedText
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: page.hasChecked && !page.checking && page.issues.length === 0
                        topPadding: 8
                        bottomPadding: 8
                        text: qsTr("No spelling errors found")
                        color: Theme.success
                    }

                    ListView {
                        id: errorList
                        anchors.fill: parent
                        visible: page.issues.length > 0
                        model: page.issues
                        spacing: 8
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true
                        ScrollBar.vertical: ScrollBar {}

                        delegate: Item {
                            id: errorItem
                            property int issueIndex: index
                            // Same width idiom as the model and voice rows: the view width minus
                            // the vertical scrollbar gutter.
                            width: ListView.view ? ListView.view.width - 12 : implicitWidth
                            height: errorColumn.implicitHeight + 20

                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.smallRadius
                                color: page.hoveredIssue === errorItem.issueIndex ? Theme.accentSoft : "transparent"
                                border.width: page.hoveredIssue === errorItem.issueIndex ? 1 : 0
                                border.color: Theme.accent
                            }
                            // Declared before the content so the chips stay clickable; empty space scrolls or selects.
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: page.hoveredIssue = errorItem.issueIndex
                                onExited: if (page.hoveredIssue === errorItem.issueIndex) page.hoveredIssue = -1
                                onClicked: page.scrollToIssue(errorItem.issueIndex)
                            }

                            ColumnLayout {
                                id: errorColumn
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 4

                                RowLayout {
                                    spacing: 8

                                    Label {
                                        text: String(errorItem.issueIndex + 1)
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: Theme.danger
                                        leftPadding: 8
                                        rightPadding: 8
                                        topPadding: 2
                                        bottomPadding: 2
                                        background: Rectangle {
                                            color: Theme.danger
                                            opacity: 0.12
                                            radius: height / 2
                                        }
                                    }
                                    Label {
                                        text: modelData.word
                                        font.bold: true
                                        color: Theme.danger
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }
                                Label {
                                    text: page.contextSnippet(modelData)
                                    color: Theme.mutedText
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Repeater {
                                        model: modelData.suggestions

                                        Button {
                                            text: modelData
                                            hoverEnabled: true
                                            leftPadding: 10
                                            rightPadding: 10
                                            topPadding: 4
                                            bottomPadding: 4
                                            font.pixelSize: Theme.textSize - 2
                                            Accessible.name: text
                                            onClicked: page.applySuggestion(errorItem.issueIndex, modelData)

                                            contentItem: Label {
                                                text: parent.text
                                                font: parent.font
                                                color: parent.hovered ? "#ffffff" : Theme.accent
                                            }
                                            background: Rectangle {
                                                radius: height / 2
                                                color: parent.hovered ? Theme.accent : Theme.accentSoft
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Wavy underlines drawn over the text; attached to the text content in Component.onCompleted.
    SpellUnderlines {
        id: underlines
        issues: page.issues
        highlightIndex: page.hoveredIssue
        flashIndex: page.flashIssue
    }

    // Tracks the mouse over the text without intercepting clicks, so the area stays editable.
    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onPositionChanged: {
            const index = page.issues.length > 0 ? page.issueAt(input.textInput.positionAt(mouseX, mouseY)) : -1
            if (index !== page.hoveredIssue) {
                page.hoveredIssue = index
                if (index >= 0)
                    page.showSuggestions(index, false)
                else
                    page.wordHoverLost()
            }
        }
        onExited: {
            page.hoveredIssue = -1
            page.wordHoverLost()
        }
    }

    // Suggestions of the hovered or clicked word.
    Popup {
        id: suggestionsPopup
        property int issueIndex: -1
        property bool pinned: false
        modal: false
        dim: false
        padding: 10
        z: 100

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.smallRadius
        }

        contentItem: Item {
            id: popupHost
            implicitWidth: popupColumn.implicitWidth
            implicitHeight: popupColumn.implicitHeight

            // The suggestion chip under the cursor (for the highlight), or -1 when over none.
            property int hoveredChip: -1

            // Sits above every control (z: 1) and owns the hover anywhere inside the popup, so
            // the keep-open logic has a single reliable source instead of competing per-button
            // hover signals, whose enter/exit order Qt does not guarantee. It accepts no
            // clicks, so the buttons below still receive them.
            MouseArea {
                id: popupHover
                anchors.fill: parent
                z: 1
                hoverEnabled: true
                acceptedButtons: Qt.NoButton

                function refreshHover() {
                    var index = -1
                    for (var i = 0; i < chipRepeater.count; ++i) {
                        var item = chipRepeater.itemAt(i)
                        if (!item)
                            continue
                        var point = item.mapFromItem(popupHover, mouseX, mouseY)
                        if (item.contains(point)) {
                            index = i
                            break
                        }
                    }
                    popupHost.hoveredChip = index
                }
                onEntered: { page.popupHoverIn(); refreshHover() }
                onPositionChanged: { page.popupHoverIn(); refreshHover() }
                onExited: {
                    popupHost.hoveredChip = -1
                    page.popupHoverOut()
                }
            }
            ColumnLayout {
                id: popupColumn
                anchors.fill: parent
                spacing: 6

                Label {
                    text: qsTr("Suggestions")
                    font.pixelSize: 12
                    color: Theme.mutedText
                }
                Repeater {
                    id: chipRepeater
                    model: (suggestionsPopup.issueIndex >= 0 && suggestionsPopup.issueIndex < App.spellingIssues.length)
                           ? App.spellingIssues[suggestionsPopup.issueIndex].suggestions
                           : []

                    Button {
                        property int chipIndex: index
                        text: modelData
                        Layout.fillWidth: true
                        // Hover is owned by popupHover above; the chip only receives clicks.
                        hoverEnabled: false
                        leftPadding: 10
                        rightPadding: 10
                        topPadding: 4
                        bottomPadding: 4
                        font.pixelSize: Theme.textSize - 2
                        Accessible.name: text
                        onClicked: page.applySuggestion(suggestionsPopup.issueIndex, modelData)

                        contentItem: Label {
                            text: parent.text
                            font: parent.font
                            color: popupHost.hoveredChip === chipIndex ? "#ffffff" : Theme.accent
                            horizontalAlignment: Text.AlignHCenter
                        }
                        background: Rectangle {
                            radius: Theme.smallRadius
                            color: popupHost.hoveredChip === chipIndex ? Theme.accent : Theme.accentSoft
                        }
                    }
                }
                Button {
                    text: qsTr("Keep")
                    Layout.alignment: Qt.AlignHCenter
                    hoverEnabled: false
                    onClicked: suggestionsPopup.close()
                }
            }
        }
    }

    component PaneButton: ToolButton {
        display: AbstractButton.TextBesideIcon
        icon.width: 18
        icon.height: 18
        icon.color: palette.buttonText
        Accessible.name: text
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: Theme.border
    }
}
