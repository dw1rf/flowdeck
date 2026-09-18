#pragma once

#include <QObject>
#include <QJsonObject>
#include <QTimer>

#include "core/workspace_engine.hpp"

namespace flowdeck {

class WorkspaceStore : public QObject {
    Q_OBJECT
 public:
    explicit WorkspaceStore(QObject* parent = nullptr);
    const QVector<Workspace>& workspaces() const { return workspaces_; }
    QVector<Workspace>& workspaces() { return workspaces_; }
    const QJsonObject& settings() const { return settings_; }
    const QJsonObject& lastSession() const { return lastSession_; }
    void setSetting(const QString& key, const QJsonValue& value);
    bool save(QString* error = nullptr);
    bool saveWorkspace(int index, const Workspace& workspace, QString* error = nullptr);
    int addWorkspace(const Workspace& workspace);
    bool removeWorkspace(int index);
    void beginAutosave();

 signals:
    void sessionChanged();
    void settingsChanged();

 private:
    static QString dataDirectory();
    static QJsonObject read(const QString& file);
    static bool write(const QString& file, const QJsonObject& object, QString* error);
    void updateSession();
    QVector<Workspace> workspaces_;
    QJsonObject settings_;
    QJsonObject lastSession_;
    QTimer scanTimer_;
    QTimer writeTimer_;
    QString lastFingerprint_;
};

} // namespace flowdeck
