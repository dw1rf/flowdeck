#pragma once

#include <windows.h>

#include <unordered_map>

namespace flowdeck {

// Registers global hotkeys and routes them to a target window.
class HotkeyManager {
 public:
    static HotkeyManager& Instance();

    HotkeyManager(const HotkeyManager&) = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;

    // Register a global hotkey. Returns true on success.
    bool Register(HWND target, UINT id, UINT modifiers, UINT vk);

    // Unregister a single hotkey by id.
    void Unregister(UINT id);

    // Unregister all hotkeys.
    void UnregisterAll();

 private:
    HotkeyManager() = default;
    ~HotkeyManager();

    struct Entry {
        UINT id{};
    };
    std::unordered_map<UINT, Entry> entries_;
};

}  // namespace flowdeck
