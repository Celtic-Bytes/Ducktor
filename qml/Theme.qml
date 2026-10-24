pragma Singleton
import QtQuick

// Shared colors and sizes; follows the system light or dark preference.
QtObject {
    readonly property bool dark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark

    readonly property color window: dark ? "#15171c" : "#eef1f5"
    readonly property color surface: dark ? "#1f2229" : "#ffffff"
    readonly property color surfaceAlt: dark ? "#252932" : "#f6f8fa"
    readonly property color border: dark ? "#323743" : "#dde2e8"
    readonly property color text: dark ? "#e7e9ed" : "#1d232b"
    readonly property color mutedText: dark ? "#9aa3ae" : "#5d6875"
    readonly property color accent: dark ? "#5b9cff" : "#2563eb"
    readonly property color accentSoft: dark ? "#1e3355" : "#e6eeff"
    readonly property color success: "#16a34a"
    readonly property color successHover: "#15924a"
    readonly property color successPressed: "#11753b"
    readonly property color danger: dark ? "#f87171" : "#c62828"
    readonly property color warning: dark ? "#f0b45a" : "#b26a00"
    readonly property color duck: "#f5b400"

    readonly property int radius: 10
    readonly property int smallRadius: 6
    readonly property int textSize: 15
}
