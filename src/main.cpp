// FlowDeck — entry point.
// Starts the Win32 message loop, registers global hotkeys,
// loads plugins, and dispatches palette commands.

#include <windows.h>

#include <iostream>

#include "core/hotkey_manager.hpp"
#include "core/palette.hpp"
#include "core/plugin_manager.hpp"
#include "core/window_tiler.hpp"

namespace {

constexpr UINT kToggleHotkeyId = 0x0001;
constexpr UINT kTileCodingHotkeyId = 0x0002;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_HOTKEY: {
            switch (LOWORD(wp)) {
                case kToggleHotkeyId:
                    flowdeck::Palette::Instance().Toggle();
                    break;
                case kTileCodingHotkeyId:
                    flowdeck::WindowTiler::Instance().ApplyPreset("coding");
                    break;
            }
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

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int) {
    // Console for early debugging — remove for release builds.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    }

    std::wcout << L"[FlowDeck] starting...\n";

    HWND hwnd = CreateMessageWindow(inst);
    if (!hwnd) {
        std::wcerr << L"[FlowDeck] failed to create message window\n";
        return 1;
    }

    auto& hotkeys = flowdeck::HotkeyManager::Instance();
    hotkeys.Register(hwnd, kToggleHotkeyId, MOD_CONTROL | MOD_ALT, VK_SPACE);
    hotkeys.Register(hwnd, kTileCodingHotkeyId, MOD_CONTROL | MOD_SHIFT, 'T');

    auto& plugins = flowdeck::PluginManager::Instance();
    plugins.LoadAll(L"plugins");
    plugins.RegisterCommands(flowdeck::Palette::Instance());

    std::wcout << L"[FlowDeck] ready. Ctrl+Alt+Space = palette, Ctrl+Shift+T = tile coding.\n";

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hotkeys.UnregisterAll();
    return static_cast<int>(msg.wParam);
}
