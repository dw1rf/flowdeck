#include "core/window_tiler.hpp"

#include <windows.h>
#include <dwmapi.h>

#include <cmath>
#include <iostream>

#pragma comment(lib, "dwmapi.lib")

namespace flowdeck {

namespace {

using Slot = TileRect;

struct WindowInfo {
    HWND hwnd;
    std::wstring title;
};

// Filter out tool windows, cloaked UWP shells and the desktop itself.
bool IsTileable(HWND hwnd) {
    if (!IsWindowVisible(hwnd)) return false;
    if (IsIconic(hwnd)) return false;
    if (GetWindow(hwnd, GW_OWNER) != nullptr) return false;

    const LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (ex & WS_EX_TOOLWINDOW) return false;
    if (ex & WS_EX_NOACTIVATE) return false;

    // Skip windows cloaked by DWM (background UWP apps).
    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked,
                                        sizeof(cloaked))) &&
        cloaked != 0) {
        return false;
    }

    wchar_t title[256] = {};
    if (GetWindowTextW(hwnd, title, 256) == 0) return false;

    RECT r{};
    if (!GetWindowRect(hwnd, &r)) return false;
    if ((r.right - r.left) < 120 || (r.bottom - r.top) < 80) return false;

    return true;
}

BOOL CALLBACK CollectProc(HWND hwnd, LPARAM lp) {
    auto* out = reinterpret_cast<std::vector<WindowInfo>*>(lp);
    if (!IsTileable(hwnd)) return TRUE;

    wchar_t title[256] = {};
    GetWindowTextW(hwnd, title, 256);
    out->push_back(WindowInfo{hwnd, title});
    return TRUE;
}

std::vector<WindowInfo> CollectWindows() {
    std::vector<WindowInfo> out;
    EnumWindows(CollectProc, reinterpret_cast<LPARAM>(&out));
    return out;
}

Slot WorkArea() {
    RECT r{};
    SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
    return {r.left, r.top, r.right - r.left, r.bottom - r.top};
}

std::vector<Slot> BuildGrid(const Slot& area, int n) {
    std::vector<Slot> slots;
    const int cols = static_cast<int>(std::ceil(std::sqrt(double(n))));
    const int rows = static_cast<int>(std::ceil(double(n) / cols));
    const int cw = area.w / cols;
    const int ch = area.h / rows;
    for (int i = 0; i < n; ++i) {
        const int r = i / cols;
        const int c = i % cols;
        slots.push_back({area.x + c * cw, area.y + r * ch, cw, ch});
    }
    return slots;
}

std::vector<Slot> BuildPreset(const std::string& name, const Slot& area,
                              int n) {
    std::vector<Slot> slots;
    if (n <= 0) return slots;

    if (name == "coding") {
        const int main_w = area.w * 7 / 10;
        if (n == 1) {
            slots.push_back(area);
            return slots;
        }
        slots.push_back({area.x, area.y, main_w, area.h});
        const int side_x = area.x + main_w;
        const int side_w = area.w - main_w;
        const int each = area.h / (n - 1);
        for (int i = 1; i < n; ++i) {
            slots.push_back({side_x, area.y + (i - 1) * each, side_w, each});
        }
        return slots;
    }

    if (name == "trading") {
        const int each = area.w / n;
        for (int i = 0; i < n; ++i) {
            slots.push_back({area.x + i * each, area.y, each, area.h});
        }
        return slots;
    }

    if (name == "chill") {
        const int main_w = n == 1 ? area.w : area.w * 3 / 4;
        const int main_h = main_w * 9 / 16;
        const int y = area.y + (area.h - main_h) / 2;
        slots.push_back({area.x, y > area.y ? y : area.y, main_w,
                         main_h < area.h ? main_h : area.h});
        if (n > 1) {
            const int side_x = area.x + main_w;
            const int side_w = area.w - main_w;
            const int each = area.h / (n - 1);
            for (int i = 1; i < n; ++i) {
                slots.push_back(
                    {side_x, area.y + (i - 1) * each, side_w, each});
            }
        }
        return slots;
    }

    return BuildGrid(area, n);
}

}  // namespace

WindowTiler& WindowTiler::Instance() {
    static WindowTiler instance;
    return instance;
}

std::vector<std::wstring> WindowTiler::ListTileableWindows() const {
    std::vector<std::wstring> titles;
    for (const auto& w : CollectWindows()) titles.push_back(w.title);
    return titles;
}

void WindowTiler::ApplyPreset(const std::string& name) {
    auto windows = CollectWindows();
    if (windows.empty()) {
        std::cout << "[tiler] nothing to tile\n";
        return;
    }
    if (windows.size() > max_windows_) windows.resize(max_windows_);

    const Slot area = WorkArea();
    const auto slots =
        BuildPreset(name, area, static_cast<int>(windows.size()));

    std::cout << "[tiler] preset '" << name << "' on " << windows.size()
              << " window(s)\n";

    for (size_t i = 0; i < windows.size() && i < slots.size(); ++i) {
        const Slot& s = slots[i];
        // Restore first so a maximised window can actually be moved.
        ShowWindow(windows[i].hwnd, SW_RESTORE);
        SetWindowPos(windows[i].hwnd, nullptr, s.x, s.y, s.w, s.h,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

LayoutPreview WindowTiler::PreviewPreset(const std::string& name) const {
    auto windows = CollectWindows();
    LayoutPreview preview{};
    preview.work_area = WorkArea();
    if (windows.size() > max_windows_) {
        preview.omitted = windows.size() - max_windows_;
        windows.resize(max_windows_);
    }
    const auto slots = BuildPreset(name, preview.work_area,
                                   static_cast<int>(windows.size()));
    for (size_t i = 0; i < windows.size() && i < slots.size(); ++i) {
        preview.windows.push_back({windows[i].title, slots[i]});
    }
    return preview;
}

}  // namespace flowdeck
