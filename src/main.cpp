#include <windows.h>
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include "core/palette.hpp"
#include "core/plugin_manager.hpp"
#include "core/python_plugin_loader.hpp"
#include "core/python_runtime.hpp"
#include "core/lua_plugin_loader.hpp"
#include "ui/flowdeck_controller.hpp"

namespace {
class Hotkeys : public QAbstractNativeEventFilter {
 public:
    Hotkeys(flowdeck::FlowDeckController* controller, QQuickWindow* manager,
            QQuickWindow* palette) : controller_(controller), manager_(manager), palette_(palette) {}
    void registerAll() {
        unregisterAll();
        if (RegisterHotKey(nullptr, 1, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_SPACE)) ids_.append(1);
        const auto spaces = controller_->workspaces();
        for (int i = 0; i < spaces.size(); ++i) {
            const QString value = spaces[i].toMap().value("hotkey").toString().toUpper();
            if (value.isEmpty()) continue;
            UINT mod = MOD_NOREPEAT;
            if (value.contains("CTRL+")) mod |= MOD_CONTROL;
            if (value.contains("ALT+")) mod |= MOD_ALT;
            if (value.contains("SHIFT+")) mod |= MOD_SHIFT;
            const auto key = value.section('+', -1);
            UINT vk = key == "SPACE" ? VK_SPACE : key == "ENTER" ? VK_RETURN :
                      key.size() == 1 ? static_cast<UINT>(key[0].unicode()) : 0;
            if (vk && RegisterHotKey(nullptr, 100 + i, mod, vk)) ids_.append(100 + i);
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
            if (controller_->selectedWorkspace().value("directApply").toBool()) controller_->applySelected();
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
    QApplication app(argc, argv);
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
    flowdeck::FlowDeckController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("flowdeck", &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Manager.qml")));
    engine.load(QUrl(QStringLiteral("qrc:/qml/Palette.qml")));
    if (engine.rootObjects().size() != 2) return 1;
    auto* manager = qobject_cast<QQuickWindow*>(engine.rootObjects()[0]);
    auto* palette = qobject_cast<QQuickWindow*>(engine.rootObjects()[1]);
    if (!manager || !palette) return 1;
    QObject::connect(&controller, &flowdeck::FlowDeckController::requestManager,
                     manager, [manager] { manager->show(); manager->raise(); manager->requestActivate(); });
    Hotkeys hotkeys(&controller, manager, palette);
    app.installNativeEventFilter(&hotkeys);
    QObject::connect(&controller, &flowdeck::FlowDeckController::hotkeysChanged,
                     &app, [&hotkeys] { hotkeys.registerAll(); });
    hotkeys.registerAll();
    QSystemTrayIcon tray(QIcon(":/icons/flowdeck.svg"));
    tray.setToolTip("FlowDeck");
    QObject::connect(&tray, &QSystemTrayIcon::activated, manager,
                     [manager](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                             manager->show(); manager->raise(); manager->requestActivate();
                         }
                     });
    tray.show();
    if (app.arguments().contains("--smoke-test")) {
        const auto& commands = paletteCore.All();
        const auto has = [&](const std::string& id) {
            return std::any_of(commands.begin(), commands.end(),
                [&](const flowdeck::Command& c) { return c.id == id; });
        };
        const bool passed = manager->isVisible() && pythonReady &&
                            has("example-hello:hello") && has("example_cpp:hello") &&
                            paletteCore.ExecuteById("example-hello:hello") &&
                            paletteCore.ExecuteById("example_cpp:hello");
        std::cout << (passed ? "[smoke] passed\n" : "[smoke] failed\n");
        hotkeys.unregisterAll();
        flowdeck::LuaPluginLoader::instance().unloadAll();
        flowdeck::PythonPluginLoader::Instance().UnloadAll();
        flowdeck::python::Shutdown();
        return passed ? 0 : 1;
    }
    controller.store().beginAutosave();
    const int result = app.exec();
    hotkeys.unregisterAll();
    flowdeck::LuaPluginLoader::instance().unloadAll();
    flowdeck::PythonPluginLoader::Instance().UnloadAll();
    flowdeck::python::Shutdown();
    return result;
}
