#pragma once

#include <windows.h>

#include <list>
#include <string>

#include "core/palette.hpp"
#include "plugin_api.h"

namespace flowdeck {

// Loads plugins from a directory. Each plugin is a DLL implementing
// the FlowDeck Plugin ABI (see plugin_api.h).
class PluginManager {
 public:
    static PluginManager& Instance();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Scan a directory and load every plugin DLL found.
    void LoadAll(const std::wstring& dir);

    // Register all loaded plugin commands with the palette.
    void RegisterCommands(Palette& palette);

 private:
    PluginManager() = default;
    ~PluginManager();

    struct LoadedPlugin {
        HMODULE module = nullptr;
        FlowDeckPluginContext ctx{};
        FlowDeckPluginVTable vtable{};
        std::string name;
    };

    std::list<LoadedPlugin> plugins_;
};

}  // namespace flowdeck
