#pragma once

#include <string>

namespace flowdeck {

// Tiles windows using Win32 API. Presets are named layouts.
class WindowTiler {
 public:
    static WindowTiler& Instance();

    WindowTiler(const WindowTiler&) = delete;
    WindowTiler& operator=(const WindowTiler&) = delete;

    // Apply a named preset ("coding", "trading", "chill", ...).
    void ApplyPreset(const std::string& name);

 private:
    WindowTiler() = default;
};

}  // namespace flowdeck
