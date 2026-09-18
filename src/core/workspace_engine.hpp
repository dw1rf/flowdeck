#pragma once

#include <windows.h>

#include <QJsonObject>
#include <QRect>
#include <QString>
#include <QVector>

namespace flowdeck {

struct Zone {
    QString id;
    QString label;
    QRectF bounds; // Normalized to the selected canvas.
    QString executable;
    QString windowClass;
    QString titlePattern;
    double aspectRatio = 0.0;
};

struct ActionStep {
    QString type;
    QString program;
    QStringList arguments;
    QString script;
    int timeoutMs = 10000;
};

struct Workspace {
    QString id;
    QString name;
    QString monitor;
    QString canvasMode = "native";
    int canvasWidth = 2560;
    int canvasHeight = 1440;
    int gap = 8;
    QString hotkey;
    bool directApply = false;
    bool trusted = true;
    QVector<Zone> zones;
    QVector<ActionStep> before;
    QVector<ActionStep> after;
};

struct WindowRecord {
    HWND handle = nullptr;
    QString title;
    QString executable;
    QString windowClass;
    QString monitor;
    QRect rect;
    int showCommand = SW_SHOWNORMAL;
};

struct Placement {
    WindowRecord window;
    QString zoneId;
    QRect target;
};

struct LayoutPlan {
    QRect monitorArea;
    QRect canvas;
    bool monitorMissing = false;
    QVector<Placement> placements;
    QStringList unassignedZones;
    QString fingerprint;
};

class WorkspaceEngine {
 public:
    static QVector<WindowRecord> windows();
    static QVector<QPair<QString, QRect>> monitors();
    static QRect fitCanvas(const QRect& workArea, const Workspace& workspace);
    static LayoutPlan plan(const Workspace& workspace);
    static QString fingerprint(const QVector<WindowRecord>& windows);
    static bool apply(const LayoutPlan& plan, const Workspace& workspace,
                      QString* error);
    static bool undo(QString* error);
    static QJsonObject snapshot();
    static QString restore(const QJsonObject& snapshot, bool launchMissing,
                           bool previewOnly);

 private:
    static QVector<WindowRecord> undoWindows_;
};

QJsonObject toJson(const Workspace& workspace);
Workspace workspaceFromJson(const QJsonObject& object);
QVector<Workspace> defaultWorkspaces();

} // namespace flowdeck
