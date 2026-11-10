import QtQuick
import Ducktor

// Wavy red underlines for the words the spell checker has flagged. The canvas is attached to
// the text input's content item (see SpellCheckPage), so it scrolls with the text; the word
// ranges come from positionToRectangle, which uses the same coordinate system.
Canvas {
    id: root

    // [{word, start, end, suggestions}] with UTF-16 offsets, as reported by the helper.
    property var issues: []
    // The TextInput whose content item hosts this canvas.
    property var textInput: null
    // Index of the issue to highlight (hover from the list or from the text); -1 otherwise.
    property int highlightIndex: -1
    // Index of the issue to flash briefly (after a click on the list); -1 otherwise.
    property int flashIndex: -1

    anchors.fill: parent

    onIssuesChanged: requestPaint()
    onTextInputChanged: requestPaint()
    onHighlightIndexChanged: requestPaint()
    onFlashIndexChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    Connections {
        target: root.textInput
        function onTextChanged() { root.requestPaint() }
    }

    onPaint: {
        const context = getContext("2d")
        context.clearRect(0, 0, width, height)
        if (!root.textInput || !root.issues || root.issues.length === 0)
            return

        for (let i = 0; i < root.issues.length; ++i) {
            const issue = root.issues[i]
            const start = root.textInput.positionToRectangle(issue.start)
            const end = root.textInput.positionToRectangle(issue.end)

            if (i === root.highlightIndex || i === root.flashIndex) {
                const x = Math.min(start.x, end.x)
                const y = Math.min(start.y, end.y)
                const right = Math.max(start.x + start.width, end.x + end.width)
                const bottom = Math.max(start.y + start.height, end.y + end.height)
                context.fillStyle = i === root.flashIndex ? "#992563eb" : "#552563eb"
                context.fillRect(x, y, right - x, bottom - y)
            }

            drawUnderline(context, start, end)
        }
    }

    function drawUnderline(context, start, end) {
        context.strokeStyle = Theme.danger
        context.lineWidth = 2

        // A squiggle from x1 to x2 at the bottom of the line at y.
        function squiggle(x1, x2, y) {
            context.beginPath()
            context.moveTo(x1, y)
            if (x2 - x1 < 6) {
                context.lineTo(x2, y)
                context.stroke()
                return
            }
            let x = x1
            let direction = 1
            while (x < x2) {
                const next = Math.min(x + 6, x2)
                context.quadraticCurveTo((x + next) / 2, y + direction * 3, next, y)
                x = next
                direction = -direction
            }
            context.stroke()
        }

        const lineBottom = start.y + start.height - 2
        if (start.y === end.y) {
            squiggle(start.x, end.x + end.width, lineBottom)
            return
        }
        // The word wraps: underline the start line, every full line in between and the end line.
        const right = root.width
        squiggle(start.x, right, lineBottom)
        for (let y = start.y + start.height; y < end.y; y += start.height)
            squiggle(0, right, y - 2)
        squiggle(0, end.x + end.width, end.y + end.height - 2)
    }
}
