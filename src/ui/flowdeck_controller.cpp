#include "ui/flowdeck_controller.hpp"

#include <QDesktopServices>
#include <QCoreApplication>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>
#include <algorithm>

#include "core/palette.hpp"

namespace flowdeck {
namespace {
QString displayName(const Workspace& w, const QString& language) {
    if (language != "ru") return w.name;
    if (w.id == "coding" && w.name == "Coding") return "Кодинг";
    if (w.id == "trading" && w.name == "Trading") return "Трейдинг";
    if (w.id == "chill" && w.name == "Chill") return "Чилл";
    if (w.id == "grid" && w.name == "Grid") return "Сетка";
    return w.name;
}
QVariantMap rectMap(const QRect& r) {
    return {{"x", r.x()}, {"y", r.y()}, {"width", r.width()}, {"height", r.height()}};
}
QVariantMap zoneMap(const Zone& z, const QString& language) {
    QString label = z.label;
    if (language == "ru") {
        static const QMap<QString,QString> names = {{"Browser","Браузер"},{"Chart 1","График 1"},
            {"Chart 2","График 2"},{"Chart 3","График 3"},{"Terminal","Терминал"},
            {"Video 16:9","Видео 16:9"},{"Top left","Слева сверху"},
            {"Top right","Справа сверху"},{"Bottom left","Слева снизу"},
            {"Bottom right","Справа снизу"}};
        label = names.value(label,label);
    }
    return {{"id", z.id}, {"label", label}, {"x", z.bounds.x()},
            {"y", z.bounds.y()}, {"w", z.bounds.width()},
            {"h", z.bounds.height()}, {"aspectRatio", z.aspectRatio}, {"executable", z.executable},
            {"windowClass", z.windowClass}, {"titlePattern", z.titlePattern}};
}
QVariantMap actionMap(const ActionStep& a) {
    QStringList argumentText;
    for (const auto& argument : a.arguments)
        argumentText.append(argument.contains(' ') ? "\"" + argument + "\"" : argument);
    return {{"type", a.type}, {"program", a.program},
            {"arguments", argumentText.join(' ')}, {"script", a.script},
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
        result.append(QVariantMap{{"id", w.id}, {"name", displayName(w,language())}, {"hotkey", w.hotkey},
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
    for (const auto& z : w.zones) zones.append(zoneMap(z,language()));
    for (const auto& a : w.before) before.append(actionMap(a));
    for (const auto& a : w.after) after.append(actionMap(a));
    return {{"id", w.id}, {"name", displayName(w,language())}, {"monitor", w.monitor},
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
            {"monitorMissing", currentPlan_.monitorMissing},
            {"placements", placements}, {"unassigned", currentPlan_.unassignedZones},
            {"fingerprint", currentPlan_.fingerprint}};
}
QVariantMap FlowDeckController::settings() const { return store_.settings().toVariantMap(); }
QString FlowDeckController::language() const { return store_.settings().value("language").toString("ru"); }
QVariantMap FlowDeckController::i18n() const {
    QVariantMap result;
    for (const auto& key : {"spaces","editor","plugins","settings","workspace","preview","apply","undo",
                            "new","duplicate","delete","zones","addZone","assign","monitor","canvas","actual",
                            "gap","hotkey","directApply","actionsBefore","actionsAfter","language","accent",
                            "contrast","scale","density","channel","restore","restoreHint","launchMissing",
                            "trust","pluginWarning","search","noWindow","import","export","checkUpdates",
                            "installUpdate","openPlugins","aspectRatio","free","auto","unassigned","arguments",
                            "script","exe","windowClass","titlePattern"})
        result.insert(QString::fromLatin1(key), text(QString::fromLatin1(key)));
    return result;
}
QVariantList FlowDeckController::commands() const {
    QVariantList result;
    for (const auto& w : store_.workspaces())
        result.append(QVariantMap{{"id", "workspace:" + w.id}, {"title", displayName(w,language())},
                                  {"hint", text("workspace")}});
    for (const auto& command : Palette::Instance().All())
        result.append(QVariantMap{{"id", QString::fromStdString(command.id)},
                                  {"title", QString::fromStdString(language() == "ru" && !command.titleRu.empty() ? command.titleRu :
                                                                    language() == "en" && !command.titleEn.empty() ? command.titleEn : command.title)},
                                  {"hint", QString::fromStdString(command.hint)}});
    result.append(QVariantMap{{"id", "core:undo"}, {"title", text("undo")}, {"hint", "FlowDeck"}});
    result.append(QVariantMap{{"id", "core:settings"}, {"title", text("settings")}, {"hint", "FlowDeck"}});
    return result;
}
QVariantList FlowDeckController::searchCommands(const QString& query) const {
    const auto all = commands();
    const auto needle = query.trimmed().toCaseFolded();
    if (needle.isEmpty()) return all;
    const auto score = [&needle](const QString& value) -> int {
        const auto haystack = value.toCaseFolded();
        int total = 0, next = 0, run = 0;
        for (const auto character : needle) {
            bool found = false;
            while (next < haystack.size()) {
                if (haystack[next] == character) {
                    const bool boundary = next == 0 || !haystack[next-1].isLetterOrNumber();
                    total += boundary ? 12 : 2 + run * 3;
                    ++run; ++next; found = true; break;
                }
                ++next; run = 0;
            }
            if (!found) return 0;
        }
        if (haystack.startsWith(needle)) total += 8;
        return qMax(1,total-static_cast<int>(haystack.size()/16));
    };
    QVector<QPair<int,QVariantMap>> ranked;
    for (const auto& value : all) {
        const auto command = value.toMap();
        const int valueScore = qMax(score(command.value("title").toString()),
                                    score(command.value("id").toString())-5);
        if (valueScore > 0) ranked.append({valueScore,command});
    }
    std::stable_sort(ranked.begin(),ranked.end(),[](const auto& a,const auto& b){return a.first>b.first;});
    QVariantList result;
    for (const auto& entry : ranked) result.append(entry.second);
    return result;
}
QVariantList FlowDeckController::plugins() const {
    QVariantList list;
    QDir root(QCoreApplication::applicationDirPath()+"/plugins");
    for (const auto& folder : root.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
        QFile manifest(root.filePath(folder+"/manifest.json"));
        if (!manifest.open(QIODevice::ReadOnly)) continue;
        const auto data = QJsonDocument::fromJson(manifest.readAll()).object();
        if (data.isEmpty()) continue;
        list.append(QVariantMap{{"name",data.value("name").toString(folder)},
                                {"version",data.value("version").toString()},
                                {"language",data.value("entry").toString().endsWith(".lua") ? "Lua" : "Python"},
                                {"description",data.value("description").toString()}});
    }
    return list;
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
        {"pluginWarning", {"Плагины Python, Lua и C++ выполняют доверенный локальный код с доступом к системе.",
                            "Python, Lua and C++ plugins run trusted local code with system access."}},
        {"search", {"Поиск команд и пространств", "Search commands and spaces"}},
        {"noWindow", {"Окно не назначено", "No window assigned"}},
        {"import", {"Импорт", "Import"}}, {"export", {"Экспорт", "Export"}},
        {"checkUpdates", {"Проверить обновления", "Check updates"}},
        {"installUpdate", {"Установить обновление", "Install update"}},
        {"openPlugins", {"Открыть папку плагинов", "Open plugins folder"}},
        {"aspectRatio", {"Соотношение сторон", "Aspect ratio"}},
        {"free", {"Свободно", "Free"}}, {"auto", {"Авто", "Auto"}},
        {"unassigned", {"без назначений", "unassigned"}},
        {"arguments", {"Аргументы", "Arguments"}},
        {"script", {"Скрипт / заголовок", "Script / title"}},
        {"exe", {"Путь EXE / окончание", "EXE path / suffix"}},
        {"windowClass", {"Класс окна", "Window class"}},
        {"titlePattern", {"Шаблон заголовка (regex)", "Title pattern (regex)"}},
    };
    const auto it = strings.find(key);
    return it == strings.end() ? key : (language() == "en" ? it.value().second : it.value().first);
}
void FlowDeckController::setStatus(const QString& value) { status_ = value; emit statusChanged(); }
void FlowDeckController::saveCurrent(const Workspace& w) {
    QString error;
    if (!store_.saveWorkspace(selected_, w, &error)) setStatus(error);
    currentPlan_ = WorkspaceEngine::plan(w);
    prepared_ = false;
    emit workspacesChanged(); emit selectedChanged(); emit previewChanged(); emit commandsChanged(); emit hotkeysChanged();
}
void FlowDeckController::selectWorkspace(int i) {
    if (i < 0 || i >= store_.workspaces().size()) return;
    selected_ = i; currentPlan_ = WorkspaceEngine::plan(store_.workspaces()[i]);
    prepared_ = false;
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
    else if (field == "aspectRatio") z.aspectRatio = qBound(0.0,value.toDouble(),10.0);
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
    w.zones[i].executable = exe; w.zones[i].windowClass = cls;
    w.zones[i].titlePattern = title.isEmpty() ? QString() : "^" + QRegularExpression::escape(title) + "$";
    saveCurrent(w);
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
    else if (field == "arguments") a.arguments = QProcess::splitCommand(value.toString());
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
    if (!prepared_ && !w.before.isEmpty()) {
        if (!WorkspaceEngine::prepare(w,&error)) { setStatus(error); return; }
        prepared_ = true;
        currentPlan_ = WorkspaceEngine::plan(w);
        emit previewChanged(); emit windowsChanged();
        setStatus(language()=="ru" ? "Шаги выполнены. Проверьте обновлённый план и примените." :
                                     "Steps completed. Review the updated plan, then apply.");
        return;
    }
    if (!WorkspaceEngine::apply(currentPlan_,w,&error,prepared_)) {
        currentPlan_ = WorkspaceEngine::plan(w); emit previewChanged(); setStatus(error); return;
    }
    prepared_ = false;
    setStatus(language()=="en" ? "Workspace applied" : "Пространство применено"); refresh();
}
void FlowDeckController::undo() { QString error; WorkspaceEngine::undo(&error); prepared_ = false; setStatus(error); refresh(); }
void FlowDeckController::refresh() {
    if (store_.workspaces().isEmpty()) return;
    currentPlan_ = WorkspaceEngine::plan(store_.workspaces()[selected_]); emit windowsChanged(); emit previewChanged();
}
void FlowDeckController::setSetting(const QString& key, const QVariant& value) {
    QVariant accepted = value;
    if (key == "accent" && !QColor(value.toString()).isValid()) {
        setStatus(language()=="ru" ? "Неверный цвет" : "Invalid color"); return;
    }
    if (key == "scale") accepted = qBound(.8,value.toDouble(),1.5);
    if (key == "language" && value.toString() != "ru" && value.toString() != "en") return;
    if (key == "channel" && value.toString() != "preview" && value.toString() != "stable") return;
    if (key == "density" && value.toString() != "compact" && value.toString() != "comfortable") return;
    if (key == "contrast" && value.toString() != "normal" && value.toString() != "high") return;
    store_.setSetting(key, QJsonValue::fromVariant(accepted)); emit settingsChanged(); emit commandsChanged();
    if (key == "paletteHotkey") emit hotkeysChanged();
    if (key == "language") { emit workspacesChanged(); emit selectedChanged(); }
}
void FlowDeckController::restoreSession(bool launchMissing) {
    setStatus(WorkspaceEngine::restore(store_.lastSession(), launchMissing, false)); refresh();
    store_.beginAutosave();
}
void FlowDeckController::dismissRestoration() { store_.beginAutosave(); }
QString FlowDeckController::restorationSummary() const {
    return WorkspaceEngine::restore(store_.lastSession(), false, true);
}
QVariantList FlowDeckController::restorationDiff() const {
    QVariantList result;
    const auto live = WorkspaceEngine::windows();
    QSet<HWND> used;
    for (const auto& value : store_.lastSession().value("windows").toArray()) {
        const auto saved = value.toObject();
        const auto exe = saved.value("executable").toString();
        const auto title = saved.value("title").toString();
        const auto klass = saved.value("windowClass").toString();
        auto it = std::find_if(live.begin(),live.end(),[&](const WindowRecord& w) {
            return !used.contains(w.handle) && w.executable.compare(exe,Qt::CaseInsensitive)==0 &&
                   w.windowClass.compare(klass,Qt::CaseInsensitive)==0 && w.title==title;
        });
        if (it == live.end()) it = std::find_if(live.begin(),live.end(),[&](const WindowRecord& w) {
            return !used.contains(w.handle) && w.executable.compare(exe,Qt::CaseInsensitive)==0 &&
                   w.windowClass.compare(klass,Qt::CaseInsensitive)==0;
        });
        if (it != live.end()) used.insert(it->handle);
        const auto rect = saved.value("rect").toObject();
        result.append(QVariantMap{{"title",title},{"executable",exe},
                                  {"saved",QString("%1,%2  %3×%4").arg(rect.value("x").toInt()).arg(rect.value("y").toInt()).arg(rect.value("w").toInt()).arg(rect.value("h").toInt())},
                                  {"current",it==live.end() ? QString() : QString("%1,%2  %3×%4").arg(it->rect.x()).arg(it->rect.y()).arg(it->rect.width()).arg(it->rect.height())},
                                  {"missing",it==live.end()}});
    }
    return result;
}
void FlowDeckController::runCommand(const QString& id) {
    if (id.startsWith("workspace:")) {
        for (int i=0;i<store_.workspaces().size();++i) if (store_.workspaces()[i].id==id.mid(10)) {
            selectWorkspace(i);
            if (store_.workspaces()[i].directApply) {
                applySelected(); if (prepared_) emit requestManager();
            } else emit requestManager();
            return;
        }
    }
    if (id=="core:undo") { undo(); return; }
    if (id=="core:settings") { emit requestManager(); return; }
    Palette::Instance().ExecuteById(id.toStdString());
}
void FlowDeckController::openPluginsFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath()+"/plugins"));
}
void FlowDeckController::installPlugin() {
    const auto source = QFileDialog::getExistingDirectory(nullptr,
        language()=="ru" ? "Выберите папку плагина" : "Select plugin folder");
    if (source.isEmpty()) return;
    QFile manifest(source+"/manifest.json");
    if (!manifest.open(QIODevice::ReadOnly)) { setStatus("manifest.json missing"); return; }
    const auto parsed = QJsonDocument::fromJson(manifest.readAll());
    if (!parsed.isObject()) { setStatus("Invalid manifest.json"); return; }
    const auto data = parsed.object();
    const auto name = data.value("name").toString();
    const auto entry = data.value("entry").toString();
    static const QRegularExpression safeName("^[A-Za-z0-9_-]+$");
    static const QRegularExpression safeEntry("^[A-Za-z0-9_.-]+\\.(py|lua)$");
    if (!safeName.match(name).hasMatch() ||
        !safeEntry.match(entry).hasMatch() || !QFileInfo::exists(source+"/"+entry)) {
        setStatus("Invalid plugin name or entry"); return;
    }
    const auto message = language()=="ru" ?
        QString("Установить %1? Плагин выполняет доверенный локальный код с вашими правами. Проверьте его содержимое.").arg(name) :
        QString("Install %1? Plugins run trusted local code with your permissions. Review its contents first.").arg(name);
    if (QMessageBox::warning(nullptr,"FlowDeck",message,QMessageBox::Yes|QMessageBox::No,QMessageBox::No)
        != QMessageBox::Yes) return;
    const auto target = QCoreApplication::applicationDirPath()+"/plugins/"+name;
    if (QDir(target).exists()) { setStatus("Plugin already installed"); return; }
    if (!QDir().mkpath(target)) { setStatus("Could not create plugin directory"); return; }
    QDirIterator files(source,QDir::Files,QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const auto file = files.next();
        const auto relative = QDir(source).relativeFilePath(file);
        const auto destination = QDir(target).filePath(relative);
        if (!QDir().mkpath(QFileInfo(destination).absolutePath()) || !QFile::copy(file,destination)) {
            setStatus("Plugin copy failed: "+relative); return;
        }
    }
    setStatus(language()=="ru" ? "Плагин установлен. Перезапустите FlowDeck." :
                              "Plugin installed. Restart FlowDeck.");
    emit pluginsChanged();
}
} // namespace flowdeck
