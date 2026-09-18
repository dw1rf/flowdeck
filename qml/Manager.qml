import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1250; height: 820; minimumWidth: 950; minimumHeight: 660
    visible: true
    title: "FlowDeck"
    color: "#10151a"
    property color accent: flowdeck.settings.accent || "#63d8c7"
    property int page: 0
    property int zoneIndex: -1
    property bool restoreVisible: flowdeck.restorationSummary() !== "0 matched, 0 missing"
    font.family: "Segoe UI"
    font.pixelSize: 14 * (flowdeck.settings.scale || 1)

    component ActionButton: Button {
        id: button
        property bool primary: false
        implicitHeight: 36
        background: Rectangle {
            radius: 9
            color: button.primary ? root.accent : (button.hovered ? "#303b44" : "#263039")
            border.color: button.primary ? root.accent : "#40505b"
        }
        contentItem: Text {
            text: button.text; color: button.primary ? "#10201f" : "#e6eef0"
            font: button.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
    }
    component Field: TextField {
        color: "#e7eff0"; selectionColor: root.accent; selectedTextColor: "#10151a"
        background: Rectangle { radius: 7; color: "#202a32"; border.color: parent.activeFocus ? root.accent : "#3a4a55" }
        implicitHeight: 36
    }
    component LabelText: Text { color: "#a9b8be"; font.pixelSize: 12 * (flowdeck.settings.scale || 1) }
    component SelectBox: ComboBox {
        id: box
        implicitHeight: 36
        contentItem: Text { text: box.displayText; color: "#e7eff0"; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
        background: Rectangle { radius: 7; color: "#202a32"; border.color: "#3a4a55" }
        delegate: ItemDelegate { width: box.width; text: modelData; highlighted: box.highlightedIndex === index }
    }

    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 220; Layout.fillHeight: true; color: "#151d23"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 20; spacing: 8
                Text { text: "◧  FlowDeck"; color: root.accent; font.pixelSize: 22; font.bold: true; Layout.bottomMargin: 20 }
                Repeater {
                    model: [flowdeck.text("spaces"), flowdeck.text("editor"), flowdeck.text("plugins"), flowdeck.text("settings")]
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true; height: 44; radius: 9
                        color: root.page === index ? "#29433f" : "transparent"
                        Text { anchors.centerIn: parent; text: modelData; color: root.page === index ? root.accent : "#b1c0c6"; font.pixelSize: 15 }
                        TapHandler { onTapped: root.page = index }
                    }
                }
                Item { Layout.fillHeight: true }
                Text { text: "Ctrl+Alt+Space  ·  Palette"; color: "#71848d"; font.pixelSize: 11 }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 24; spacing: 18
            RowLayout {
                Layout.fillWidth: true
                Text { text: [flowdeck.text("spaces"),flowdeck.text("editor"),flowdeck.text("plugins"),flowdeck.text("settings")][root.page]; color: "#f1f7f6"; font.pixelSize: 26; font.bold: true }
                Item { Layout.fillWidth: true }
                ActionButton { text: flowdeck.text("undo"); onClicked: flowdeck.undo() }
                ActionButton { text: "⟳"; onClicked: flowdeck.refresh() }
            }
            Rectangle {
                visible: root.restoreVisible && root.page === 0
                Layout.fillWidth: true; implicitHeight: 75; radius: 10; color: "#2c352e"; border.color: "#738553"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    ColumnLayout {
                        Text { text: flowdeck.text("restoreHint"); color: "#eaf1dc" }
                        LabelText { text: flowdeck.restorationSummary() }
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: flowdeck.text("restore"); onClicked: restoreDialog.open() }
                    ActionButton { text: "×"; onClicked: root.restoreVisible = false }
                }
            }
            RowLayout {
                visible: root.page < 2
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
                Rectangle {
                    Layout.preferredWidth: 228; Layout.fillHeight: true; radius: 12; color: "#1a242b"
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 12; spacing: 10
                        RowLayout {
                            LabelText { text: flowdeck.text("spaces") }
                            Item { Layout.fillWidth: true }
                            ActionButton { text: "+"; onClicked: flowdeck.createWorkspace() }
                        }
                        ListView {
                            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 6
                            model: flowdeck.workspaces
                            delegate: Rectangle {
                                required property int index
                                required property var modelData
                                width: ListView.view.width; height: 55; radius: 8
                                color: flowdeck.selectedIndex === index ? "#2b4945" : "#232f37"
                                Column {
                                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; anchors.leftMargin: 12
                                    Text { text: modelData.name; color: "#f1f6f5"; font.bold: true }
                                    Text { text: modelData.hotkey || "—"; color: "#8ca4aa"; font.pixelSize: 11 }
                                }
                                TapHandler { onTapped: flowdeck.selectWorkspace(index) }
                            }
                        }
                        RowLayout {
                            ActionButton { text: flowdeck.text("duplicate"); onClicked: flowdeck.duplicateWorkspace() }
                            ActionButton { text: flowdeck.text("delete"); onClicked: flowdeck.deleteWorkspace() }
                        }
                    }
                }
                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    ColumnLayout {
                        width: parent.width - 16; spacing: 16
                        RowLayout {
                            Layout.fillWidth: true
                            Field { Layout.fillWidth: true; text: flowdeck.selectedWorkspace.name || ""; onEditingFinished: flowdeck.changeWorkspace("name", text) }
                            ActionButton { text: flowdeck.text("preview"); onClicked: flowdeck.refresh() }
                            ActionButton { text: flowdeck.text("apply"); primary: true; enabled: flowdeck.preview.placements.length > 0; onClicked: flowdeck.applySelected() }
                        }
                        Rectangle {
                            Layout.fillWidth: true; implicitHeight: 300; radius: 14
                            color: "#1a252d"; border.color: "#3c525b"
                            Item {
                                id: canvas
                                anchors.centerIn: parent
                                width: Math.min(parent.width - 42, (parent.height - 52) * Math.max(1, flowdeck.preview.canvas.width) / Math.max(1, flowdeck.preview.canvas.height))
                                height: Math.min(parent.height - 52, width * Math.max(1, flowdeck.preview.canvas.height) / Math.max(1, flowdeck.preview.canvas.width))
                                Rectangle { anchors.fill: parent; radius: 5; color: "#12212a"; border.color: root.accent }
                                Repeater {
                                    model: flowdeck.selectedWorkspace.zones || []
                                    delegate: Rectangle {
                                        id: zone
                                        required property int index
                                        required property var modelData
                                        x: modelData.x * canvas.width; y: modelData.y * canvas.height
                                        width: modelData.w * canvas.width; height: modelData.h * canvas.height
                                        radius: 6; color: ["#365c65","#534a74","#65533d","#3b6250"][index % 4]
                                        border.color: root.zoneIndex === index ? "#ffffff" : "#82a4a9"; border.width: root.zoneIndex === index ? 2 : 1
                                        Text { anchors.centerIn: parent; width: parent.width - 10; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: "#f3f8f9"; font.bold: true; text: zone.modelData.label + "\n" + (zone.modelData.executable || flowdeck.text("noWindow")) }
                                        DragHandler {
                                            target: null
                                            onActiveChanged: if (!active) flowdeck.moveZone(zone.index, zone.x/canvas.width, zone.y/canvas.height, zone.width/canvas.width, zone.height/canvas.height)
                                            onTranslationChanged: { zone.x = Math.max(0,Math.min(canvas.width-zone.width,zone.modelData.x*canvas.width+translation.x)); zone.y = Math.max(0,Math.min(canvas.height-zone.height,zone.modelData.y*canvas.height+translation.y)) }
                                        }
                                        TapHandler { onTapped: root.zoneIndex = zone.index }
                                        Rectangle {
                                            width: 12; height: 12; radius: 3; color: root.accent
                                            anchors.right: parent.right; anchors.bottom: parent.bottom
                                            DragHandler {
                                                target: null
                                                onActiveChanged: if (!active) flowdeck.moveZone(zone.index, zone.x/canvas.width, zone.y/canvas.height, zone.width/canvas.width, zone.height/canvas.height)
                                                onTranslationChanged: { zone.width = Math.max(30,Math.min(canvas.width-zone.x,zone.modelData.w*canvas.width+translation.x)); zone.height = Math.max(25,Math.min(canvas.height-zone.y,zone.modelData.h*canvas.height+translation.y)) }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        LabelText { text: flowdeck.text("actual") + ": " + flowdeck.preview.canvas.width + " × " + flowdeck.preview.canvas.height + " px  ·  " + flowdeck.preview.unassigned.length + " unassigned" }
                        RowLayout {
                            Layout.fillWidth: true
                            ActionButton { text: flowdeck.text("addZone"); onClicked: flowdeck.addZone() }
                            ActionButton { text: flowdeck.text("delete"); enabled: root.zoneIndex >= 0; onClicked: { flowdeck.removeZone(root.zoneIndex); root.zoneIndex = -1 } }
                            Item { Layout.fillWidth: true }
                            ActionButton { text: "Import"; onClicked: flowdeck.importWorkspace() }
                            ActionButton { text: "Export"; onClicked: flowdeck.exportWorkspace() }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            LabelText { text: flowdeck.text("monitor") }
                            SelectBox {
                                id: monitorBox; Layout.preferredWidth: 190
                                model: ["Auto"].concat(flowdeck.monitors.map(function(m) { return m.name }))
                                currentIndex: Math.max(0,model.indexOf(flowdeck.selectedWorkspace.monitor))
                                onActivated: flowdeck.changeWorkspace("monitor", currentIndex === 0 ? "" : currentText)
                            }
                            LabelText { text: flowdeck.text("canvas") }
                            SelectBox {
                                model: ["native","16:9","21:9","custom"]
                                currentIndex: Math.max(0,model.indexOf(flowdeck.selectedWorkspace.canvasMode))
                                onActivated: flowdeck.changeWorkspace("canvasMode", currentText)
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            LabelText { text: "W" }
                            Field { Layout.preferredWidth: 80; text: flowdeck.selectedWorkspace.canvasWidth; validator: IntValidator { bottom: 1; top: 16000 }; onEditingFinished: flowdeck.changeWorkspace("canvasWidth", Number(text)) }
                            LabelText { text: "H" }
                            Field { Layout.preferredWidth: 80; text: flowdeck.selectedWorkspace.canvasHeight; validator: IntValidator { bottom: 1; top: 16000 }; onEditingFinished: flowdeck.changeWorkspace("canvasHeight", Number(text)) }
                            LabelText { text: flowdeck.text("gap") }
                            Field { Layout.preferredWidth: 60; text: flowdeck.selectedWorkspace.gap; validator: IntValidator { bottom: 0; top: 100 }; onEditingFinished: flowdeck.changeWorkspace("gap", Number(text)) }
                        }
                        RowLayout {
                            LabelText { text: flowdeck.text("hotkey") }
                            Field { Layout.preferredWidth: 160; text: flowdeck.selectedWorkspace.hotkey || ""; placeholderText: "Ctrl+Shift+T"; onEditingFinished: flowdeck.changeWorkspace("hotkey", text) }
                            CheckBox { text: flowdeck.text("directApply"); checked: flowdeck.selectedWorkspace.directApply || false; onToggled: flowdeck.changeWorkspace("directApply", checked) }
                        }
                        Rectangle {
                            visible: root.zoneIndex >= 0 && root.zoneIndex < (flowdeck.selectedWorkspace.zones || []).length
                            Layout.fillWidth: true; implicitHeight: zoneDetails.implicitHeight + 28; radius: 10; color: "#202d35"
                            ColumnLayout {
                                id: zoneDetails; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 14
                                LabelText { text: flowdeck.text("zones") + " · " + (root.zoneIndex+1) }
                                Field { Layout.fillWidth: true; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].label : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"label",text) }
                                SelectBox {
                                    Layout.fillWidth: true
                                    model: [flowdeck.text("assign")].concat(flowdeck.windows.map(function(w) { return w.title + "  ·  " + w.executable.split(/[\\/]/).pop() }))
                                    onActivated: if (currentIndex > 0) { var w = flowdeck.windows[currentIndex-1]; flowdeck.assignZone(root.zoneIndex,w.executable,w.windowClass,"") }
                                }
                                Field { Layout.fillWidth: true; placeholderText: "EXE path / suffix"; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].executable : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"executable",text) }
                                Field { Layout.fillWidth: true; placeholderText: "Window class"; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].windowClass : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"windowClass",text) }
                                Field { Layout.fillWidth: true; placeholderText: "Title pattern (regex)"; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].titlePattern : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"titlePattern",text) }
                            }
                        }
                        Repeater {
                            model: [true,false]
                            delegate: ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                LabelText { text: modelData ? flowdeck.text("actionsBefore") : flowdeck.text("actionsAfter") }
                                Repeater {
                                    model: modelData ? flowdeck.selectedWorkspace.before : flowdeck.selectedWorkspace.after
                                    delegate: RowLayout {
                                        required property int index
                                        required property var modelData
                                        Layout.fillWidth: true
                                        SelectBox { model: ["launch","powershell","wait","focus","minimize","plugin"]; currentIndex: Math.max(0,model.indexOf(modelData.type)); onActivated: flowdeck.editAction(parent.parent.modelData,index,"type",currentText) }
                                        Field { Layout.fillWidth: true; text: modelData.program; placeholderText: "EXE / plugin ID"; onEditingFinished: flowdeck.editAction(parent.parent.modelData,index,"program",text) }
                                        Field { Layout.fillWidth: true; text: modelData.arguments; placeholderText: "Arguments"; onEditingFinished: flowdeck.editAction(parent.parent.modelData,index,"arguments",text) }
                                        Field { Layout.fillWidth: true; text: modelData.script; placeholderText: "Script / title"; onEditingFinished: flowdeck.editAction(parent.parent.modelData,index,"script",text) }
                                        ActionButton { text: "×"; onClicked: flowdeck.removeAction(parent.parent.modelData,index) }
                                    }
                                }
                                ActionButton { text: "+"; onClicked: flowdeck.addAction(modelData) }
                            }
                        }
                        ActionButton { visible: !flowdeck.selectedWorkspace.trusted; text: flowdeck.text("trust"); onClicked: flowdeck.trustWorkspace() }
                    }
                }
            }
            ColumnLayout {
                visible: root.page === 2; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
                Text { text: flowdeck.text("pluginWarning"); color: "#edc985"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                ActionButton { text: "Open plugins folder"; onClicked: flowdeck.openPluginsFolder() }
                Text { text: "Python  ·  Lua 5.5.1"; color: "#b7c8cb" }
            }
            ColumnLayout {
                visible: root.page === 3; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
                LabelText { text: flowdeck.text("language") }
                SelectBox { model: ["🇷🇺 Русский", "🇬🇧 English"]; currentIndex: flowdeck.language === "en" ? 1 : 0; onActivated: flowdeck.setSetting("language", currentIndex === 1 ? "en" : "ru") }
                LabelText { text: flowdeck.text("accent") }
                Field { text: flowdeck.settings.accent; onEditingFinished: flowdeck.setSetting("accent", text) }
                LabelText { text: flowdeck.text("contrast") }
                SelectBox { model: ["normal","high"]; currentIndex: model.indexOf(flowdeck.settings.contrast); onActivated: flowdeck.setSetting("contrast", currentText) }
                LabelText { text: flowdeck.text("scale") }
                Slider { from: .8; to: 1.5; stepSize: .05; value: flowdeck.settings.scale || 1; onMoved: flowdeck.setSetting("scale", value) }
                LabelText { text: flowdeck.text("density") }
                SelectBox { model: ["comfortable","compact"]; currentIndex: model.indexOf(flowdeck.settings.density); onActivated: flowdeck.setSetting("density", currentText) }
                LabelText { text: flowdeck.text("channel") }
                SelectBox { model: ["preview","stable"]; currentIndex: model.indexOf(flowdeck.settings.channel); onActivated: flowdeck.setSetting("channel", currentText) }
                Item { Layout.fillHeight: true }
            }
            Text { text: flowdeck.status; color: "#e5ad7d"; Layout.fillWidth: true; elide: Text.ElideRight }
        }
    }
    Dialog {
        id: restoreDialog; title: flowdeck.text("restore"); modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No | Dialog.Cancel
        contentItem: Column { spacing: 12; Text { text: flowdeck.restorationSummary(); color: "#e5eeee" }; Text { text: flowdeck.text("launchMissing"); color: "#e5eeee" } }
        onAccepted: { flowdeck.restoreSession(true); root.restoreVisible = false }
        onRejected: { flowdeck.restoreSession(false); root.restoreVisible = false }
    }
}
