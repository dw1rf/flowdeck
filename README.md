# FlowDeck

FlowDeck is an early Windows command palette and window tiler written in C++20. It loads native DLL plugins and embedded Python plugins.

## Download and try it

Download `FlowDeck-windows-x64.zip` from [Releases](https://github.com/dw1rf/flowdeck/releases). Extract the archive and run `FlowDeck-x64/flowdeck.exe`. The archive includes Python 3.12 and both example plugins; a separate Python installation is not needed.

| Shortcut | Action |
| --- | --- |
| Ctrl+Alt+Space | Toggle the command list in the console |
| Ctrl+Alt+Enter | Run the selected command |
| Ctrl+Shift+T | Tile windows with the coding preset |
| Ctrl+Shift+G | Tile windows with the trading preset |

To check the downloaded package without entering the message loop, run `flowdeck.exe --smoke-test` from a terminal. It loads both plugins, runs their greeting commands, then exits with code 0 on success. This check does not test interactive hotkeys or window placement.

The palette has no graphical window or keyboard query input yet. Python plugin changes require an application restart. Manifest permissions are parsed but are not enforced.

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
