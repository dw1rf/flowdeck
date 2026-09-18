#include "core/window_tiler.hpp"

#include <windows.h>

#include <iostream>

namespace flowdeck {

namespace {

struct Layout {
    LONG x, y, w, h;
};

// Compute a layout for the foreground window given a preset.
// Real implementation will enumerate all windows and tile them.
Layout PresetLayout(const std::string& name) {
    RECT screen{};
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);
    const LONG sw = screen.right - screen.left;
    const LONG sh = screen.bottom - screen.top;

    if (name == "coding") {
        // IDE 70% left, browser 30% right (browser handled by caller).
        return {screen.left, screen.top, static_cast<LONG>(sw * 0.7), sh};
    }
    if (name == "trading") {
        // Three vertical strips — caller will iterate.
        return {screen.left, screen.top, sw / 3, sh};
    }
    // "chill" or default: 16:9 centered.
    const LONG w = static_cast<LONG>(sw * 0.7);
    const LONG h = static_cast<LONG>(w * 9 / 16);
    return {(sw - w) / 2, (sh - h) / 2, w, h};
}

BOOL CALLBACK TileCallback(HWND hwnd, LPARAM lp) {
    auto* name = reinterpret_cast<const std::string*>(lp);
    if (!IsWindowVisible(hwnd)) return TRUE;

    // For now, just tile the foreground window.
    if (hwnd != GetForegroundWindow()) return TRUE;

    const Layout l = PresetLayout(*name);
    MoveWindow(hwnd, l.x, l.y, l.w, l.h, TRUE);
    return TRUE;
}

}  // namespace

WindowTiler& WindowTiler::Instance() {
    static WindowTiler instance;
    return instance;
}

void WindowTiler::ApplyPreset(const std::string& name) {
    std::cout << "[tiler] applying preset: " << name << "\n";
    EnumWindows(TileCallback, reinterpret_cast<LPARAM>(&name));
}

}  // namespace flowdeck
