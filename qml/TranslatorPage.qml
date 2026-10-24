import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

Page {
    id: page

    property string sourceCode: "auto"
    property string targetCode: Qt.locale().name.startsWith("es") ? "en" : "es"
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            spacing: 8

            ComboBox {
                id: sourceBox
                Layout.fillWidth: true
                model: App.sourceLanguages
                textRole: "name"
                valueRole: "code"
                enabled: !App.busy
                Accessible.name: qsTr("Source language")
                onModelChanged: page.sourceCode = page.syncCombo(sourceBox, page.sourceCode)
                Component.onCompleted: page.sourceCode = page.syncCombo(sourceBox, page.sourceCode)
                onActivated: page.sourceCode = currentValue
            }
            Button {
                text: "⇄"
                enabled: !App.busy
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Swap languages")
                Accessible.name: qsTr("Swap languages")
                onClicked: page.swapLanguages()
            }
            ComboBox {
                id: targetBox
                Layout.fillWidth: true
                model: App.targetLanguages
                textRole: "name"
                valueRole: "code"
                enabled: !App.busy
                Accessible.name: qsTr("Target language")
                onModelChanged: page.targetCode = page.syncCombo(targetBox, page.targetCode)
                Component.onCompleted: page.targetCode = page.syncCombo(targetBox, page.targetCode)
                onActivated: page.targetCode = currentValue
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

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

                    RowLayout {
                        spacing: 8

                        Button {
                            text: page.recording
                                  ? qsTr("Stop %1 / %2").arg(page.formatSeconds(App.recordingSeconds)).arg(page.formatSeconds(Settings.maxRecordingSeconds))
                                  : qsTr("Dictate")
                            enabled: page.recording || !App.busy
                            highlighted: page.recording
                            onClicked: page.recording ? App.stopRecording() : App.startRecording(page.sourceCode)
                        }
                        Button {
                            text: qsTr("Clear")
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
                            opacity: 0.8
                        }
                        Label {
                            text: qsTr("%1 / %2").arg(input.length).arg(Settings.maxInputChars)
                            color: input.overLimit ? "#c62828" : palette.windowText
                            opacity: input.overLimit ? 1.0 : 0.7
                            Accessible.name: qsTr("%1 of %2 characters").arg(input.length).arg(Settings.maxInputChars)
                        }
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextArea {
                            text: App.translatedText
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                            textFormat: TextEdit.PlainText
                            placeholderText: qsTr("The translation will appear here")
                            Accessible.name: qsTr("Translation")
                        }
                    }

                    RowLayout {
                        spacing: 8

                        Button {
                            text: qsTr("Copy")
                            enabled: App.translatedText.length > 0
                            onClicked: App.copyToClipboard(App.translatedText)
                        }
                        Button {
                            text: App.playing ? qsTr("Stop audio") : qsTr("Listen")
                            enabled: App.playing || (!App.busy && App.translatedText.length > 0)
                            onClicked: App.playing ? App.stopAudio() : App.speak(App.translatedText, page.targetCode)
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            visible: !App.translationComplete
                            text: qsTr("Possibly incomplete")
                            color: "#c62828"
                        }
                    }
                }
            }
        }

        RowLayout {
            spacing: 12

            BusyIndicator {
                running: App.busy && !page.recording
                visible: running
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
            }
            Label {
                Layout.fillWidth: true
                text: App.statusText
                elide: Text.ElideRight
                Accessible.role: Accessible.StatusBar
            }
            Button {
                text: qsTr("Cancel")
                visible: App.busy
                onClicked: App.cancel()
            }
            Button {
                text: qsTr("Translate")
                highlighted: true
                enabled: !App.busy && input.text.trim().length > 0 && !input.overLimit
                onClicked: page.translateNow()
            }
        }
    }
}
