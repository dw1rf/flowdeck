#include "core/workspace_engine.hpp"

#include <dwmapi.h>
#include <shobjidl.h>

#include <algorithm>

#include <QCryptographicHash>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QThread>
#include <QtMath>

#include "core/palette.hpp"

namespace flowdeck {
namespace {

QString fromWide(const wchar_t* value) { return QString::fromWCharArray(value); }

QRect fromRect(const RECT& rect) {
    return QRect(rect.left, rect.top, rect.right - rect.left,
                 rect.bottom - rect.top);
}

RECT toRect(const QRect& rect) {
    return RECT{rect.x(), rect.y(), rect.x() + rect.width(),
                rect.y() + rect.height()};
}

QString monitorFor(HWND window) {
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST),
                        reinterpret_cast<MONITORINFO*>(&info))) return fromWide(info.szDevice);
    return {};
}

QJsonObject rectJson(const QRect& rect) {
    return {{"x", rect.x()}, {"y", rect.y()}, {"w", rect.width()},
            {"h", rect.height()}};
}

QRect rectFromJson(const QJsonObject& value) {
    return QRect(value.value("x").toInt(), value.value("y").toInt(),
                 value.value("w").toInt(), value.value("h").toInt());
}

QJsonObject zoneJson(const Zone& zone) {
    return {{"id", zone.id}, {"label", zone.label},
            {"x", zone.bounds.x()}, {"y", zone.bounds.y()},
            {"w", zone.bounds.width()}, {"h", zone.bounds.height()},
            {"aspectRatio", zone.aspectRatio},
            {"executable", zone.executable}, {"windowClass", zone.windowClass},
            {"titlePattern", zone.titlePattern}};
}

Zone zoneFromJson(const QJsonObject& object) {
    Zone zone;
    zone.id = object.value("id").toString();
    zone.label = object.value("label").toString();
    zone.bounds = QRectF(object.value("x").toDouble(),
                         object.value("y").toDouble(),
                         object.value("w").toDouble(),
                         object.value("h").toDouble());
    const double width = qBound(.05,zone.bounds.width(),1.0);
    const double height = qBound(.05,zone.bounds.height(),1.0);
    zone.bounds = QRectF(qBound(0.0,zone.bounds.x(),1.0-width),
                         qBound(0.0,zone.bounds.y(),1.0-height),width,height);
    zone.aspectRatio = object.value("aspectRatio").toDouble();
    zone.executable = object.value("executable").toString();
    zone.windowClass = object.value("windowClass").toString();
    zone.titlePattern = object.value("titlePattern").toString();
    return zone;
}

QJsonObject actionJson(const ActionStep& action) {
    QJsonArray args;
    for (const auto& argument : action.arguments) args.append(argument);
    return {{"type", action.type}, {"program", action.program},
            {"arguments", args}, {"script", action.script},
            {"timeoutMs", action.timeoutMs}};
}

ActionStep actionFromJson(const QJsonObject& object) {
    ActionStep action;
    action.type = object.value("type").toString();
    action.program = object.value("program").toString();
    for (const auto& argument : object.value("arguments").toArray())
        action.arguments.append(argument.toString());
    action.script = object.value("script").toString();
    action.timeoutMs = qBound(100, object.value("timeoutMs").toInt(10000), 120000);
    return action;
}

