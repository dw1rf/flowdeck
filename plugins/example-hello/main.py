"""Example FlowDeck plugin written in Python.

The host exposes a built-in module called ``flowdeck``:

    flowdeck.notify(message: str) -> None
    flowdeck.tile(preset: str) -> None      # "coding" | "trading" | "chill" | "grid"

A plugin is discovered by its ``manifest.json`` and must expose either a
module-level ``COMMANDS`` list or a ``get_commands()`` function returning one.
Each entry is a dict with ``id`` and ``title``; the handler is either a
``run`` callable in the dict or a module function named ``run_<id>``.
"""

import datetime

import flowdeck


def on_load():
    """Called once after the module is imported."""
    flowdeck.notify("example-hello loaded")


def on_unload():
    """Called once before the plugin is dropped."""
    flowdeck.notify("example-hello unloaded")


def run_hello():
    flowdeck.notify("Hello from Python!")


def run_clock():
    now = datetime.datetime.now().strftime("%H:%M:%S")
    flowdeck.notify(f"It is {now}")


def run_tile_chill():
    flowdeck.tile("chill")


COMMANDS = [
    {"id": "hello", "title": "Say Hello (Python)"},
    {"id": "clock", "title": "Show Current Time"},
    {"id": "tile_chill", "title": "Tile: Chill (Python)"},
]
