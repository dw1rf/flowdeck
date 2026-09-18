import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 660; height: 490; visible: false
    title: "FlowDeck Palette"
    color: "#182228"
    flags: Qt.Tool | Qt.WindowStaysOnTopHint
    property var results: []
    property int choice: 0
    function search() {
        var q = input.text.toLowerCase().trim()
        var entries = flowdeck.commands
        results = entries.filter(function(c) { return q === "" || c.title.toLowerCase().indexOf(q) >= 0 || c.id.toLowerCase().indexOf(q) >= 0 })
        choice = 0
    }
    onVisibleChanged: if (visible) { input.forceActiveFocus(); input.selectAll(); search() }
    Connections { target: flowdeck; function onCommandsChanged() { root.search() } }
    Rectangle { anchors.fill: parent; color: "#182228"; border.color: "#55716e"; border.width: 1; radius: 12 }
    Column {
        anchors.fill: parent; anchors.margins: 18; spacing: 12
        TextField {
            id: input; width: parent.width; height: 54
            placeholderText: flowdeck.text("search")
            font.pixelSize: 20; color: "#f1f7f6"
            background: Rectangle { color: "#25343d"; radius: 10; border.color: "#47665f" }
            onTextChanged: root.search()
            Keys.onDownPressed: root.choice = Math.min(root.results.length-1,root.choice+1)
            Keys.onUpPressed: root.choice = Math.max(0,root.choice-1)
            Keys.onReturnPressed: { if (root.results.length) { flowdeck.runCommand(root.results[root.choice].id); root.hide() } }
            Keys.onEscapePressed: root.hide()
        }
        ListView {
            id: list; width: parent.width; height: parent.height - 82; clip: true
            model: root.results; spacing: 5; currentIndex: root.choice
            delegate: Rectangle {
                required property int index
                required property var modelData
                width: list.width; height: 54; radius: 8
                color: root.choice === index ? "#2f534f" : "#202e36"
                Row {
                    anchors.fill: parent; anchors.margins: 12; spacing: 16
                    Text { text: "⌘"; color: "#72d9c5"; font.pixelSize: 20; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        Text { text: modelData.title; color: "#f1f7f6"; font.pixelSize: 15 }
                        Text { text: modelData.hint; color: "#93adb1"; font.pixelSize: 11 }
                    }
                }
                TapHandler { onTapped: { flowdeck.runCommand(modelData.id); root.hide() } }
            }
        }
    }
}
