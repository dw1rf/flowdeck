# FlowDeck Plugins

Plugins extend FlowDeck. Each plugin is a folder under `plugins/` containing:

```
plugins/
  my-plugin/
    manifest.json
    main.py        # or main.lua
    README.md      # optional
```

## manifest.json

```json
{
  "name": "my-plugin",
  "version": "0.1.0",
  "author": "you",
  "description": "Does something cool",
  "entry": "main.py",
  "commands": [
    {
      "id": "hello",
      "title": "Say Hello",
      "hotkey": "Ctrl+Shift+H",
      "scope": "global"
    }
  ],
  "permissions": ["clipboard", "windows"]
}
```

## Plugin API (Python)

```python
from flowdeck import Plugin, command

plugin = Plugin("my-plugin")

@command("hello")
def hello(ctx):
    ctx.notify("Hello from my-plugin!")

@command("tile-coding")
def tile_coding(ctx):
    ctx.windows.tile(preset="coding")
```

## Permissions

Plugins declare permissions in the manifest. Available permissions:

- `clipboard` — read/write clipboard
- `windows` — move, resize, tile windows
- `process` — list/launch/kill processes
- `network` — HTTP requests
- `filesystem` — read/write files (scoped to plugin folder by default)

## Hot-Reload

Plugins are hot-reloaded on file change. No restart needed.

## Distribution

Plugins can be distributed independently of FlowDeck and may be licensed under any terms.
