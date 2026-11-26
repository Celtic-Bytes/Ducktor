import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Ducktor
import Ducktor.Core

ComboBox {
    id: box

    // The unsorted/unpartitioned language list provided by the caller.
    property var sourceModel: []

    function isFavorite(code) {
        return Settings.favoriteLanguages.indexOf(code) >= 0
    }

    function partitioned(model, favorites) {
        favorites = favorites || Settings.favoriteLanguages
        const favoriteItems = []
        const normalItems = []
        const autoItems = []
        if (model) {
            for (let i = 0; i < model.length; ++i) {
                const item = model[i]
                if (!item || item.code === undefined)
                    continue
                if (item.code === "auto")
                    autoItems.push(item)
                else if (favorites.indexOf(item.code) >= 0)
                    favoriteItems.push(item)
                else
                    normalItems.push(item)
            }
        }
        return autoItems.concat(favoriteItems).concat(normalItems)
    }

    model: partitioned(sourceModel, Settings.favoriteLanguages)

    popup: Popup {
        y: box.height
        width: box.width
        height: Math.min(contentItem.implicitHeight, (box.Window ? box.Window.height : 600) - topMargin - bottomMargin)
        topMargin: 6
        bottomMargin: 6
        palette: box.palette

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: box.delegateModel
            currentIndex: box.highlightedIndex
            highlightMoveDuration: 0
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: box.palette.window
            border.color: box.palette.mid
        }

        onOpened: Qt.callLater(function() {
            if (contentItem)
                contentItem.positionViewAtBeginning()
        })
    }

    delegate: ItemDelegate {
        id: entry

        required property var modelData
        required property int index

        width: ListView.view ? ListView.view.width : implicitWidth
        highlighted: box.highlightedIndex === index
        Accessible.name: modelData.name

        background: Rectangle {
            color: (entry.modelData.code === (box.currentValue || ""))
                ? (Theme.dark ? Qt.rgba(0.95, 0.77, 0.0, 0.14) : Qt.rgba(0.95, 0.77, 0.0, 0.22))
                : (entry.hovered ? Theme.surfaceAlt : "transparent")
            border.color: (entry.modelData.code === (box.currentValue || ""))
                ? Theme.duck : "transparent"
            border.width: (entry.modelData.code === (box.currentValue || "")) ? 1 : 0
            radius: Theme.smallRadius
        }

        contentItem: RowLayout {
            spacing: 8

            Label {
                text: entry.modelData.name
                Layout.fillWidth: true
                color: Theme.text
            }

            ToolButton {
                id: star
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                focusPolicy: Qt.StrongFocus
                activeFocusOnTab: true
                visible: entry.modelData.code !== "auto"
                enabled: entry.modelData.code !== "auto"
                contentItem: Label {
                    text: box.isFavorite(entry.modelData.code) ? "\u2605" : "\u2606"
                    color: box.isFavorite(entry.modelData.code) ? "#f2c94c" : Theme.mutedText
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                ToolTip.visible: hovered
                ToolTip.text: box.isFavorite(entry.modelData.code)
                    ? qsTr("Remove %1 from favorites").arg(entry.modelData.name)
                    : qsTr("Add %1 to favorites").arg(entry.modelData.name)
                Accessible.name: ToolTip.text
                onClicked: Settings.toggleFavoriteLanguage(entry.modelData.code)
            }
        }
    }
}
