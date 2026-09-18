#include "core/update_manager.hpp"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include <QVersionNumber>

namespace flowdeck {
namespace {
QString cacheDir() {
#ifdef Q_OS_WIN
    return qEnvironmentVariable("LOCALAPPDATA") + "/FlowDeck/updates";
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/updates";
#endif
}
QString currentTag() {
#ifdef FLOWDECK_BUILD_TAG
    return QStringLiteral(FLOWDECK_BUILD_TAG);
#else
    return "local";
#endif
}
}
UpdateManager::UpdateManager(QObject* parent) : QObject(parent) {
    timer_.setInterval(24 * 60 * 60 * 1000);
    connect(&timer_, &QTimer::timeout, this, [this] { check(channel_); });
    timer_.start();
}
void UpdateManager::setStatus(const QString& value) { status_ = value; emit changed(); }
void UpdateManager::check(const QString& channel) {
    channel_ = channel == "stable" ? "stable" : "preview";
    QNetworkRequest request(QUrl("https://api.github.com/repos/dw1rf/flowdeck/releases?per_page=100"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "FlowDeck-updater");
    auto* reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const auto bytes = reply->readAll();
        const auto error = reply->error();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) { setStatus("Update check failed"); return; }
        const auto releases = QJsonDocument::fromJson(bytes).array();
        for (const auto& item : releases) {
            const auto release = item.toObject();
            const auto tag = release.value("tag_name").toString();
            if (release.value("draft").toBool()) continue;
            if (channel_ == "preview" && !tag.startsWith("build-")) continue;
            if (channel_ == "stable" && !tag.startsWith('v')) continue;
            if (tag == currentTag()) { setStatus("Up to date"); return; }
            if (channel_ == "preview" && currentTag().startsWith("build-") &&
                tag.mid(6).toInt() <= currentTag().mid(6).toInt()) { setStatus("Up to date"); return; }
            if (channel_ == "stable" && currentTag().startsWith('v') &&
                QVersionNumber::compare(QVersionNumber::fromString(tag.mid(1)),
                                        QVersionNumber::fromString(currentTag().mid(1))) <= 0) {
                setStatus("Up to date"); return;
            }
            QUrl setup, hash;
            for (const auto& asset : release.value("assets").toArray()) {
                const auto a = asset.toObject();
                const auto name = a.value("name").toString();
                if (name == "FlowDeck-Setup-x64.exe") setup = QUrl(a.value("browser_download_url").toString());
                if (name == "FlowDeck-Setup-x64.exe.sha256") hash = QUrl(a.value("browser_download_url").toString());
            }
            if (!setup.isValid() || !hash.isValid()) continue;
            version_ = tag; setupUrl_ = setup; hashUrl_ = hash; ready_ = false;
            setStatus("Downloading " + tag);
            cacheCurrentInstaller(releases);
            download(hashUrl_, "FlowDeck-Setup-x64.exe.sha256", true);
            return;
        }
        setStatus("No update available");
    });
}
void UpdateManager::cacheCurrentInstaller(const QJsonArray& releases) {
    rollbackRequired_ = false;
    if (currentTag() == "local" || QFile::exists(cacheDir()+"/last-installed-installer.exe")) return;
    for (const auto& item : releases) {
        const auto release = item.toObject();
        if (release.value("tag_name").toString() != currentTag()) continue;
        QUrl setup, hash;
        for (const auto& asset : release.value("assets").toArray()) {
            const auto a = asset.toObject();
            if (a.value("name") == "FlowDeck-Setup-x64.exe") setup = QUrl(a.value("browser_download_url").toString());
            if (a.value("name") == "FlowDeck-Setup-x64.exe.sha256") hash = QUrl(a.value("browser_download_url").toString());
        }
        if (!setup.isValid() || !hash.isValid() || setup.host() != "github.com" || hash.host() != "github.com") return;
        rollbackRequired_ = true;
        QNetworkRequest hashRequest(hash);
        hashRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
        auto* hashReply = network_.get(hashRequest);
        connect(hashReply,&QNetworkReply::finished,this,[this,hashReply,setup] {
            const auto expected = hashReply->readAll().left(64).toLower();
            const auto error = hashReply->error(); hashReply->deleteLater();
            if (error != QNetworkReply::NoError || expected.size()!=64) return;
            QNetworkRequest setupRequest(setup);
            setupRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);
            auto* setupReply = network_.get(setupRequest);
            connect(setupReply,&QNetworkReply::finished,this,[this,setupReply,expected] {
                const auto bytes = setupReply->readAll();
                const auto error = setupReply->error(); setupReply->deleteLater();
                if (error != QNetworkReply::NoError ||
                    QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=expected) return;
                QDir().mkpath(cacheDir());
                QSaveFile file(cacheDir()+"/last-installed-installer.exe");
                if (file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size() && file.commit() && ready_)
                    setStatus("Ready to install " + version_);
            });
        });
        return;
    }
}
void UpdateManager::download(const QUrl& url, const QString& filename, bool checksum) {
    if (url.scheme() != "https" ||
        (url.host() != "github.com" && url.host() != "objects.githubusercontent.com" &&
         url.host() != "release-assets.githubusercontent.com")) {
        setStatus("Untrusted update URL"); return;
    }
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, filename, checksum] {
        const auto bytes = reply->readAll();
        const auto error = reply->error();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) { setStatus("Update download failed"); return; }
        if (checksum) {
            expectedHash_ = bytes.left(64).toLower();
            if (expectedHash_.size() != 64) { setStatus("Invalid checksum file"); return; }
            download(setupUrl_, "FlowDeck-Setup-x64.exe", false);
            return;
        }
        const auto actual = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
        if (actual != expectedHash_) { setStatus("Update checksum mismatch"); return; }
        QDir().mkpath(cacheDir());
        QSaveFile file(cacheDir() + "/" + filename);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
            setStatus("Could not save update"); return;
        }
        ready_ = true; setStatus("Ready to install " + version_);
    });
}
void UpdateManager::install() {
    if (!ready_) return;
    if (rollbackRequired_ && !QFile::exists(cacheDir()+"/last-installed-installer.exe")) {
        setStatus("Preparing rollback installer; try again shortly"); return;
    }
    const auto path = cacheDir() + "/FlowDeck-Setup-x64.exe";
    if (!QFile::exists(path)) { setStatus("Installer missing"); return; }
    const auto previous = cacheDir() + "/previous-installer.exe";
    QFile::remove(previous);
    const auto old = cacheDir() + "/last-installed-installer.exe";
    if (QFile::exists(old)) QFile::copy(old, previous);
    QFile::remove(old); QFile::copy(path, old);
    if (!QProcess::startDetached(path, {})) { setStatus("Installer could not start"); return; }
    QCoreApplication::quit();
}
}
