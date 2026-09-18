#include "core/workspace_store.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

namespace flowdeck {

QString WorkspaceStore::dataDirectory() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QJsonObject WorkspaceStore::read(const QString& file) {
    QFile input(QDir(dataDirectory()).filePath(file));
    if (!input.open(QIODevice::ReadOnly)) return {};
    const auto document = QJsonDocument::fromJson(input.readAll());
    return document.isObject() ? document.object() : QJsonObject{};
}

bool WorkspaceStore::write(const QString& file, const QJsonObject& object,
                           QString* error) {
    if (!QDir().mkpath(dataDirectory())) {
        if (error) *error = QStringLiteral("Could not create settings directory.");
        return false;
    }
    QSaveFile output(QDir(dataDirectory()).filePath(file));
    if (!output.open(QIODevice::WriteOnly) ||
        output.write(QJsonDocument(object).toJson()) < 0 || !output.commit()) {
        if (error) *error = output.errorString();
        return false;
    }
    return true;
}

WorkspaceStore::WorkspaceStore(QObject* parent) : QObject(parent) {
    const auto saved = read("workspaces.json");
    for (const auto& value : saved.value("workspaces").toArray())
        workspaces_.append(workspaceFromJson(value.toObject()));
    if (workspaces_.isEmpty()) workspaces_ = defaultWorkspaces();
    settings_ = read("settings.json");
    if (settings_.isEmpty()) {
        settings_ = {{"language", "ru"}, {"accent", "#63d8c7"},
                     {"contrast", "normal"}, {"scale", 1.0},
                     {"density", "comfortable"}, {"channel", "preview"}};
    }
    lastSession_ = read("last-session.json");
    lastFingerprint_ = WorkspaceEngine::fingerprint(WorkspaceEngine::windows());
    scanTimer_.setInterval(1000);
    connect(&scanTimer_, &QTimer::timeout, this, &WorkspaceStore::updateSession);
    writeTimer_.setSingleShot(true);
    writeTimer_.setInterval(1200);
    connect(&writeTimer_, &QTimer::timeout, this, [this] {
        write("last-session.json", WorkspaceEngine::snapshot(), nullptr);
        emit sessionChanged();
    });
}

void WorkspaceStore::setSetting(const QString& key, const QJsonValue& value) {
    settings_.insert(key, value);
    write("settings.json", settings_, nullptr);
    emit settingsChanged();
}

bool WorkspaceStore::save(QString* error) {
    QJsonArray array;
    for (const auto& workspace : workspaces_) array.append(toJson(workspace));
    return write("workspaces.json", {{"version", 1}, {"workspaces", array}}, error);
}

bool WorkspaceStore::saveWorkspace(int index, const Workspace& workspace,
                                   QString* error) {
    if (index < 0 || index >= workspaces_.size()) return false;
    workspaces_[index] = workspace;
    return save(error);
}

int WorkspaceStore::addWorkspace(const Workspace& workspace) {
    workspaces_.append(workspace);
    save();
    return workspaces_.size() - 1;
}

bool WorkspaceStore::removeWorkspace(int index) {
    if (index < 0 || index >= workspaces_.size()) return false;
    workspaces_.removeAt(index);
    return save();
}

void WorkspaceStore::beginAutosave() { scanTimer_.start(); }

void WorkspaceStore::updateSession() {
    const auto current = WorkspaceEngine::fingerprint(WorkspaceEngine::windows());
    if (current == lastFingerprint_) return;
    lastFingerprint_ = current;
    writeTimer_.start();
}

} // namespace flowdeck
