# FlowDeck

FlowDeck is an early Windows command palette and window tiler written in C++20. It loads native DLL plugins and embedded Python plugins.

## Download and try it

Download `FlowDeck-windows-x64.zip` from [Releases](https://github.com/dw1rf/flowdeck/releases). Extract the archive and run `FlowDeck-x64/flowdeck.exe`. The archive includes Python 3.12 and both example plugins; a separate Python installation is not needed.

| Shortcut | Action |
| --- | --- |
| Ctrl+Alt+Space | Open or close the command window |
| Ctrl+Alt+Enter | Apply the selected command while the window is open |
| Ctrl+Shift+T | Open the coding layout preview |
| Ctrl+Shift+G | Open the trading layout preview |

The launcher opens when FlowDeck starts. Type to search, use the arrow keys to choose a command, and press Enter or click the action button to run it. Selecting a layout shows the current windows in their proposed positions. No windows move until you apply the layout. Closing the launcher leaves FlowDeck in the system tray; right-click the tray icon to exit.

To check the downloaded package without entering the message loop, run `flowdeck.exe --smoke-test` from a terminal. It creates the launcher, loads both plugins, runs their greeting commands, then exits with code 0 on success. This check does not test interactive hotkeys or window placement.

Python plugin changes require an application restart. Manifest permissions are parsed but are not enforced.

## Build locally

Install Visual Studio 2022 with Desktop development with C++, CMake 3.20 or newer, and Python 3.12 with development headers and import library. Then run:

```powershell
cmake -S . -B build -A x64 -DPython3_ROOT_DIR="C:/Python312"
cmake --build build --config Release
./tools/package-windows.ps1
```

The packaging script downloads the official Python 3.12.10 embeddable distribution and creates `dist/FlowDeck-windows-x64.zip`. It requires a network connection. If Python is already discoverable by CMake, omit `-DPython3_ROOT_DIR`.

## CI and releases

GitHub Actions builds and smoke tests every pull request, push to `main`, and manual run. Each successful `main` push publishes a development prerelease named `build-<run number>` with the tested ZIP. Pushing a `v*` tag publishes a regular release with the same tested ZIP. Failed builds never publish a release. The ZIP is also retained as an Actions artifact.

Python plugins live in `plugins/<name>/` with a `manifest.json` and an entry script. See [the example](plugins/example-hello/main.py) for the current API.

## License

FlowDeck is licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE). The packaged CPython runtime retains its own license in `PYTHON-LICENSE.txt`.
