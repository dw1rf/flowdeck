#include "core/notifications.hpp"
#include <QSystemTrayIcon>
#include <QStringList>
namespace flowdeck {
namespace { QSystemTrayIcon* activeTray = nullptr; QStringList pending; }
void notify(const QString& message) {
    if (activeTray) activeTray->showMessage("FlowDeck",message,QSystemTrayIcon::Information,4000);
    else pending.append(message);
}
void setNotificationTray(QSystemTrayIcon* tray) {
    activeTray = tray;
    if (tray) for (const auto& message : pending) notify(message);
    pending.clear();
}
}
