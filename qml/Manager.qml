import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1250; height: 820; minimumWidth: 950; minimumHeight: 660
    visible: true
    title: "FlowDeck"
    color: flowdeck.settings.contrast === "high" ? "#060a0d" : "#10151a"
    property color accent: flowdeck.settings.accent || "#63d8c7"
    property bool highContrast: flowdeck.settings.contrast === "high"
    property bool compact: flowdeck.settings.density === "compact"
    property int page: 0
    property int zoneIndex: -1
    property bool restoreVisible: flowdeck.restorationSummary() !== "0 matched, 0 missing"
    function placementTitle(zoneId) {
        var list = flowdeck.preview.placements
        for (var i=0;i<list.length;i++) if (list[i].zoneId === zoneId) return list[i].title
        return flowdeck.i18n.noWindow
    }
    font.family: "Segoe UI"
    font.pixelSize: 14 * (flowdeck.settings.scale || 1)

    component ActionButton: Button {
        id: button
        property bool primary: false
        implicitHeight: root.compact ? 31 : 36
        background: Rectangle {
            radius: 9
            color: button.primary ? root.accent : (button.hovered ? "#303b44" : root.highContrast ? "#111b20" : "#263039")
            border.color: button.primary ? root.accent : root.highContrast ? "#e7f5f2" : "#40505b"
        }
        contentItem: Text {
            text: button.text; color: button.primary ? "#10201f" : "#e6eef0"
            font: button.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
    }
    component Field: TextField {
        color: "#e7eff0"; selectionColor: root.accent; selectedTextColor: "#10151a"
        background: Rectangle { radius: 7; color: root.highContrast ? "#0c151a" : "#202a32"; border.color: parent.activeFocus ? root.accent : root.highContrast ? "#e7f5f2" : "#3a4a55" }
        implicitHeight: root.compact ? 31 : 36
    }
    component LabelText: Text { color: "#a9b8be"; font.pixelSize: 12 * (flowdeck.settings.scale || 1) }
    component SelectBox: ComboBox {
        id: box
        implicitHeight: root.compact ? 31 : 36
        contentItem: Text { text: box.displayText; color: "#e7eff0"; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
        background: Rectangle { radius: 7; color: root.highContrast ? "#0c151a" : "#202a32"; border.color: root.highContrast ? "#e7f5f2" : "#3a4a55" }
        delegate: ItemDelegate { width: box.width; text: modelData; highlighted: box.highlightedIndex === index }
    }

    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 220; Layout.fillHeight: true; color: root.highContrast ? "#0a1013" : "#151d23"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 20; spacing: 8
                Text { text: "◧  FlowDeck"; color: root.accent; font.pixelSize: 22; font.bold: true; Layout.bottomMargin: 20 }
                Repeater {
                    model: [flowdeck.i18n.spaces, flowdeck.i18n.editor, flowdeck.i18n.plugins, flowdeck.i18n.settings]
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true; height: root.compact ? 36 : 44; radius: 9
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
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: root.compact ? 16 : 24; spacing: root.compact ? 10 : 18
            RowLayout {
                Layout.fillWidth: true
                Text { text: [flowdeck.i18n.spaces,flowdeck.i18n.editor,flowdeck.i18n.plugins,flowdeck.i18n.settings][root.page]; color: "#f1f7f6"; font.pixelSize: 26; font.bold: true }
                Item { Layout.fillWidth: true }
                ActionButton { text: flowdeck.i18n.undo; onClicked: flowdeck.undo() }
                ActionButton { text: "⟳"; onClicked: flowdeck.refresh() }
            }
            Rectangle {
                visible: updater.ready
                Layout.fillWidth: true; implicitHeight: 54; radius: 10; color: "#26453f"; border.color: root.accent
                RowLayout {
                    anchors.fill: parent; anchors.margins: 9
                    Text { text: updater.status; color: "#e6f5f1" }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: flowdeck.i18n.installUpdate; primary: true; onClicked: updateDialog.open() }
                }
            }
            Rectangle {
                visible: root.restoreVisible && root.page === 0
                Layout.fillWidth: true; implicitHeight: 75; radius: 10; color: "#2c352e"; border.color: "#738553"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 12
                    ColumnLayout {
                        Text { text: flowdeck.i18n.restoreHint; color: "#eaf1dc" }
                        LabelText { text: flowdeck.restorationSummary() }
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: flowdeck.i18n.restore; onClicked: restoreDialog.open() }
                    ActionButton { text: "×"; onClicked: { root.restoreVisible = false; flowdeck.dismissRestoration() } }
                }
            }
            RowLayout {
                visible: root.page < 2
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: root.compact ? 10 : 18
                Rectangle {
                    Layout.preferredWidth: 228; Layout.fillHeight: true; radius: 12; color: root.highContrast ? "#0c161a" : "#1a242b"
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 12; spacing: 10
                        RowLayout {
                            LabelText { text: flowdeck.i18n.spaces }
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
                            ActionButton { text: flowdeck.i18n.duplicate; onClicked: flowdeck.duplicateWorkspace() }
                            ActionButton { text: flowdeck.i18n.delete; onClicked: flowdeck.deleteWorkspace() }
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
                            ActionButton { text: flowdeck.i18n.preview; onClicked: flowdeck.refresh() }
                            ActionButton { text: flowdeck.selectedWorkspace.before.length > 0 && !flowdeck.prepared ? (flowdeck.language === "ru" ? "Выполнить шаги и обновить план" : "Run steps and refresh plan") : flowdeck.i18n.apply; primary: true; enabled: flowdeck.preview.placements.length > 0 || (flowdeck.selectedWorkspace.before.length > 0 && !flowdeck.prepared); onClicked: flowdeck.applySelected() }
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
                                        radius: 6; color: root.page === 0 ? "#263841" : ["#365c65","#534a74","#65533d","#3b6250"][index % 4]
                                        border.color: root.zoneIndex === index ? "#ffffff" : "#82a4a9"; border.width: root.zoneIndex === index ? 2 : 1
                                        Text { anchors.centerIn: parent; width: parent.width - 10; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: "#f3f8f9"; font.bold: true; text: root.page === 0 ? zone.modelData.label : zone.modelData.label + "\n" + root.placementTitle(zone.modelData.id) }
                                        DragHandler {
                                            target: null
                                            onActiveChanged: if (!active) flowdeck.moveZone(zone.index, zone.x/canvas.width, zone.y/canvas.height, zone.width/canvas.width, zone.height/canvas.height)
                                            onTranslationChanged: { zone.x = Math.max(0,Math.min(canvas.width-zone.width,zone.modelData.x*canvas.width+translation.x)); zone.y = Math.max(0,Math.min(canvas.height-zone.height,zone.modelData.y*canvas.height+translation.y)) }
                                        }
                                        TapHandler { onTapped: root.zoneIndex = zone.index }
                                        Rectangle {
                                            visible: root.page === 1
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
                                Repeater {
                                    model: root.page === 0 ? flowdeck.preview.placements : []
                                    delegate: Rectangle {
                                        required property int index
                                        required property var modelData
                                        x: (modelData.target.x - flowdeck.preview.canvas.x) / Math.max(1,flowdeck.preview.canvas.width) * canvas.width
                                        y: (modelData.target.y - flowdeck.preview.canvas.y) / Math.max(1,flowdeck.preview.canvas.height) * canvas.height
                                        width: modelData.target.width / Math.max(1,flowdeck.preview.canvas.width) * canvas.width
                                        height: modelData.target.height / Math.max(1,flowdeck.preview.canvas.height) * canvas.height
                                        radius: 6; color: ["#3c6b74","#695d8c","#816a4e","#48775d"][index % 4]
                                        border.color: "#c9e1df"; border.width: 1
                                        Text { anchors.centerIn: parent; width: parent.width-10; text: modelData.title; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: "#ffffff"; font.bold: true }
                                    }
                                }
                            }
                        }
                        LabelText { text: flowdeck.i18n.actual + ": " + flowdeck.preview.canvas.width + " × " + flowdeck.preview.canvas.height + " px  ·  " + flowdeck.preview.unassigned.length + " " + flowdeck.i18n.unassigned }
                        LabelText { visible: flowdeck.preview.monitorMissing; text: flowdeck.language === "ru" ? "Выбранный монитор не найден — предпросмотр на основном" : "Selected monitor is missing — preview uses the first display"; color: "#edb77f" }
                        RowLayout {
                            Layout.fillWidth: true
                            ActionButton { text: flowdeck.i18n.addZone; onClicked: flowdeck.addZone() }
                            ActionButton { text: flowdeck.i18n.delete; enabled: root.zoneIndex >= 0; onClicked: { flowdeck.removeZone(root.zoneIndex); root.zoneIndex = -1 } }
                            Item { Layout.fillWidth: true }
                            ActionButton { text: flowdeck.i18n.import; onClicked: flowdeck.importWorkspace() }
                            ActionButton { text: flowdeck.i18n.export; onClicked: flowdeck.exportWorkspace() }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            LabelText { text: flowdeck.i18n.monitor }
                            SelectBox {
                                id: monitorBox; Layout.preferredWidth: 190
                                model: [flowdeck.i18n.auto].concat(flowdeck.monitors.map(function(m) { return m.name }))
                                currentIndex: Math.max(0,model.indexOf(flowdeck.selectedWorkspace.monitor))
                                onActivated: flowdeck.changeWorkspace("monitor", currentIndex === 0 ? "" : currentText)
                            }
                            LabelText { text: flowdeck.i18n.canvas }
                            SelectBox {
                                model: ["native","16:9","21:9","custom"]
                                currentIndex: Math.max(0,model.indexOf(flowdeck.selectedWorkspace.canvasMode))
                                onActivated: flowdeck.changeWorkspace("canvasMode", currentText)
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            LabelText { text: "W" }
                            Field {
                                Layout.preferredWidth: 80
                                text: flowdeck.selectedWorkspace.canvasWidth
                                validator: IntValidator { bottom: 1; top: 16000 }
                                onEditingFinished: flowdeck.changeWorkspace("canvasWidth", Number(text))
                            }
                            LabelText { text: "H" }
                            Field {
                                Layout.preferredWidth: 80
                                text: flowdeck.selectedWorkspace.canvasHeight
                                validator: IntValidator { bottom: 1; top: 16000 }
                                onEditingFinished: flowdeck.changeWorkspace("canvasHeight", Number(text))
                            }
                            LabelText { text: flowdeck.i18n.gap }
                            Field {
                                Layout.preferredWidth: 60
                                text: flowdeck.selectedWorkspace.gap
                                validator: IntValidator { bottom: 0; top: 100 }
                                onEditingFinished: flowdeck.changeWorkspace("gap", Number(text))
                            }
                        }
                        RowLayout {
                            LabelText { text: flowdeck.i18n.hotkey }
                            Field { Layout.preferredWidth: 160; text: flowdeck.selectedWorkspace.hotkey || ""; placeholderText: "Ctrl+Shift+T"; onEditingFinished: flowdeck.changeWorkspace("hotkey", text) }
                            CheckBox { text: flowdeck.i18n.directApply; checked: flowdeck.selectedWorkspace.directApply || false; onToggled: flowdeck.changeWorkspace("directApply", checked) }
                        }
                        Rectangle {
                            visible: root.zoneIndex >= 0 && root.zoneIndex < (flowdeck.selectedWorkspace.zones || []).length
                            Layout.fillWidth: true; implicitHeight: zoneDetails.implicitHeight + 28; radius: 10; color: "#202d35"
                            ColumnLayout {
                                id: zoneDetails; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 14
                                LabelText { text: flowdeck.i18n.zones + " · " + (root.zoneIndex+1) }
                                Field { Layout.fillWidth: true; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].label : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"label",text) }
                                SelectBox {
                                    Layout.fillWidth: true
                                    model: [flowdeck.i18n.assign].concat(flowdeck.windows.map(function(w) { return w.title + "  ·  " + w.executable.split(/[\\/]/).pop() }))
                                    onActivated: if (currentIndex > 0) { var w = flowdeck.windows[currentIndex-1]; flowdeck.assignZone(root.zoneIndex,w.executable,w.windowClass,w.title) }
                                }
                                Field { Layout.fillWidth: true; placeholderText: flowdeck.i18n.exe; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].executable : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"executable",text) }
                                Field { Layout.fillWidth: true; placeholderText: flowdeck.i18n.windowClass; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].windowClass : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"windowClass",text) }
                                Field { Layout.fillWidth: true; placeholderText: flowdeck.i18n.titlePattern; text: root.zoneIndex >= 0 ? flowdeck.selectedWorkspace.zones[root.zoneIndex].titlePattern : ""; onEditingFinished: flowdeck.editZone(root.zoneIndex,"titlePattern",text) }
                                RowLayout {
                                    LabelText { text: flowdeck.i18n.aspectRatio }
                                    SelectBox { model: [flowdeck.i18n.free,"16:9","21:9"]; currentIndex: root.zoneIndex < 0 ? 0 : Math.abs(flowdeck.selectedWorkspace.zones[root.zoneIndex].aspectRatio-16/9) < .01 ? 1 : Math.abs(flowdeck.selectedWorkspace.zones[root.zoneIndex].aspectRatio-21/9) < .01 ? 2 : 0; onActivated: flowdeck.editZone(root.zoneIndex,"aspectRatio",currentIndex === 1 ? 16/9 : currentIndex === 2 ? 21/9 : 0) }
                                }
                            }
                        }
                        Repeater {
                            model: [true,false]
                            delegate: ColumnLayout {
                                id: actionSection
                                required property var modelData
                                property bool beforeActions: modelData
                                Layout.fillWidth: true
                                LabelText { text: modelData ? flowdeck.i18n.actionsBefore : flowdeck.i18n.actionsAfter }
                                Repeater {
                                    model: modelData ? flowdeck.selectedWorkspace.before : flowdeck.selectedWorkspace.after
                                    delegate: RowLayout {
                                        required property int index
                                        required property var modelData
                                        Layout.fillWidth: true
                                        SelectBox { model: ["launch","powershell","wait","focus","minimize","plugin"]; currentIndex: Math.max(0,model.indexOf(modelData.type)); onActivated: flowdeck.editAction(actionSection.beforeActions,index,"type",currentText) }
                                        Field { Layout.fillWidth: true; text: modelData.program; placeholderText: "EXE / plugin ID"; onEditingFinished: flowdeck.editAction(actionSection.beforeActions,index,"program",text) }
                                        Field { Layout.fillWidth: true; text: modelData.arguments; placeholderText: flowdeck.i18n.arguments; onEditingFinished: flowdeck.editAction(actionSection.beforeActions,index,"arguments",text) }
                                        Field { Layout.fillWidth: true; text: modelData.script; placeholderText: flowdeck.i18n.script; onEditingFinished: flowdeck.editAction(actionSection.beforeActions,index,"script",text) }
                                        Field { Layout.preferredWidth: 72; text: modelData.timeoutMs; placeholderText: "ms"; onEditingFinished: flowdeck.editAction(actionSection.beforeActions,index,"timeoutMs",Number(text)) }
                                        ActionButton { text: "×"; onClicked: flowdeck.removeAction(actionSection.beforeActions,index) }
                                    }
                                }
                                ActionButton { text: "+"; onClicked: flowdeck.addAction(modelData) }
                            }
                        }
                        ActionButton { visible: !flowdeck.selectedWorkspace.trusted; text: flowdeck.i18n.trust; onClicked: flowdeck.trustWorkspace() }
                    }
                }
            }
            ColumnLayout {
                visible: root.page === 2; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
                Text { text: flowdeck.i18n.pluginWarning; color: "#edc985"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                ActionButton { text: flowdeck.language === "ru" ? "Установить папку плагина" : "Install plugin folder"; primary: true; onClicked: flowdeck.installPlugin() }
                ActionButton { text: flowdeck.i18n.openPlugins; onClicked: flowdeck.openPluginsFolder() }
                Repeater {
                    model: flowdeck.plugins
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true; implicitHeight: 78; radius: 10; color: "#202e36"
                        Column {
                            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; anchors.leftMargin: 16; spacing: 4
                            Text { text: modelData.name + "  ·  " + modelData.version + "  ·  " + modelData.language; color: "#f1f7f6"; font.bold: true }
                            Text { text: modelData.description; color: "#a9b8be" }
                        }
                    }
                }
            }
            ColumnLayout {
                visible: root.page === 3; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
                LabelText { text: flowdeck.i18n.language }
                SelectBox { model: ["🇷🇺 Русский", "🇬🇧 English"]; currentIndex: flowdeck.language === "en" ? 1 : 0; onActivated: flowdeck.setSetting("language", currentIndex === 1 ? "en" : "ru") }
                LabelText { text: flowdeck.i18n.hotkey + " · Palette" }
                Field { text: flowdeck.settings.paletteHotkey || "Ctrl+Alt+Space"; onEditingFinished: flowdeck.setSetting("paletteHotkey",text) }
                LabelText { text: flowdeck.i18n.accent }
                Field { text: flowdeck.settings.accent; onEditingFinished: flowdeck.setSetting("accent", text) }
                LabelText { text: flowdeck.i18n.contrast }
                SelectBox { model: ["normal","high"]; currentIndex: model.indexOf(flowdeck.settings.contrast); onActivated: flowdeck.setSetting("contrast", currentText) }
                LabelText { text: flowdeck.i18n.scale }
                Slider { from: .8; to: 1.5; stepSize: .05; value: flowdeck.settings.scale || 1; onMoved: flowdeck.setSetting("scale", value) }
                LabelText { text: flowdeck.i18n.density }
                SelectBox { model: ["comfortable","compact"]; currentIndex: model.indexOf(flowdeck.settings.density); onActivated: flowdeck.setSetting("density", currentText) }
                LabelText { text: flowdeck.i18n.channel }
                SelectBox { model: ["preview","stable"]; currentIndex: model.indexOf(flowdeck.settings.channel); onActivated: flowdeck.setSetting("channel", currentText) }
                ActionButton { text: flowdeck.i18n.checkUpdates; onClicked: updater.check(flowdeck.settings.channel) }
                LabelText { text: updater.status }
                Item { Layout.fillHeight: true }
            }
            Text { text: flowdeck.status; color: "#e5ad7d"; Layout.fillWidth: true; elide: Text.ElideRight }
        }
    }
    Dialog {
        id: restoreDialog; title: flowdeck.i18n.restore; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Column { width: 520; spacing: 12
            Text { text: flowdeck.restorationSummary(); color: "#e5eeee" }
            ListView {
                width: parent.width; height: Math.min(280,contentHeight); clip: true
                model: flowdeck.restorationDiff()
                delegate: Column {
                    required property var modelData
                    width: ListView.view.width; spacing: 2
                    Text { text: modelData.title; color: "#e5eeee"; elide: Text.ElideRight; width: parent.width }
                    Text { text: modelData.missing ? "Closed  →  " + modelData.saved : modelData.current + "  →  " + modelData.saved; color: modelData.missing ? "#e3ad72" : "#91c5b8"; font.pixelSize: 11 }
                }
            }
            CheckBox { id: launchMissingCheck; text: flowdeck.i18n.launchMissing; checked: false }
        }
        onAccepted: { flowdeck.restoreSession(launchMissingCheck.checked); root.restoreVisible = false }
        onRejected: { flowdeck.dismissRestoration(); root.restoreVisible = false }
    }
    Dialog {
        id: updateDialog; modal: true; anchors.centerIn: parent
        title: "Install " + updater.availableVersion + "?"
        standardButtons: Dialog.Yes | Dialog.Cancel
        onAccepted: updater.install()
    }
}