bool matches(const Zone& zone, const WindowRecord& window) {
    if (zone.executable.isEmpty() && zone.windowClass.isEmpty() &&
        zone.titlePattern.isEmpty()) return false;
    if (!zone.executable.isEmpty() &&
        !window.executable.endsWith(zone.executable, Qt::CaseInsensitive)) return false;
    if (!zone.windowClass.isEmpty() &&
        window.windowClass.compare(zone.windowClass, Qt::CaseInsensitive) != 0)
        return false;
    if (!zone.titlePattern.isEmpty()) {
        const QRegularExpression expression(zone.titlePattern,
                                             QRegularExpression::CaseInsensitiveOption);
        if (expression.isValid()) {
            if (!expression.match(window.title).hasMatch()) return false;
        } else if (!window.title.contains(zone.titlePattern, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

bool runStep(const ActionStep& step, QString* error) {
    if (step.type == "launch") {
        if (QProcess::startDetached(step.program, step.arguments)) return true;
        *error = QStringLiteral("Unable to launch %1").arg(step.program);
        return false;
    }
    if (step.type == "powershell") {
        QProcess process;
        const auto mode = QFileInfo::exists(step.script) ? "-File" : "-Command";
        process.start("powershell.exe", {"-NoProfile", "-NonInteractive", mode, step.script});
        if (process.waitForFinished(step.timeoutMs) &&
            process.exitStatus() == QProcess::NormalExit &&
            process.exitCode() == 0) return true;
        if (process.state() != QProcess::NotRunning) process.kill();
        *error = QStringLiteral("PowerShell step failed: %1")
                     .arg(QString::fromUtf8(process.readAllStandardError()));
        return false;
    }
    if (step.type == "wait") {
        QElapsedTimer clock;
        clock.start();
        while (clock.elapsed() < step.timeoutMs) {
            for (const auto& window : WorkspaceEngine::windows()) {
                if (window.executable.endsWith(step.program, Qt::CaseInsensitive) &&
                    (step.script.isEmpty() ||
                     window.title.contains(step.script, Qt::CaseInsensitive))) return true;
            }
            QThread::msleep(100);
        }
        *error = QStringLiteral("Window did not appear: %1").arg(step.program);
        return false;
    }
    if (step.type == "focus" || step.type == "minimize") {
        for (const auto& window : WorkspaceEngine::windows()) {
            if (!window.executable.endsWith(step.program, Qt::CaseInsensitive)) continue;
            if (step.type == "focus" && !SetForegroundWindow(window.handle)) {
                *error = QStringLiteral("Could not focus window: %1").arg(window.title);
                return false;
            }
            if (step.type == "minimize") ShowWindow(window.handle, SW_MINIMIZE);
            return true;
        }
        *error = QStringLiteral("Window not found: %1").arg(step.program);
        return false;
    }
    if (step.type == "plugin") {
        if (Palette::Instance().ExecuteById(step.program.toStdString())) return true;
        *error = QStringLiteral("Plugin command not found: %1").arg(step.program);
        return false;
    }
    *error = QStringLiteral("Unsupported action: %1").arg(step.type);
    return false;
}

} // namespace

QVector<WindowRecord> WorkspaceEngine::undoWindows_;

QVector<WindowRecord> WorkspaceEngine::windows() {
    QVector<WindowRecord> result;
    IVirtualDesktopManager* desktop = nullptr;
    CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL,
                     IID_PPV_ARGS(&desktop));
    struct Context { QVector<WindowRecord>* output; IVirtualDesktopManager* desktop; } context{&result, desktop};
    EnumWindows([](HWND window, LPARAM context) -> BOOL {
        auto* state = reinterpret_cast<Context*>(context);
        auto* output = state->output;
        if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) ||
            GetWindowLongPtrW(window, GWL_EXSTYLE) & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE))
            return TRUE;
        if (state->desktop) {
            BOOL current = FALSE;
            if (SUCCEEDED(state->desktop->IsWindowOnCurrentVirtualDesktop(window, &current)) && !current)
                return TRUE;
        }
        DWORD pid = 0;
        GetWindowThreadProcessId(window, &pid);
        if (pid == GetCurrentProcessId()) return TRUE;
        int cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED,
                                            &cloaked, sizeof(cloaked))) && cloaked)
            return TRUE;
        wchar_t title[512]{};
        if (!GetWindowTextW(window, title, 512)) return TRUE;
        wchar_t className[256]{};
        GetClassNameW(window, className, 256);
        wchar_t executable[32768]{};
        DWORD length = 32768;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (process) {
            QueryFullProcessImageNameW(process, 0, executable, &length);
            CloseHandle(process);
        }
        WINDOWPLACEMENT placement{};
        placement.length = sizeof(placement);
        if (!GetWindowPlacement(window, &placement)) return TRUE;
        RECT bounds = placement.rcNormalPosition;
        if (bounds.right <= bounds.left || bounds.bottom <= bounds.top)
            GetWindowRect(window, &bounds);
        if (bounds.right - bounds.left < 120 || bounds.bottom - bounds.top < 80)
            return TRUE;
        output->append({window, fromWide(title), fromWide(executable),
                        fromWide(className), monitorFor(window),
                        fromRect(bounds), static_cast<int>(placement.showCmd)});
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
    if (desktop) desktop->Release();
    return result;
}

