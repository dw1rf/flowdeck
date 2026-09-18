# FlowDeck

FlowDeck is a Windows workspace manager and command palette. It shows a preview before moving windows, saves the current window session locally, and loads Python, Lua, and native plugins.

## Install

Download `FlowDeck-Setup-x64.exe` and its `.sha256` file from [Releases](https://github.com/dw1rf/flowdeck/releases). The installer creates a Start menu shortcut and an uninstaller. The ZIP is available for portable testing. Both packages include Python 3.12, Qt, Lua, and the example plugins; no separate runtime installation is needed.

FlowDeck starts with a large manager window. Closing it leaves the app in the system tray. Right-click the tray icon to quit. `Ctrl+Alt+Space` opens the compact command palette. The default Coding and Trading shortcuts open their previews; you can change a workspace shortcut or explicitly enable immediate application in its editor.

## Workspaces

The **Spaces** view contains Coding, Trading, and Chill templates. Select one to see its zones at the selected monitor's actual available size. The **Editor** lets you drag and resize zones, choose a monitor, a full monitor or virtual 16:9 / 21:9 / custom canvas, gaps, a zone aspect ratio, and matching rules by executable, class, or title pattern. A canvas is fitted inside the monitor work area, so the taskbar is respected. The editor shows the fitted pixel size. On a 3440×1440 display with a 40 px taskbar, a 2560×1440 virtual 16:9 canvas is fitted to 2489×1400 and centered.

Assign a currently open window to each role in the preview. FlowDeck remembers the application and class rule. Other windows are left alone when applying a named workspace. If windows have changed since the preview was calculated, FlowDeck refreshes the preview and asks you to apply again. **Undo** restores the previous positions and minimized/maximized states.

Before and after actions may launch an EXE, run a PowerShell script, wait for a window, focus or minimize a window, or run a plugin command. They run in sequence and stop on failure. Imported workspaces with actions cannot execute them until you review and trust that workspace. These are trusted local actions, not a sandbox.

FlowDeck saves an automatic snapshot of the current user windows in `%LOCALAPPDATA%\FlowDeck\last-session.json`, separately from named workspaces. On startup it offers restoration. Launching applications that are no longer open is an optional checkbox in the restoration dialog. Settings and named spaces are saved in the same local directory, outside the installation folder.

## Plugins

Python plugins have `plugins/<name>/manifest.json` and `main.py`. [The Python example](plugins/example-hello/main.py) shows `COMMANDS`, `run_<id>`, and `flowdeck.notify()`. Command dictionaries may include `title_ru` and `title_en` for localized titles.

Lua 5.5.1 plugins use the same manifest format with `"entry": "main.lua"`. [The Lua example](plugins/example-lua/main.lua) shows the same command and notification API. Lua commands are declared in a global `COMMANDS` table and implemented as `run_<id>()`. Python, Lua, and native plugins run as trusted local code with the user's system permissions; review a plugin before installing it. Restart FlowDeck after plugin changes.

## Updates

The default Preview channel accepts `build-*` GitHub prereleases. Stable accepts `v*` releases. FlowDeck checks at startup and daily. It downloads the installer and verifies the SHA-256 checksum in the background; installation and restart require confirmation in the UI. The previous downloaded installer is kept in `%LOCALAPPDATA%\FlowDeck\updates` for rollback after later updates.

## Build

Install Visual Studio 2022 with Desktop development with C++, CMake 3.20+, Python 3.12 with development headers, and Qt 6.8.3 MSVC 2022 x64 (Qt Quick, Quick Controls, SVG). CMake downloads and builds Lua 5.5.1 from the official source archive.

```powershell
cmake -S . -B build -A x64 -DPython3_ROOT_DIR="C:/Python312" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
cmake --build build --config Release
./tools/package-windows.ps1
```

The package script downloads Python's embeddable distribution and runs `windeployqt`. CI also builds the Inno Setup installer, calculates its SHA-256, installs it in a temporary directory, and runs a packaged smoke test. Pull requests upload artifacts but do not publish releases. Successful pushes to `main` publish Preview releases; `v*` tags publish Stable releases.

Run `flowdeck.exe --smoke-test` to check Qt loading, Python/Lua/C++ commands, and the 3440×1440 canvas calculation without moving windows. Add `--ui-test` to click through the built-in screens without changing other windows. Interactive hotkeys and real window placement still require a desktop test.

## License

FlowDeck uses the [PolyForm Noncommercial License 1.0.0](LICENSE). It dynamically links Qt under LGPL terms, embeds CPython under the Python Software Foundation License, and includes Lua under the MIT License. Qt DLLs in the package can be replaced with compatible builds; see the upstream license notices accompanying those libraries.
