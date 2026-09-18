#include "core/hotkey_manager.hpp"

#include <windows.h>

namespace flowdeck {

HotkeyManager& HotkeyManager::Instance() {
    static HotkeyManager instance;
    return instance;
}

bool HotkeyManager::Register(HWND target, UINT id, UINT modifiers, UINT vk) {
    if (!RegisterHotKey(target, id, modifiers, vk)) {
        return false;
    }
    entries_[id] = Entry{id};
    return true;
}

void HotkeyManager::Unregister(UINT id) {
    auto it = entries_.find(id);
    if (it == entries_.end()) return;
    UnregisterHotKey(nullptr, id);
    entries_.erase(it);
}

void HotkeyManager::UnregisterAll() {
    for (const auto& [id, _] : entries_) {
        UnregisterHotKey(nullptr, id);
    }
    entries_.clear();
}

HotkeyManager::~HotkeyManager() { UnregisterAll(); }

}  // namespace flowdeck
