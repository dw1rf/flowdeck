#pragma once

#include <string>
#include <vector>

namespace flowdeck {

struct TileRect {
    int x, y, w, h;
};

struct WindowPlacement {
    std::wstring title;
    TileRect rect;
};

struct LayoutPreview {
    TileRect work_area;
    std::vector<WindowPlacement> windows;
    size_t omitted = 0;
};

// Tiles real top-level windows via Win32.
class WindowTiler {
 public:
    static WindowTiler& Instance();

    WindowTiler(const WindowTiler&) = delete;
    WindowTiler& operator=(const WindowTiler&) = delete;

    // Apply a named preset: "coding", "trading", "chill", "grid".
    // Unknown names fall back to "grid".
    void ApplyPreset(const std::string& name);

    // Uses the same window order, work area and slot builder as ApplyPreset.
    LayoutPreview PreviewPreset(const std::string& name) const;

    // Titles of the windows the tiler currently considers tileable.
    std::vector<std::wstring> ListTileableWindows() const;

    // Max windows a preset will arrange (extras are left alone).
    void set_max_windows(size_t n) { max_windows_ = n; }
    size_t max_windows() const { return max_windows_; }

 private:
    WindowTiler() = default;

    size_t max_windows_ = 6;
};

}  // namespace flowdeck
