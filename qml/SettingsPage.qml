import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

Page {
    id: settingsPage

    signal privacyRequested()

    title: qsTr("Settings")
    background: null

    component SectionTitle: Label {
        font.bold: true
        font.pixelSize: 16
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

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
            VoiceSection {}

            ScrollView {
                id: generalScroll
                contentWidth: availableWidth

                ColumnLayout {
                    width: generalScroll.availableWidth - 12
                    spacing: 14

                    Card {
                        Layout.fillWidth: true

                        GridLayout {
                            anchors.fill: parent
                            columns: 2
                            columnSpacing: 16
                            rowSpacing: 10

                            SectionTitle {
                                Layout.columnSpan: 2
                                text: qsTr("Limits")
                            }
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
                                color: Theme.mutedText
                                text: qsTr("These limits keep Ducktor responsive on computers with 8 GB of memory. You can lower them; higher values will be allowed once they have been tested.")
                            }
                        }
                    }

                    Card {
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            SectionTitle { text: qsTr("Voice") }
                            Switch {
                                text: qsTr("Read the translation aloud after dictating")
                                checked: Settings.autoPlayVoiceResults
                                onToggled: Settings.autoPlayVoiceResults = checked
                            }
                        }
                    }

                    Card {
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            SectionTitle { text: qsTr("Privacy") }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                text: qsTr("Ducktor translates, transcribes and speaks on this computer. It has no account, telemetry or history, and it only connects to the internet when you download a model.")
                            }
                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WrapAnywhere
                                color: Theme.mutedText
                                text: qsTr("Models folder: %1").arg(Models.modelsDirectory)
                            }
                            Button {
                                text: qsTr("Read the privacy policy")
                                onClicked: settingsPage.privacyRequested()
                            }
                        }
                    }
                }
            }
        }
    }
}
