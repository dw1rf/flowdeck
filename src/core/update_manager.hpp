#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>

namespace flowdeck {
class UpdateManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
 public:
    explicit UpdateManager(QObject* parent = nullptr);
    QString status() const;
    void setLanguage(const QString& language) { language_ = language; emit changed(); }
    QString availableVersion() const { return version_; }
    bool ready() const { return ready_; }
    Q_INVOKABLE void check(const QString& channel);
    Q_INVOKABLE void install();
 signals:
    void changed();
 private:
    void download(const QUrl& url, const QString& filename, bool checksum);
    void cacheCurrentInstaller(const QJsonArray& releases);
    void setStatus(const QString& value);
    QNetworkAccessManager network_;
    QTimer timer_;
    QString channel_ = "preview";
    QString status_;
    QString language_ = "ru";
    QString version_;
    QByteArray expectedHash_;
    QUrl setupUrl_;
    QUrl hashUrl_;
    bool ready_ = false;
    bool rollbackRequired_ = false;
};
}