QVector<QPair<QString, QRect>> WorkspaceEngine::monitors() {
    QVector<QPair<QString, QRect>> result;
    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM context) -> BOOL {
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            if (GetMonitorInfoW(monitor, reinterpret_cast<MONITORINFO*>(&info))) {
                auto* output = reinterpret_cast<QVector<QPair<QString, QRect>>*>(context);
                output->append({fromWide(info.szDevice), fromRect(info.rcWork)});
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&result));
    return result;
}

QRect WorkspaceEngine::fitCanvas(const QRect& workArea,
                                  const Workspace& workspace) {
    if (workspace.canvasMode == "native") return workArea;
    int desiredWidth = qMax(1, workspace.canvasWidth);
    int desiredHeight = qMax(1, workspace.canvasHeight);
    if (workspace.canvasMode == "16:9") desiredWidth = qRound(desiredHeight * 16.0 / 9.0);
    if (workspace.canvasMode == "21:9") desiredWidth = qRound(desiredHeight * 21.0 / 9.0);
    const double scale = qMin(1.0, qMin(double(workArea.width()) / desiredWidth,
                                        double(workArea.height()) / desiredHeight));
    const int width = qMax(1, qRound(desiredWidth * scale));
    const int height = qMax(1, qRound(desiredHeight * scale));
    return QRect(workArea.x() + (workArea.width() - width) / 2,
                 workArea.y() + (workArea.height() - height) / 2,
                 width, height);
}

QString WorkspaceEngine::fingerprint(const QVector<WindowRecord>& list) {
    QByteArray serialized;
    for (const auto& window : list) {
        serialized += QByteArray::number(reinterpret_cast<quintptr>(window.handle));
        serialized += window.title.toUtf8() + window.executable.toUtf8();
        serialized += QByteArray::number(window.rect.x()) + ',';
        serialized += QByteArray::number(window.rect.y()) + ',';
        serialized += QByteArray::number(window.rect.width()) + ',';
        serialized += QByteArray::number(window.rect.height()) + ';';
        serialized += QByteArray::number(window.showCommand) + window.monitor.toUtf8();
    }
    return QString::fromLatin1(QCryptographicHash::hash(serialized,
        QCryptographicHash::Sha256).toHex());
}

