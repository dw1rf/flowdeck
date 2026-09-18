# FlowDeck

**Universal command palette with window tiling and extensible plugin system for Windows.**

FlowDeck combines a Raycast-style command palette, a window tiler with presets, and a plugin runtime into one fast, keyboard-first desktop tool.

## Features

- ⌘ **Command Palette** — fuzzy search, global hotkey, instant actions
- 🪟 **Window Tiler** — presets (Coding, Trading, Chill), hotkey-driven, per-app rules
- 🔌 **Plugin System** — write plugins in Python or Lua, hot-reloadable, sandboxed
- ⚡ **Fast & Light** — native Rust/Tauri core, minimal footprint
- 🎨 **Themable** — match your desktop aesthetic

## Philosophy

- **Keyboard-first** — everything reachable in ≤2 keystrokes
- **Extensible** — the core is small; plugins do the rest
- **Non-commercial** — free to use and extend, not to sell

## Plugin System

Plugins live in `plugins/` and are auto-discovered. Each plugin is a folder with a `manifest.json` and an entry script.

```json
// plugins/my-plugin/manifest.json
{
  "name": "my-plugin",
  "version": "0.1.0",
  "author": "you",
  "description": "Does something cool",
  "entry": "main.py",
  "commands": [
    { "id": "hello", "title": "Say Hello", "hotkey": "Ctrl+Shift+H" }
  ]
}
```

See [`plugins/README.md`](plugins/README.md) for the full API.

## License

This project is licensed under the **PolyForm Noncommercial License 1.0.0**.

- ✅ You may use, modify, and extend FlowDeck
- ✅ You may write and share plugins (under any license)
- ❌ You may **not** sell, sublicense commercially, or monetize FlowDeck itself
- ✅ Attribution / copyright notices must be retained

See [`LICENSE`](LICENSE) for the full text.

## Contributing

PRs welcome! Read [`CONTRIBUTING.md`](CONTRIBUTING.md) first.

---

Made by [@dw1rf](https://github.com/dw1rf)