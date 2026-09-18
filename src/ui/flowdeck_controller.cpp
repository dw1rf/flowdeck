#include "ui/flowdeck_controller.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>

#include "core/palette.hpp"

namespace flowdeck {
namespace {
QVariantMap rectMap(const QRect& r) {
    return {{"x", r.x()}, {"y", r.y()}, {"width", r.width()}, {"height", r.height()}};
}
QVariantMap zoneMap(const Zone& z) {
    return {{"id", z.id}, {"label", z.label}, {"x", z.bounds.x()},
            {"y", z.bounds.y()}, {"w", z.bounds.width()},
            {"h", z.bounds.height()}, {"executable", z.executable},
            {"windowClass", z.windowClass}, {"titlePattern", z.titlePattern}};
}
QVariantMap actionMap(const ActionStep& a) {
    return {{"type", a.type}, {"program", a.program},
            {"arguments", a.arguments.join(' ')}, {"script", a.script},
            {"timeoutMs", a.timeoutMs}};
}
}

FlowDeckController::FlowDeckController(QObject* parent) : QObject(parent) {
    connect(&store_, &WorkspaceStore::settingsChanged, this, &FlowDeckController::settingsChanged);
    currentPlan_ = WorkspaceEngine::plan(store_.workspaces().first());
}
QVariantList FlowDeckController::workspaces() const {
    QVariantList result;
    for (const auto& w : store_.workspaces())
        result.append(QVariantMap{{"id", w.id}, {"name", w.name}, {"hotkey", w.hotkey},
                                  {"directApply", w.directApply}});
    return result;
}
QVariantList FlowDeckController::windows() const {
    QVariantList result;
    for (const auto& w : WorkspaceEngine::windows())
        result.append(QVariantMap{{"title", w.title}, {"executable", w.executable},
                                  {"windowClass", w.windowClass}, {"monitor", w.monitor},
                                  {"rect", rectMap(w.rect)}});
    return result;
}
QVariantList FlowDeckController::monitors() const {
    QVariantList result;
    for (const auto& m : WorkspaceEngine::monitors())
        result.append(QVariantMap{{"name", m.first}, {"rect", rectMap(m.second)}});
    return result;
}
QVariantMap FlowDeckController::selectedWorkspace() const {
    if (selected_ < 0 || selected_ >= store_.workspaces().size()) return {};
    const auto& w = store_.workspaces()[selected_];
    QVariantList zones, before, after;
    for (const auto& z : w.zones) zones.append(zoneMap(z));
    for (const auto& a : w.before) before.append(actionMap(a));
    for (const auto& a : w.after) after.append(actionMap(a));
    return {{"id", w.id}, {"name", w.name}, {"monitor", w.monitor},
            {"canvasMode", w.canvasMode}, {"canvasWidth", w.canvasWidth},
            {"canvasHeight", w.canvasHeight}, {"gap", w.gap},
            {"hotkey", w.hotkey}, {"directApply", w.directApply},
            {"trusted", w.trusted}, {"zones", zones}, {"before", before},
            {"after", after}};
}
QVariantMap FlowDeckController::preview() const {
    QVariantList placements;
    for (const auto& p : currentPlan_.placements)
        placements.append(QVariantMap{{"zoneId", p.zoneId}, {"title", p.window.title},
                                      {"target", rectMap(p.target)}});
    return {{"canvas", rectMap(currentPlan_.canvas)},
            {"monitorArea", rectMap(currentPlan_.monitorArea)},
            {"placements", placements}, {"unassigned", currentPlan_.unassignedZones},
            {"fingerprint", currentPlan_.fingerprint}};
}
QVariantMap FlowDeckController::settings() const { return store_.settings().toVariantMap(); }
QString FlowDeckController::language() const { return store_.settings().value("language").toString("ru"); }
QVariantList FlowDeckController::commands() const {
    QVariantList result;
    for (const auto& w : store_.workspaces())
        result.append(QVariantMap{{"id", "workspace:" + w.id}, {"title", w.name},
                                  {"hint", text("workspace")}});
    for (const auto& command : Palette::Instance().All())
        result.append(QVariantMap{{"id", QString::fromStdString(command.id)},
                                  {"title", QString::fromStdString(command.title)},
                                  {"hint", QString::fromStdString(command.hint)}});
    result.append(QVariantMap{{"id", "core:undo"}, {"title", text("undo")}, {"hint", "FlowDeck"}});
    result.append(QVariantMap{{"id", "core:settings"}, {"title", text("settings")}, {"hint", "FlowDeck"}});
    return result;
}
QString FlowDeckController::text(const QString& key) const {
    static const QMap<QString, QPair<QString, QString>> strings = {
        {"spaces", {"Пространства", "Spaces"}}, {"editor", {"Редактор", "Editor"}},
        {"plugins", {"Плагины", "Plugins"}}, {"settings", {"Настройки", "Settings"}},
        {"workspace", {"Пространство", "Workspace"}}, {"preview", {"Предпросмотр", "Preview"}},
        {"apply", {"Применить", "Apply"}}, {"undo", {"Отменить", "Undo"}},
        {"new", {"Создать", "Create"}}, {"duplicate", {"Копия", "Duplicate"}},
        {"delete", {"Удалить", "Delete"}}, {"zones", {"Зоны", "Zones"}},
        {"addZone", {"Добавить зону", "Add zone"}}, {"assign", {"Назначить окно", "Assign window"}},
        {"monitor", {"Монитор", "Monitor"}}, {"canvas", {"Виртуальная область", "Virtual canvas"}},
        {"actual", {"Фактический размер", "Actual size"}}, {"gap", {"Отступы", "Gaps"}},
        {"hotkey", {"Горячая клавиша", "Hotkey"}},
        {"directApply", {"Применять сразу", "Apply immediately"}},
        {"actionsBefore", {"Перед раскладкой", "Before layout"}},
        {"actionsAfter", {"После раскладки", "After layout"}},
        {"language", {"Язык", "Language"}}, {"accent", {"Акцент", "Accent"}},
        {"contrast", {"Контраст", "Contrast"}}, {"scale", {"Масштаб", "Scale"}},
        {"density", {"Плотность", "Density"}}, {"channel", {"Канал обновлений", "Update channel"}},
        {"restore", {"Восстановить окна", "Restore windows"}},
        {"restoreHint", {"Найден прошлый сеанс. Сравните окна перед восстановлением.",
                          "A previous session was found. Compare windows before restoring."}},
        {"launchMissing", {"Запустить закрытые приложения?", "Launch closed applications?"}},
        {"trust", {"Я проверил действия и доверяю профилю", "I reviewed and trust these actions"}},
        {"pluginWarning", {"Плагины Python и Lua выполняют доверенный локальный код с доступом к системе.",
                            "Python and Lua plugins run trusted local code with system access."}},
        {"search", {"Поиск команд и пространств", "Search commands and spaces"}},
        {"noWindow", {"Окно не назначено", "No window assigned"}},
    };
    const auto it = strings.find(key);
    return it == strings.end() ? key : (language() == "en" ? it.value().second : it.value().first);
}
void FlowDeckController::setStatus(const QString& value) { status_ = value; emit statusChanged(); }
void FlowDeckController::saveCurrent(const Workspace& w) {
    QString error;
    if (!store_.saveWorkspace(selected_, w, &error)) setStatus(error);
    currentPlan_ = WorkspaceEngine::plan(w);
    emit workspacesChanged(); emit selectedChanged(); emit previewChanged(); emit commandsChanged(); emit hotkeysChanged();
}
void FlowDeckController::selectWorkspace(int i) {
    if (i < 0 || i >= store_.workspaces().size()) return;
    selected_ = i; currentPlan_ = WorkspaceEngine::plan(store_.workspaces()[i]);
    emit selectedChanged(); emit previewChanged();
}
void FlowDeckController::createWorkspace() {
    Workspace w; w.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    w.name = language() == "en" ? "New workspace" : "Новое пространство";
    w.zones.append({"zone1", language() == "en" ? "Zone 1" : "Зона 1", QRectF(0,0,1,1), {},{}, {}});
    selectWorkspace(store_.addWorkspace(w)); emit workspacesChanged(); emit commandsChanged();
}
void FlowDeckController::duplicateWorkspace() {
    if (store_.workspaces().isEmpty()) return;
    auto w = store_.workspaces()[selected_]; w.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    w.name += " copy"; w.hotkey.clear(); selectWorkspace(store_.addWorkspace(w));
    emit workspacesChanged(); emit commandsChanged();
}
void FlowDeckController::deleteWorkspace() {
    if (store_.workspaces().size() <= 1) return;
    store_.removeWorkspace(selected_); selectWorkspace(qMin(selected_, store_.workspaces().size()-1));
    emit workspacesChanged(); emit commandsChanged(); emit hotkeysChanged();
}
void FlowDeckController::changeWorkspace(const QString& field, const QVariant& value) {
    auto w = store_.workspaces()[selected_];
    if (field == "name") w.name = value.toString();
    else if (field == "monitor") w.monitor = value.toString();
    else if (field == "canvasMode") w.canvasMode = value.toString();
    else if (field == "canvasWidth") w.canvasWidth = qBound(1, value.toInt(), 16000);
    else if (field == "canvasHeight") w.canvasHeight = qBound(1, value.toInt(), 16000);
    else if (field == "gap") w.gap = qBound(0, value.toInt(), 100);
    else if (field == "hotkey") w.hotkey = value.toString();
    else if (field == "directApply") w.directApply = value.toBool();
    saveCurrent(w);
}
void FlowDeckController::addZone() {
    auto w = store_.workspaces()[selected_];
    w.zones.append({QUuid::createUuid().toString(QUuid::WithoutBraces), "Zone " + QString::number(w.zones.size()+1),
                    QRectF(.1,.1,.4,.4), {},{}, {}}); saveCurrent(w);
}
void FlowDeckController::removeZone(int i) {
    auto w = store_.workspaces()[selected_]; if (i < 0 || i >= w.zones.size()) return;
    w.zones.removeAt(i); saveCurrent(w);
}
void FlowDeckController::editZone(int i, const QString& field, const QVariant& value) {
    auto w = store_.workspaces()[selected_]; if (i < 0 || i >= w.zones.size()) return;
    auto& z = w.zones[i];
    if (field == "label") z.label = value.toString();
    else if (field == "executable") z.executable = value.toString();
    else if (field == "windowClass") z.windowClass = value.toString();
    else if (field == "titlePattern") z.titlePattern = value.toString();
    saveCurrent(w);
}
void FlowDeckController::moveZone(int i, double x, double y, double width, double height) {
    auto w = store_.workspaces()[selected_]; if (i < 0 || i >= w.zones.size()) return;
    width = qBound(.05, width, 1.0); height = qBound(.05, height, 1.0);
    w.zones[i].bounds = QRectF(qBound(0.0,x,1.0-width), qBound(0.0,y,1.0-height),width,height);
    saveCurrent(w);
}
void FlowDeckController::assignZone(int i, const QString& exe, const QString& cls, const QString& title) {
    auto w = store_.workspaces()[selected_]; if (i < 0 || i >= w.zones.size()) return;
    w.zones[i].executable = exe; w.zones[i].windowClass = cls; w.zones[i].titlePattern = title; saveCurrent(w);
}
void FlowDeckController::addAction(bool before) {
    auto w = store_.workspaces()[selected_]; (before ? w.before : w.after).append({"launch", {},{}, {},10000}); saveCurrent(w);
}
void FlowDeckController::editAction(bool before, int i, const QString& field, const QVariant& value) {
    auto w = store_.workspaces()[selected_]; auto& actions = before ? w.before : w.after;
    if (i < 0 || i >= actions.size()) return;
    auto& a = actions[i];
    if (field == "type") a.type = value.toString();
    else if (field == "program") a.program = value.toString();
    else if (field == "arguments") a.arguments = value.toString().split(' ', Qt::SkipEmptyParts);
    else if (field == "script") a.script = value.toString();
    else if (field == "timeoutMs") a.timeoutMs = qBound(100,value.toInt(),120000);
    saveCurrent(w);
}
void FlowDeckController::removeAction(bool before, int i) {
    auto w = store_.workspaces()[selected_]; auto& actions = before ? w.before : w.after;
    if (i < 0 || i >= actions.size()) return; actions.removeAt(i); saveCurrent(w);
}
void FlowDeckController::trustWorkspace() { auto w = store_.workspaces()[selected_]; w.trusted = true; saveCurrent(w); }
void FlowDeckController::importWorkspace() {
    const auto path = QFileDialog::getOpenFileName(nullptr, text("workspace"), {}, "JSON (*.json)");
    if (path.isEmpty()) return; QFile file(path); if (!file.open(QIODevice::ReadOnly)) return;
    const auto document = QJsonDocument::fromJson(file.readAll()); if (!document.isObject()) return;
    auto w = workspaceFromJson(document.object()); w.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    w.trusted = false; selectWorkspace(store_.addWorkspace(w)); emit workspacesChanged(); emit commandsChanged();
}
void FlowDeckController::exportWorkspace() {
    const auto path = QFileDialog::getSaveFileName(nullptr, text("workspace"), {}, "JSON (*.json)");
    if (path.isEmpty()) return; QFile file(path); if (!file.open(QIODevice::WriteOnly)) return;
    file.write(QJsonDocument(toJson(store_.workspaces()[selected_])).toJson());
}
void FlowDeckController::applySelected() {
    auto& w = store_.workspaces()[selected_]; QString error;
    if (!WorkspaceEngine::apply(currentPlan_,w,&error)) {
        currentPlan_ = WorkspaceEngine::plan(w); emit previewChanged(); setStatus(error); return;
    }
    setStatus(language()=="en" ? "Workspace applied" : "Пространство применено"); refresh();
}
void FlowDeckController::undo() { QString error; WorkspaceEngine::undo(&error); setStatus(error); refresh(); }
void FlowDeckController::refresh() {
    if (store_.workspaces().isEmpty()) return;
    currentPlan_ = WorkspaceEngine::plan(store_.workspaces()[selected_]); emit windowsChanged(); emit previewChanged();
}
void FlowDeckController::setSetting(const QString& key, const QVariant& value) {
    store_.setSetting(key, QJsonValue::fromVariant(value)); emit settingsChanged(); emit commandsChanged();
}
void FlowDeckController::restoreSession(bool launchMissing) {
    setStatus(WorkspaceEngine::restore(store_.lastSession(), launchMissing, false)); refresh();
}
QString FlowDeckController::restorationSummary() const {
    return WorkspaceEngine::restore(store_.lastSession(), false, true);
}
void FlowDeckController::runCommand(const QString& id) {
    if (id.startsWith("workspace:")) {
        for (int i=0;i<store_.workspaces().size();++i) if (store_.workspaces()[i].id==id.mid(10)) {
            selectWorkspace(i); if (store_.workspaces()[i].directApply) applySelected(); else emit requestManager(); return;
        }
    }
    if (id=="core:undo") { undo(); return; }
    if (id=="core:settings") { emit requestManager(); return; }
    Palette::Instance().ExecuteById(id.toStdString());
}
void FlowDeckController::openPluginsFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath()+"/plugins"));
}
} // namespace flowdeck
