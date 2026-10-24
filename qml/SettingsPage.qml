import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor.Core

Page {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: qsTr("Translation") }
            TabButton { text: qsTr("Speech recognition") }
            TabButton { text: qsTr("Voices") }
            TabButton { text: qsTr("General") }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            ModelSection {
                capability: "translation"
                listModel: Models.translationModels
                intro: qsTr("Models that translate text. Larger files usually translate better but need more memory and time.")
            }
            ModelSection {
                capability: "speech-to-text"
                listModel: Models.speechToTextModels
                intro: qsTr("Models that turn your voice into text. They detect the spoken language when the source language is set to automatic.")
            }
            ModelSection {
                capability: "text-to-speech"
                listModel: Models.textToSpeechModels
                intro: qsTr("Voices that read translations aloud. Each voice speaks one language; text translation works without them.")
            }

            ScrollView {
                id: generalScroll
                contentWidth: availableWidth

                ColumnLayout {
                    width: generalScroll.availableWidth
                    spacing: 16

                    GroupBox {
                        title: qsTr("Limits")
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            columnSpacing: 16
                            rowSpacing: 8

                            Label { text: qsTr("Maximum text length (characters)") }
                            SpinBox {
                                from: Settings.minInputChars
                                to: Settings.hardMaxInputChars
                                stepSize: 500
                                editable: true
                                value: Settings.maxInputChars
                                onValueModified: Settings.maxInputChars = value
                            }
                            Label { text: qsTr("Maximum recording length (seconds)") }
                            SpinBox {
                                from: Settings.minRecordingSeconds
                                to: Settings.hardMaxRecordingSeconds
                                stepSize: 10
                                editable: true
                                value: Settings.maxRecordingSeconds
                                onValueModified: Settings.maxRecordingSeconds = value
                            }
                            Label {
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                opacity: 0.75
                                text: qsTr("These limits keep Ducktor responsive on computers with 8 GB of memory. You can lower them; higher values will be allowed once they have been tested.")
                            }
                        }
                    }

                    GroupBox {
                        title: qsTr("Voice")
                        Layout.fillWidth: true

                        Switch {
                            text: qsTr("Read the translation aloud after dictating")
                            checked: Settings.autoPlayVoiceResults
                            onToggled: Settings.autoPlayVoiceResults = checked
                        }
                    }

                    GroupBox {
                        title: qsTr("Privacy")
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                text: qsTr("Ducktor translates, transcribes and speaks on this computer. It has no account, telemetry or history, and it only connects to the internet when you download a model.")
                            }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WrapAnywhere
                                opacity: 0.75
                                text: qsTr("Models folder: %1").arg(Models.modelsDirectory)
                            }
                        }
                    }
                }
            }
        }
    }
}
