// Example FlowDeck plugin in C++.
// Builds to a DLL that the host loads via the C ABI.

#include "plugin_api.h"

#include <cstring>

static FlowDeckPluginContext* g_ctx = nullptr;

static FlowDeckCommand kCommands[] = {
    {"hello", "Say Hello (C++)", "Ctrl+Shift+H"},
    {"tile-coding", "Tile: Coding (C++)", nullptr},
};

static void on_load(FlowDeckPluginContext* ctx) {
    g_ctx = ctx;
    if (g_ctx && g_ctx->notify) {
        g_ctx->notify("example-cpp loaded");
    }
}

static void on_unload(void) {
    if (g_ctx && g_ctx->notify) {
        g_ctx->notify("example-cpp unloaded");
    }
    g_ctx = nullptr;
}

static int get_commands(const FlowDeckCommand** out) {
    *out = kCommands;
    return sizeof(kCommands) / sizeof(kCommands[0]);
}

static void run_command(FlowDeckCommand* cmd) {
    if (!g_ctx || !g_ctx->notify) return;
    if (std::strcmp(cmd->id, "hello") == 0) {
        g_ctx->notify("Hello from the C++ plugin!");
    } else if (std::strcmp(cmd->id, "tile-coding") == 0) {
        g_ctx->notify("(would tile coding preset)");
    }
}

static FlowDeckPluginVTable build_vtable(void) {
    FlowDeckPluginVTable vt{};
    vt.on_load = on_load;
    vt.on_unload = on_unload;
    vt.get_commands = get_commands;
    vt.run_command = run_command;
    return vt;
}

extern "C" __declspec(dllexport) FlowDeckPluginVTable FlowDeckPluginGetVTable(void) {
    return build_vtable();
}