LayoutPlan WorkspaceEngine::plan(const Workspace& workspace) {
    LayoutPlan result;
    const auto availableMonitors = monitors();
    if (availableMonitors.isEmpty()) return result;
    const auto found = std::find_if(availableMonitors.begin(), availableMonitors.end(),
        [&](const auto& entry) { return entry.first == workspace.monitor; });
    result.monitorMissing = !workspace.monitor.isEmpty() && found == availableMonitors.end();
    result.monitorArea = found == availableMonitors.end() ?
        availableMonitors.front().second : found->second;
    result.canvas = fitCanvas(result.monitorArea, workspace);
    const auto candidates = windows();
    result.fingerprint = fingerprint(candidates);
    QSet<HWND> used;
    for (const auto& zone : workspace.zones) {
        const auto match = std::find_if(candidates.begin(), candidates.end(),
            [&](const WindowRecord& candidate) {
                return !used.contains(candidate.handle) && matches(zone, candidate);
            });
        if (match == candidates.end()) {
            result.unassignedZones.append(zone.label);
            continue;
        }
        used.insert(match->handle);
        const auto b = zone.bounds;
        const int gap = qMax(0, workspace.gap);
        QRect target(result.canvas.x() + qRound(b.x() * result.canvas.width()) + gap,
                     result.canvas.y() + qRound(b.y() * result.canvas.height()) + gap,
                     qRound(b.width() * result.canvas.width()) - gap * 2,
                     qRound(b.height() * result.canvas.height()) - gap * 2);
        if (zone.aspectRatio > 0 && target.isValid()) {
            const int width = qMin(target.width(), qRound(target.height() * zone.aspectRatio));
            const int height = qMin(target.height(), qRound(target.width() / zone.aspectRatio));
            target = QRect(target.x() + (target.width()-width)/2,
                           target.y() + (target.height()-height)/2, width, height);
        }
        if (target.width() < 120 || target.height() < 80) continue;
        result.placements.append({*match, zone.id, target});
    }
    return result;
}

bool WorkspaceEngine::apply(const LayoutPlan& layout, const Workspace& workspace,
                            QString* error) {
    if (layout.fingerprint != fingerprint(windows())) {
        *error = QStringLiteral("Windows changed. Refresh the preview before applying.");
        return false;
    }
    if (layout.placements.isEmpty()) {
        *error = QStringLiteral("Assign at least one window to a zone.");
        return false;
    }
    if (!workspace.trusted && (!workspace.before.isEmpty() || !workspace.after.isEmpty())) {
        *error = QStringLiteral("Review and trust imported actions first.");
        return false;
    }
    undoWindows_.clear();
    for (const auto& placement : layout.placements) undoWindows_.append(placement.window);
    for (const auto& action : workspace.before)
        if (!runStep(action, error)) return false;
    for (const auto& placement : layout.placements) {
        if (!IsWindow(placement.window.handle)) {
            *error = QStringLiteral("A window closed during layout.");
            undo(nullptr);
            return false;
        }
        ShowWindow(placement.window.handle, SW_RESTORE);
        const auto& r = placement.target;
        if (!SetWindowPos(placement.window.handle, nullptr, r.x(), r.y(),
                          r.width(), r.height(), SWP_NOZORDER | SWP_NOACTIVATE)) {
            *error = QStringLiteral("Windows rejected a layout change.");
            undo(nullptr);
            return false;
        }
    }
    for (const auto& action : workspace.after) {
        if (!runStep(action, error)) {
            undo(nullptr);
            return false;
        }
    }
    return true;
}

bool WorkspaceEngine::undo(QString* error) {
    bool success = true;
    const auto displays = monitors();
    for (const auto& window : undoWindows_) {
        if (!IsWindow(window.handle)) { success = false; continue; }
        ShowWindow(window.handle, SW_RESTORE);
        QRect r = window.rect;
        const auto found = std::find_if(displays.begin(),displays.end(),[&](const auto& display) {
            return display.first == window.monitor;
        });
        if (found == displays.end() && !displays.isEmpty()) {
            const auto& area = displays.front().second;
            r.moveTopLeft(area.topLeft());
            r.setSize(r.size().boundedTo(area.size()));
        }
        if (!SetWindowPos(window.handle, nullptr, r.x(), r.y(), r.width(),
                          r.height(), SWP_NOZORDER | SWP_NOACTIVATE)) success = false;
        if (window.showCommand == SW_SHOWMAXIMIZED) ShowWindow(window.handle, SW_MAXIMIZE);
        if (window.showCommand == SW_SHOWMINIMIZED) ShowWindow(window.handle, SW_MINIMIZE);
    }
    if (!success && error) *error = QStringLiteral("Some windows could not be restored.");
    undoWindows_.clear();
    return success;
}

