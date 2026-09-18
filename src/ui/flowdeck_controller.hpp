#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "core/workspace_store.hpp"

namespace flowdeck {

class FlowDeckController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList workspaces READ workspaces NOTIFY workspacesChanged)
    Q_PROPERTY(QVariantList windows READ windows NOTIFY windowsChanged)
    Q_PROPERTY(QVariantList monitors READ monitors NOTIFY windowsChanged)
    Q_PROPERTY(QVariantMap selectedWorkspace READ selectedWorkspace NOTIFY selectedChanged)
    Q_PROPERTY(QVariantMap preview READ preview NOTIFY previewChanged)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY selectedChanged)
    Q_PROPERTY(QString language READ language NOTIFY settingsChanged)

 public:
    explicit FlowDeckController(QObject* parent = nullptr);
    QVariantList workspaces() const;
    QVariantList windows() const;
    QVariantList monitors() const;
    QVariantMap selectedWorkspace() const;
    QVariantMap preview() const;
    QVariantMap settings() const;
    QVariantList commands() const;
    QString status() const { return status_; }
    int selectedIndex() const { return selected_; }
    QString language() const;
    WorkspaceStore& store() { return store_; }
    const WorkspaceStore& store() const { return store_; }

    Q_INVOKABLE QString text(const QString& key) const;
    Q_INVOKABLE void selectWorkspace(int index);
    Q_INVOKABLE void createWorkspace();
    Q_INVOKABLE void duplicateWorkspace();
    Q_INVOKABLE void deleteWorkspace();
    Q_INVOKABLE void changeWorkspace(const QString& field, const QVariant& value);
    Q_INVOKABLE void addZone();
    Q_INVOKABLE void removeZone(int index);
    Q_INVOKABLE void editZone(int index, const QString& field,
                               const QVariant& value);
    Q_INVOKABLE void moveZone(int index, double x, double y, double width,
                               double height);
    Q_INVOKABLE void assignZone(int index, const QString& executable,
                                 const QString& windowClass,
                                 const QString& titlePattern);
    Q_INVOKABLE void addAction(bool before);
    Q_INVOKABLE void editAction(bool before, int index, const QString& field,
                                 const QVariant& value);
    Q_INVOKABLE void removeAction(bool before, int index);
    Q_INVOKABLE void trustWorkspace();
    Q_INVOKABLE void importWorkspace();
    Q_INVOKABLE void exportWorkspace();
    Q_INVOKABLE void applySelected();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setSetting(const QString& key, const QVariant& value);
    Q_INVOKABLE void restoreSession(bool launchMissing);
    Q_INVOKABLE QString restorationSummary() const;
    Q_INVOKABLE void runCommand(const QString& id);
    Q_INVOKABLE void openPluginsFolder();

 signals:
    void workspacesChanged();
    void windowsChanged();
    void selectedChanged();
    void previewChanged();
    void settingsChanged();
    void commandsChanged();
    void statusChanged();
    void hotkeysChanged();
    void requestPalette();
    void requestManager();

 private:
    void setStatus(const QString& value);
    void saveCurrent(const Workspace& workspace);
    WorkspaceStore store_;
    int selected_ = 0;
    LayoutPlan currentPlan_;
    QString status_;
};

} // namespace flowdeck
