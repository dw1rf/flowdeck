#include <windows.h>
#include <objbase.h>
#include <QApplication>
#include <QFile>
#include <QAbstractNativeEventFilter>
#include <QIcon>
#include <QJsonArray>
#include <QImage>
#include <QMenu>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QThread>
#include <QTest>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include "core/palette.hpp"
#include "core/plugin_manager.hpp"
#include "core/python_plugin_loader.hpp"
#include "core/python_runtime.hpp"
#include "core/lua_plugin_loader.hpp"
#include "core/update_manager.hpp"
#include "core/notifications.hpp"
#include "core/workspace_engine.hpp"
#include "ui/flowdeck_controller.hpp"

namespace {
QFile* smokeLog = nullptr;
QtMessageHandler previousMessageHandler = nullptr;
void logMessage(QtMsgType type, const QMessageLogContext& context, const QString& value) {
    if (smokeLog) { smokeLog->write(value.toUtf8()+"\n"); smokeLog->flush(); }
    if (previousMessageHandler) previousMessageHandler(type,context,value);
}
void stage(const QString& message) {
    if (smokeLog) { smokeLog->write(message.toUtf8()+"\n"); smokeLog->flush(); }
}
void closeLog() {
    if (smokeLog) { qInstallMessageHandler(previousMessageHandler); smokeLog = nullptr; }
}
class Hotkeys : public QAbstractNativeEventFilter {
 public:
    Hotkeys(flowdeck::FlowDeckController* controller, QQuickWindow* manager,
            QQuickWindow* palette) : controller_(controller), manager_(manager), palette_(palette) {}
    void registerAll() {
        unregisterAll();
        const auto registerKey = [this](int id, const QString& value) {
            const auto hotkey = value.toUpper();
            if (hotkey.isEmpty()) return;
            UINT mod = MOD_NOREPEAT;
            if (hotkey.contains("CTRL+")) mod |= MOD_CONTROL;
            if (hotkey.contains("ALT+")) mod |= MOD_ALT;
            if (hotkey.contains("SHIFT+")) mod |= MOD_SHIFT;
            const auto key = hotkey.section('+', -1);
            UINT vk = key == "SPACE" ? VK_SPACE : key == "ENTER" ? VK_RETURN :
                      key.size() == 1 ? static_cast<UINT>(key[0].unicode()) : 0;
            if (vk && RegisterHotKey(nullptr, id, mod, vk)) ids_.append(id);
            else flowdeck::notify("Hotkey unavailable: " + value);
        };
        registerKey(1, controller_->settings().value("paletteHotkey","Ctrl+Alt+Space").toString());
        const auto spaces = controller_->workspaces();
        for (int i = 0; i < spaces.size(); ++i) {
            registerKey(100+i, spaces[i].toMap().value("hotkey").toString());
        }
    }
    void unregisterAll() { for (int id : ids_) UnregisterHotKey(nullptr, id); ids_.clear(); }
    bool nativeEventFilter(const QByteArray&, void* message, qintptr*) override {
        auto* msg = static_cast<MSG*>(message);
        if (msg->message != WM_HOTKEY) return false;
        if (msg->wParam == 1) {
            palette_->setVisible(!palette_->isVisible());
            if (palette_->isVisible()) { palette_->raise(); palette_->requestActivate(); }
        } else if (msg->wParam >= 100) {
            const int index = static_cast<int>(msg->wParam) - 100;
            controller_->selectWorkspace(index);
            if (controller_->selectedWorkspace().value("directApply").toBool()) {
                controller_->applySelected();
                if (controller_->prepared()) { manager_->show(); manager_->raise(); manager_->requestActivate(); }
            }
            else { manager_->show(); manager_->raise(); manager_->requestActivate(); }
        }
        return true;
    }
 private:
    flowdeck::FlowDeckController* controller_;
    QQuickWindow* manager_;
    QQuickWindow* palette_;
    QVector<int> ids_;
};
}