QJsonObject WorkspaceEngine::snapshot() {
    QJsonArray array;
    QJsonArray displayArray;
    for (const auto& monitor : monitors())
        displayArray.append(QJsonObject{{"name",monitor.first},{"workArea",rectJson(monitor.second)}});
    for (const auto& window : windows()) {
        array.append(QJsonObject{{"title", window.title},
            {"executable", window.executable}, {"windowClass", window.windowClass},
            {"monitor", window.monitor}, {"rect", rectJson(window.rect)},
            {"showCommand", window.showCommand}});
    }
    return {{"version", 1}, {"savedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
            {"monitors", displayArray}, {"windows", array}};
}

QString WorkspaceEngine::restore(const QJsonObject& saved, bool launchMissing,
                                  bool previewOnly) {
    const auto records = saved.value("windows").toArray();
    const auto displays = monitors();
    const auto oldDisplays = saved.value("monitors").toArray();
    auto live = windows();
    QSet<HWND> matched;
    int restored = 0;
    int missing = 0;
    for (const auto& entry : records) {
        const auto record = entry.toObject();
        const auto executable = record.value("executable").toString();
        const auto windowClass = record.value("windowClass").toString();
        const auto title = record.value("title").toString();
        auto found = std::find_if(live.begin(), live.end(), [&](const WindowRecord& item) {
            return !matched.contains(item.handle) &&
                   item.executable.compare(executable, Qt::CaseInsensitive) == 0 &&
                   item.windowClass.compare(windowClass, Qt::CaseInsensitive) == 0 &&
                   item.title == title;
        });
        if (found == live.end()) {
            found = std::find_if(live.begin(), live.end(), [&](const WindowRecord& item) {
                return !matched.contains(item.handle) &&
                       item.executable.compare(executable, Qt::CaseInsensitive) == 0 &&
                       item.windowClass.compare(windowClass, Qt::CaseInsensitive) == 0;
            });
        }
        if (found == live.end()) {
            ++missing;
            if (launchMissing && !previewOnly && !executable.isEmpty()) {
                QProcess::startDetached(executable, {});
                QElapsedTimer wait; wait.start();
                while (wait.elapsed() < 10000) {
                    QThread::msleep(200);
                    live = windows();
                    found = std::find_if(live.begin(),live.end(),[&](const WindowRecord& item) {
                        return !matched.contains(item.handle) && item.executable.compare(executable,Qt::CaseInsensitive)==0;
                    });
                    if (found != live.end()) { --missing; break; }
                }
            }
            if (found == live.end()) continue;
        }
        matched.insert(found->handle);
        if (!previewOnly) {
            auto rect = rectFromJson(record.value("rect").toObject());
            if (!rect.isValid()) continue;
            const auto monitorName = record.value("monitor").toString();
            auto screen = std::find_if(displays.begin(),displays.end(),[&](const auto& d){return d.first==monitorName;});
            if (screen == displays.end() && !displays.isEmpty()) {
                QRect oldArea;
                for (const auto& d : oldDisplays) if (d.toObject().value("name").toString()==monitorName)
                    oldArea = rectFromJson(d.toObject().value("workArea").toObject());
                const auto& newArea = displays.front().second;
                if (oldArea.isValid()) rect.translate(newArea.topLeft()-oldArea.topLeft());
                rect.moveLeft(qBound(newArea.left(),rect.left(),newArea.right()-qMin(rect.width(),newArea.width())+1));
                rect.moveTop(qBound(newArea.top(),rect.top(),newArea.bottom()-qMin(rect.height(),newArea.height())+1));
            }
            ShowWindow(found->handle, SW_RESTORE);
            SetWindowPos(found->handle, nullptr, rect.x(), rect.y(), rect.width(),
                         rect.height(), SWP_NOZORDER | SWP_NOACTIVATE);
            const int state = record.value("showCommand").toInt(SW_SHOWNORMAL);
            if (state == SW_SHOWMAXIMIZED) ShowWindow(found->handle, SW_MAXIMIZE);
            if (state == SW_SHOWMINIMIZED) ShowWindow(found->handle, SW_MINIMIZE);
        }
        ++restored;
    }
    return QStringLiteral("%1 matched, %2 missing").arg(restored).arg(missing);
}

QJsonObject toJson(const Workspace& workspace) {
    QJsonArray zones;
    for (const auto& zone : workspace.zones) zones.append(zoneJson(zone));
    QJsonArray before;
    for (const auto& action : workspace.before) before.append(actionJson(action));
    QJsonArray after;
    for (const auto& action : workspace.after) after.append(actionJson(action));
    return {{"id", workspace.id}, {"name", workspace.name},
            {"monitor", workspace.monitor}, {"canvasMode", workspace.canvasMode},
            {"canvasWidth", workspace.canvasWidth}, {"canvasHeight", workspace.canvasHeight},
            {"gap", workspace.gap}, {"hotkey", workspace.hotkey},
            {"directApply", workspace.directApply}, {"trusted", workspace.trusted},
            {"zones", zones}, {"before", before}, {"after", after}};
}

Workspace workspaceFromJson(const QJsonObject& object) {
    Workspace workspace;
    workspace.id = object.value("id").toString();
    workspace.name = object.value("name").toString();
    workspace.monitor = object.value("monitor").toString();
    workspace.canvasMode = object.value("canvasMode").toString("native");
    workspace.canvasWidth = qMax(1, object.value("canvasWidth").toInt(2560));
    workspace.canvasHeight = qMax(1, object.value("canvasHeight").toInt(1440));
    workspace.gap = qBound(0, object.value("gap").toInt(8), 100);
    workspace.hotkey = object.value("hotkey").toString();
    workspace.directApply = object.value("directApply").toBool();
    workspace.trusted = object.value("trusted").toBool(false);
    for (const auto& value : object.value("zones").toArray())
        workspace.zones.append(zoneFromJson(value.toObject()));
    for (const auto& value : object.value("before").toArray())
        workspace.before.append(actionFromJson(value.toObject()));
    for (const auto& value : object.value("after").toArray())
        workspace.after.append(actionFromJson(value.toObject()));
    return workspace;
}

QVector<Workspace> defaultWorkspaces() {
    Workspace coding;
    coding.id = "coding";
    coding.name = "Coding";
    coding.hotkey = "Ctrl+Shift+T";
    coding.zones = {{"ide", "IDE", QRectF(0, 0, .7, 1), {}, {}, {}},
                    {"browser", "Browser", QRectF(.7, 0, .3, 1), {}, {}, {}}};
    Workspace trading;
    trading.id = "trading";
    trading.name = "Trading";
    trading.hotkey = "Ctrl+Shift+G";
    trading.zones = {{"chart1", "Chart 1", QRectF(0, 0, .333, .5), {}, {}, {}},
                     {"chart2", "Chart 2", QRectF(.333, 0, .334, .5), {}, {}, {}},
                     {"chart3", "Chart 3", QRectF(.667, 0, .333, .5), {}, {}, {}},
                     {"terminal", "Terminal", QRectF(0, .5, 1, .5), {}, {}, {}}};
    Workspace chill;
    chill.id = "chill";
    chill.name = "Chill";
    chill.zones = {{"video", "Video 16:9", QRectF(0, 0, .75, 1), {}, {}, {}},
                   {"discord", "Discord", QRectF(.75, 0, .25, 1), {}, {}, {}}};
    chill.zones[0].aspectRatio = 16.0 / 9.0;
    Workspace grid;
    grid.id = "grid";
    grid.name = "Grid";
    grid.zones = {{"top-left", "Top left", QRectF(0,0,.5,.5), {},{},{}},
                  {"top-right", "Top right", QRectF(.5,0,.5,.5), {},{},{}},
                  {"bottom-left", "Bottom left", QRectF(0,.5,.5,.5), {},{},{}},
                  {"bottom-right", "Bottom right", QRectF(.5,.5,.5,.5), {},{},{}}};
    return {coding, trading, chill, grid};
}

} // namespace flowdeck
