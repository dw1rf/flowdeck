#include "core/plugin_manager.hpp"

#include <windows.h>

#include <filesystem>
#include <iostream>

namespace flowdeck {

namespace fs = std::filesystem;

namespace {

// Bridge: plugin calls ctx.notify -> forwards to console (for now).
void FLAPI notify_bridge(const char* msg) {
    std::cout << "[plugin] " << msg << "\n";
}

}  // namespace

PluginManager& PluginManager::Instance() {
    static PluginManager instance;
    return instance;
}

void PluginManager::LoadAll(const std::wstring& dir) {
    if (!fs::exists(dir)) {
        std::wcout << L"[plugins] directory not found: " << dir << L"\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != L".dll") continue;

        HMODULE mod = LoadLibraryW(entry.path().c_str());
        if (!mod) {
            std::wcerr << L"[plugins] failed to load " << entry.path() << L"\n";
            continue;
        }

        auto get_vtable = reinterpret_cast<FlowDeckPluginGetVTable>(
            GetProcAddress(mod, "FlowDeckPluginGetVTable"));
        if (!get_vtable) {
            std::wcerr << L"[plugins] missing FlowDeckPluginGetVTable in "
                       << entry.path() << L"\n";
            FreeLibrary(mod);
            continue;
        }

        FlowDeckPluginVTable vt = get_vtable();
        LoadedPlugin lp{};
        lp.module = mod;
        lp.vtable = vt;
        lp.ctx.notify = notify_bridge;
        lp.name = entry.path().stem().string();

        if (vt.on_load) vt.on_load(&lp.ctx);
        plugins_.push_back(std::move(lp));
        std::wcout << L"[plugins] loaded " << entry.path() << L"\n";
    }
}

void PluginManager::RegisterCommands(Palette& palette) {
    for (const auto& p : plugins_) {
        if (!p.vtable.get_commands) continue;
        const FlowDeckCommand* cmds = nullptr;
        int count = p.vtable.get_commands(&cmds);
        for (int i = 0; i < count; ++i) {
            palette.AddCommand({
                /*id=*/std::string(p.name) + ":" + cmds[i].id,
                /*title=*/std::string(cmds[i].title),
                /*hint=*/std::string("plugin: ") + p.name,
                /*run=*/[p, i] {
                    if (p.vtable.run_command) {
                        p.vtable.run_command(const_cast<FlowDeckCommand*>(&cmds[i]));
                    }
                },
            });
        }
    }
}

PluginManager::~PluginManager() {
    for (auto& p : plugins_) {
        if (p.vtable.on_unload) p.vtable.on_unload();
        if (p.module) FreeLibrary(p.module);
    }
}

}  // namespace flowdeck
