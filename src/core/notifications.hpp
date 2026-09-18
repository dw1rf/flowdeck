#pragma once
#include <QString>
class QSystemTrayIcon;
namespace flowdeck {
void notify(const QString& message);
void setNotificationTray(QSystemTrayIcon* tray);
}
