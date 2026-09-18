// FlowDeck Plugin ABI — stable C interface for plugins.
// Plugins are DLLs (.dll / .so / .dylib) exporting FlowDeckPluginGetVTable.
//
// Keeping this as a pure C header means plugins can be written in
// C, C++, Rust, Zig, or any language that can produce a C ABI —
// no C++ runtime coupling with the host.

#ifndef FLOWDECK_PLUGIN_API_H
#define FLOWDECK_PLUGIN_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FlowDeckPluginContext {
    // Called by plugins to send a notification to the user.
    void (*notify)(const char* message);
    // Reserved for future: clipboard, windows, process, network, fs.
    void* reserved[7];
} FlowDeckPluginContext;

typedef struct FlowDeckCommand {
    const char* id;     // stable command id, e.g. "hello"
    const char* title;  // display title, e.g. "Say Hello"
    const char* hotkey; // optional, e.g. "Ctrl+Shift+H", or NULL
} FlowDeckCommand;

typedef struct FlowDeckPluginVTable {
    // Called once when the plugin is loaded. ctx is owned by the host.
    void (*on_load)(FlowDeckPluginContext* ctx);

    // Called once when the plugin is unloaded.
    void (*on_unload)(void);

    // Returns a pointer to an array of commands and writes count.
    // The pointer must remain valid until on_unload.
    int (*get_commands)(const FlowDeckCommand** out_commands);

    // Run a command by pointer (one of the entries from get_commands).
    void (*run_command)(FlowDeckCommand* cmd);
} FlowDeckPluginVTable;

// Exported symbol a plugin DLL must provide.
typedef FlowDeckPluginVTable (*FlowDeckPluginGetVTable)(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // FLOWDECK_PLUGIN_API_H
