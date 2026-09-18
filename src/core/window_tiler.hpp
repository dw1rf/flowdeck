#pragma once

#include <string>
#include <vector>

namespace flowdeck {

// Tiles real top-level windows via Win32.
class WindowTiler {
 public:
    static WindowTiler& Instance();

    WindowTiler(const WindowTiler&) = delete;
    WindowTiler& operator=(const WindowTiler&) = delete;

    // Apply a named preset: "coding", "trading", "chill", "grid".
    // Unknown names fall back to "grid".
    void ApplyPreset(const std::string& name);

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
