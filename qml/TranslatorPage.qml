import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

Page {
    id: page

    property string sourceCode: "auto"
    property string targetCode: ""
    property bool voiceFlow: false
    readonly property bool recording: App.state === "recording"

    function syncCombo(box, code) {
        const index = box.indexOfValue(code)
        box.currentIndex = index >= 0 ? index : 0
        return box.currentValue
    }

    function translateNow() {
        App.translate(input.text, sourceCode, targetCode)
    }

    function swapLanguages() {
        const newTarget = sourceCode === "auto" ? App.detectedLanguage : sourceCode
        if (newTarget === "" || targetBox.indexOfValue(newTarget) < 0) {
            App.notice(qsTr("Choose the source language before swapping."))
            return
        }
        const newSource = targetCode
        targetCode = syncCombo(targetBox, newTarget)
        sourceCode = syncCombo(sourceBox, newSource)
        if (App.translatedText.length > 0) {
            input.setTextUnchecked(App.translatedText)
            App.clearTranslation()
        }
        voiceFlow = false
        App.setVoiceInput(false)
    }

    function formatSeconds(value) {
        const total = Math.floor(value)
        const minutes = Math.floor(total / 60)
        const seconds = total % 60
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    background: null
    Component.onCompleted: targetCode = Settings.uiLanguage === "en" ? "es" : "en"

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

    Connections {
        target: App
        function onTranscriptReady(text, language) {
            input.setTextUnchecked(text)
            page.voiceFlow = true
        }
    }

    Shortcut {
        sequences: ["Ctrl+Return", "Ctrl+Enter"]
        enabled: !App.busy
        onActivated: page.translateNow()
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 20
        columns: 3
        columnSpacing: 16
        rowSpacing: 12

        ComboBox {
            id: sourceBox
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            model: App.sourceLanguages
            textRole: "name"
            valueRole: "code"
            enabled: !App.busy
            Accessible.name: qsTr("Source language")
            onModelChanged: Qt.callLater(() => page.sourceCode = page.syncCombo(sourceBox, page.sourceCode))
            Component.onCompleted: Qt.callLater(() => page.sourceCode = page.syncCombo(sourceBox, page.sourceCode))
            onActivated: page.sourceCode = currentValue
        }
        RoundButton {
            Layout.alignment: Qt.AlignHCenter
            icon.source: "qrc:/resources/icons/swap.svg"
            icon.color: Theme.text
            enabled: !App.busy
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Swap languages")
            Accessible.name: qsTr("Swap languages")
            onClicked: page.swapLanguages()
        }
        ComboBox {
            id: targetBox
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            model: App.targetLanguages
            textRole: "name"
            valueRole: "code"
            enabled: !App.busy
            Accessible.name: qsTr("Target language")
            onModelChanged: Qt.callLater(() => page.targetCode = page.syncCombo(targetBox, page.targetCode))
            Component.onCompleted: Qt.callLater(() => page.targetCode = page.syncCombo(targetBox, page.targetCode))
            onActivated: page.targetCode = currentValue
        }

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
                    placeholderText: qsTr("Write, paste or dictate the text to translate")
                    onLimitRejected: App.notice(qsTr("The text cannot exceed %1 characters.").arg(Settings.maxInputChars))
                    onTextChanged: {
                        if (text.length === 0) {
                            page.voiceFlow = false
                            App.setVoiceInput(false)
                        }
                    }
                }

                Divider {}

                RowLayout {
                    spacing: 4

                    PaneButton {
                        text: page.recording
                              ? qsTr("Stop %1 / %2").arg(page.formatSeconds(App.recordingSeconds)).arg(page.formatSeconds(Settings.maxRecordingSeconds))
                              : qsTr("Dictate")
                        icon.source: page.recording ? "qrc:/resources/icons/stop.svg" : "qrc:/resources/icons/mic.svg"
                        palette.buttonText: page.recording ? Theme.danger : Theme.text
                        enabled: page.recording || !App.busy
                        onClicked: page.recording ? App.stopRecording() : App.startRecording(page.sourceCode)
                    }
                    PaneButton {
                        text: qsTr("Clear")
                        icon.source: "qrc:/resources/icons/clear.svg"
                        enabled: !App.busy && input.length > 0
                        onClicked: {
                            input.clear()
                            App.clearTranslation()
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        visible: page.voiceFlow && page.sourceCode === "auto" && App.detectedLanguage !== ""
                        text: qsTr("Detected: %1").arg(App.languageName(App.detectedLanguage))
                        color: Theme.mutedText
                    }
                    Label {
                        text: qsTr("%1 / %2").arg(input.length).arg(Settings.maxInputChars)
                        color: input.overLimit ? Theme.danger : Theme.mutedText
                        Accessible.name: qsTr("%1 of %2 characters").arg(input.length).arg(Settings.maxInputChars)
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillHeight: true
            spacing: 12

            Item { Layout.fillHeight: true }
            Button {
                id: translateButton
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Translate")
                enabled: !App.busy && input.text.trim().length > 0 && !input.overLimit
                leftPadding: 22
                rightPadding: 22
                implicitHeight: 48
                ToolTip.visible: hovered
                ToolTip.delay: 800
                ToolTip.text: "Ctrl+Enter"
                Accessible.name: text
                onClicked: page.translateNow()

                contentItem: Label {
                    text: translateButton.text + "  →"
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: height / 2
                    color: translateButton.pressed ? Theme.successPressed
                                                   : translateButton.hovered ? Theme.successHover : Theme.success
                    opacity: translateButton.enabled ? 1.0 : 0.45
                    border.width: translateButton.visualFocus ? 2 : 0
                    border.color: Theme.accent
                }
            }
            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                running: App.busy && !page.recording
                visible: running
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
            }
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Cancel")
                visible: App.busy
                onClicked: App.cancel()
            }
            Item { Layout.fillHeight: true }
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1

            background: Rectangle {
                color: Theme.surfaceAlt
                border.color: Theme.border
                radius: Theme.radius
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        text: App.translatedText
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        textFormat: TextEdit.PlainText
                        font.pixelSize: Theme.textSize
                        background: null
                        placeholderText: qsTr("The translation will appear here")
                        Accessible.name: qsTr("Translation")
                    }
                }

                Divider {}

                RowLayout {
                    spacing: 4

                    PaneButton {
                        text: qsTr("Copy")
                        icon.source: "qrc:/resources/icons/copy.svg"
                        enabled: App.translatedText.length > 0
                        onClicked: App.copyToClipboard(App.translatedText)
                    }
                    PaneButton {
                        text: App.playing ? qsTr("Stop audio") : qsTr("Listen")
                        icon.source: App.playing ? "qrc:/resources/icons/stop.svg" : "qrc:/resources/icons/volume.svg"
                        enabled: App.playing || (!App.busy && App.translatedText.length > 0)
                        onClicked: App.playing ? App.stopAudio() : App.speak(App.translatedText, page.targetCode)
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        visible: !App.translationComplete
                        text: qsTr("Possibly incomplete")
                        color: Theme.danger
                    }
                }
            }
        }

        Label {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            text: App.statusText
            color: Theme.mutedText
            elide: Text.ElideRight
            Accessible.role: Accessible.StatusBar
        }
    }
}
