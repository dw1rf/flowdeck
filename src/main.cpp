// FlowDeck — entry point.
//
// Boots the embedded CPython interpreter, loads native (.dll) and Python
// plugins, registers global hotkeys and pumps the Win32 message loop.

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "core/hotkey_manager.hpp"
#include "core/palette.hpp"
#include "core/plugin_manager.hpp"
#include "core/python_plugin_loader.hpp"
#include "core/python_runtime.hpp"
#include "core/window_tiler.hpp"

namespace fs = std::filesystem;

namespace {

constexpr UINT kHotkeyPalette = 1;
constexpr UINT kHotkeyTileCoding = 2;
constexpr UINT kHotkeyTileTrading = 3;
constexpr UINT kHotkeyRunSelected = 4;

std::wstring ExeDirectory() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().wstring();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_HOTKEY:
            switch (LOWORD(wp)) {
                case kHotkeyPalette:
                    flowdeck::Palette::Instance().Toggle();
                    return 0;
                case kHotkeyTileCoding:
                    flowdeck::WindowTiler::Instance().ApplyPreset("coding");
                    return 0;
                case kHotkeyTileTrading:
                    flowdeck::WindowTiler::Instance().ApplyPreset("trading");
                    return 0;
                case kHotkeyRunSelected:
                    flowdeck::Palette::Instance().ExecuteSelected();
                    return 0;
                default:
                    return 0;
            }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
}

HWND CreateMessageWindow(HINSTANCE inst) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"FlowDeckMsgWindow";
    RegisterClassW(&wc);
    return CreateWindowExW(0, L"FlowDeckMsgWindow", L"FlowDeck", 0, 0, 0, 0, 0,
                           HWND_MESSAGE, nullptr, inst, nullptr);
}

void AttachConsoleIfPossible() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) AllocConsole();
    FILE* dummy = nullptr;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int) {
    AttachConsoleIfPossible();
    std::cout << "[FlowDeck] starting\n";

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    const bool smoke_test = argv && argc == 2 &&
                            std::wstring(argv[1]) == L"--smoke-test";
    if (argv) LocalFree(argv);

    const std::wstring plugin_dir = ExeDirectory() + L"\\plugins";

    HWND hwnd = CreateMessageWindow(inst);
    if (!hwnd) {
        std::cerr << "[FlowDeck] cannot create message window\n";
        return 1;
    }

    auto& palette = flowdeck::Palette::Instance();

    // Built-in commands — available even with zero plugins installed.
    palette.AddCommand({"core:tile-coding", "Tile: Coding", "core",
                        [] { flowdeck::WindowTiler::Instance().ApplyPreset("coding"); }});
    palette.AddCommand({"core:tile-trading", "Tile: Trading", "core",
                        [] { flowdeck::WindowTiler::Instance().ApplyPreset("trading"); }});
    palette.AddCommand({"core:tile-chill", "Tile: Chill", "core",
                        [] { flowdeck::WindowTiler::Instance().ApplyPreset("chill"); }});
    palette.AddCommand({"core:tile-grid", "Tile: Grid", "core",
                        [] { flowdeck::WindowTiler::Instance().ApplyPreset("grid"); }});

    // Native DLL plugins.
    auto& native = flowdeck::PluginManager::Instance();
    native.LoadAll(plugin_dir);
    native.RegisterCommands(palette);

    // Python plugins.
    if (flowdeck::RegisterHostModule() &&
        flowdeck::python::Initialise(plugin_dir)) {
        auto& py = flowdeck::PythonPluginLoader::Instance();
        py.LoadAll(plugin_dir);
        py.RegisterCommands(palette);
    } else {
        std::cerr << "[FlowDeck] Python unavailable — native plugins only\n";
    }

    if (smoke_test) {
        const auto& commands = palette.All();
        const auto has = [&](const std::string& id) {
            return std::any_of(commands.begin(), commands.end(),
                               [&](const flowdeck::Command& command) {
                                   return command.id == id;
                               });
        };
        const bool loaded = flowdeck::python::IsReady() &&
                            has("example-hello:hello") &&
                            has("example_cpp:hello");
        const bool ran = loaded &&
                         palette.ExecuteById("example-hello:hello") &&
                         palette.ExecuteById("example_cpp:hello");
        std::cout << (ran ? "[smoke] passed\n" : "[smoke] failed\n");
        flowdeck::PythonPluginLoader::Instance().UnloadAll();
        flowdeck::python::Shutdown();
        DestroyWindow(hwnd);
        return ran ? 0 : 1;
    }

    auto& hotkeys = flowdeck::HotkeyManager::Instance();
    hotkeys.Register(hwnd, kHotkeyPalette, MOD_CONTROL | MOD_ALT, VK_SPACE);
    hotkeys.Register(hwnd, kHotkeyTileCoding, MOD_CONTROL | MOD_SHIFT, 'T');
    hotkeys.Register(hwnd, kHotkeyTileTrading, MOD_CONTROL | MOD_SHIFT, 'G');
    hotkeys.Register(hwnd, kHotkeyRunSelected, MOD_CONTROL | MOD_ALT, VK_RETURN);

    std::cout << "[FlowDeck] ready — " << palette.All().size()
              << " commands\n"
              << "  Ctrl+Alt+Space  toggle palette\n"
              << "  Ctrl+Alt+Enter  run highlighted command\n"
              << "  Ctrl+Shift+T    tile: coding\n"
              << "  Ctrl+Shift+G    tile: trading\n";

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hotkeys.UnregisterAll();
    flowdeck::PythonPluginLoader::Instance().UnloadAll();
    flowdeck::python::Shutdown();
    return static_cast<int>(msg.wParam);
}