int main(int argc, char** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    QApplication app(argc, argv);
    QQuickStyle::setStyle("Basic");
    QFile diagnostic(qEnvironmentVariable("FLOWDECK_SMOKE_LOG"));
    if (!diagnostic.fileName().isEmpty() && diagnostic.open(QIODevice::WriteOnly)) {
        smokeLog = &diagnostic;
        previousMessageHandler = qInstallMessageHandler(logMessage);
    }
    stage("FlowDeck starting");
    app.setOrganizationName("FlowDeck");
    app.setApplicationName("FlowDeck");
    app.setQuitOnLastWindowClosed(false);
    const auto pluginDir = (std::filesystem::path(QCoreApplication::applicationDirPath().toStdWString()) / L"plugins").wstring();
    auto& paletteCore = flowdeck::Palette::Instance();
    auto& native = flowdeck::PluginManager::Instance();
    native.LoadAll(pluginDir); native.RegisterCommands(paletteCore);
    bool pythonReady = false;
    if (flowdeck::RegisterHostModule() && flowdeck::python::Initialise(pluginDir)) {
        auto& py = flowdeck::PythonPluginLoader::Instance();
        py.LoadAll(pluginDir); py.RegisterCommands(paletteCore); pythonReady = true;
    }
    flowdeck::LuaPluginLoader::instance().loadAll(pluginDir);
    stage(QString("Plugins: Python=%1 Lua=%2").arg(pythonReady).arg(flowdeck::LuaPluginLoader::instance().count()));
    flowdeck::FlowDeckController controller;
    flowdeck::SetHostTileCallback([&controller](const std::string& preset) {
        controller.runCommand("workspace:" + QString::fromStdString(preset));
    });
    flowdeck::UpdateManager updater;
    updater.setLanguage(controller.language());
    QString updateChannel = controller.settings().value("channel", "preview").toString();
    QObject::connect(&controller, &flowdeck::FlowDeckController::settingsChanged,
                     &updater, [&controller, &updater, &updateChannel] {
        updater.setLanguage(controller.language());
        const auto next = controller.settings().value("channel", "preview").toString();
        if (next != updateChannel) { updateChannel = next; updater.check(next); }
    });
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("flowdeck", &controller);
    engine.rootContext()->setContextProperty("updater", &updater);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Manager.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/Palette.qml")));
    stage(QString("QML roots: %1").arg(engine.rootObjects().size()));
    if (engine.rootObjects().size() != 2) { closeLog(); return 1; }
    auto* manager = qobject_cast<QQuickWindow*>(engine.rootObjects()[0]);
    auto* palette = qobject_cast<QQuickWindow*>(engine.rootObjects()[1]);
    if (!manager || !palette) { stage("QML root types invalid"); closeLog(); return 1; }
    QObject::connect(&controller, &flowdeck::FlowDeckController::requestManager,
                     manager, [manager] { manager->show(); manager->raise(); manager->requestActivate(); });
    Hotkeys hotkeys(&controller, manager, palette);
    app.installNativeEventFilter(&hotkeys);
    QObject::connect(&controller, &flowdeck::FlowDeckController::hotkeysChanged,
                     &app, [&hotkeys] { hotkeys.registerAll(); });
    hotkeys.registerAll();
    QSystemTrayIcon tray(QIcon(":/icons/flowdeck.svg"));
    tray.setToolTip("FlowDeck");
    QMenu trayMenu;
    QObject::connect(trayMenu.addAction("Open FlowDeck"), &QAction::triggered, manager,
                     [manager] { manager->show(); manager->raise(); manager->requestActivate(); });
    QObject::connect(trayMenu.addAction("Quit"), &QAction::triggered, &app, &QCoreApplication::quit);
    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, manager,
                     [manager](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                             manager->show(); manager->raise(); manager->requestActivate();
                         }
                     });
    tray.show();
    flowdeck::setNotificationTray(&tray);
    if (app.arguments().contains("--smoke-test")) {
        app.processEvents();
        QThread::msleep(250);
        app.processEvents();
        const auto screenshot = qEnvironmentVariable("FLOWDECK_SMOKE_SCREENSHOT");
        if (!screenshot.isEmpty()) {
            const bool saved = manager->grabWindow().save(screenshot);
            stage(QString("UI screenshot saved=%1").arg(saved));
        }
        bool navigationPassed = true;
        if (app.arguments().contains("--ui-test")) {
            const auto findItem = [](auto&& self, QQuickItem* parent, const QString& name) -> QQuickItem* {
                if (!parent) return nullptr;
                if (parent->objectName() == name) return parent;
                for (auto* child : parent->childItems()) {
                    if (auto* found = self(self, child, name)) return found;
                }
                return nullptr;
            };
            const auto click = [&](int index) {
                auto* item = findItem(findItem, manager->contentItem(), "navigation-"+QString::number(index));
                if (!item) { stage(QString("Navigation %1 not found").arg(index)); return false; }
                const auto point = item->mapToScene(QPointF(item->width()/2,item->height()/2));
                QTest::mouseClick(manager,Qt::LeftButton,Qt::NoModifier,point.toPoint());
                app.processEvents();
                const auto page = manager->property("page").toInt();
                stage(QString("Navigation %1 -> %2 at %3,%4").arg(index).arg(page).arg(point.x()).arg(point.y()));
                return page == index;
            };
            navigationPassed = click(3) && click(1) && click(0);
            stage(QString("UI navigation=%1").arg(navigationPassed));
        }
        flowdeck::Workspace geometry;
        geometry.canvasMode = "16:9";
        geometry.canvasHeight = 1440;
        const auto fitted = flowdeck::WorkspaceEngine::fitCanvas(QRect(0,0,3440,1400),geometry);
        const bool geometryPassed = fitted.width() == 2489 && fitted.height() == 1400 &&
                                    fitted.x() == 475 && fitted.y() == 0 &&
                                    flowdeck::WorkspaceEngine::fitCanvas(QRect(-1920,-100,1920,1040),geometry)
                                        .intersected(QRect(-1920,-100,1920,1040)) ==
                                    flowdeck::WorkspaceEngine::fitCanvas(QRect(-1920,-100,1920,1040),geometry);
        const auto& commands = paletteCore.All();
        const auto has = [&](const std::string& id) {
            return std::any_of(commands.begin(), commands.end(),
                [&](const flowdeck::Command& c) { return c.id == id; });
        };
        const bool passed = geometryPassed && navigationPassed && manager->isVisible() && pythonReady &&
                            !controller.searchCommands("lua").isEmpty() &&
                            has("example-hello:hello") && has("example_cpp:hello") &&
                            has("example-lua:hello") &&
                            paletteCore.ExecuteById("example-hello:hello") &&
                            paletteCore.ExecuteById("example_cpp:hello") &&
                            paletteCore.ExecuteById("example-lua:hello");
        stage(QString("Smoke geometry=%1 UI=%2 Python=%3 Cpp=%4 Lua=%5")
              .arg(geometryPassed).arg(manager->isVisible()).arg(pythonReady)
              .arg(has("example_cpp:hello")).arg(flowdeck::LuaPluginLoader::instance().count()));
        std::cout << (passed ? "[smoke] passed\n" : "[smoke] failed\n");
        hotkeys.unregisterAll();
        flowdeck::setNotificationTray(nullptr);
        flowdeck::LuaPluginLoader::instance().unloadAll();
        flowdeck::PythonPluginLoader::Instance().UnloadAll();
        flowdeck::python::Shutdown();
        if (SUCCEEDED(comResult)) CoUninitialize();
        closeLog();
        return passed ? 0 : 1;
    }
    if (controller.store().lastSession().value("windows").toArray().isEmpty())
        controller.store().beginAutosave();
    QTimer::singleShot(1000, &updater, [&updater, &controller] {
        updater.check(controller.settings().value("channel").toString());
    });
    QTimer::singleShot(10000, &app, [] {
        QFile::remove(qEnvironmentVariable("LOCALAPPDATA") + "/FlowDeck/updates/previous-installer.exe");
    });
    const int result = app.exec();
    hotkeys.unregisterAll();
    flowdeck::setNotificationTray(nullptr);
    flowdeck::LuaPluginLoader::instance().unloadAll();
    flowdeck::PythonPluginLoader::Instance().UnloadAll();
    flowdeck::python::Shutdown();
    if (SUCCEEDED(comResult)) CoUninitialize();
    closeLog();
    return result;
}
