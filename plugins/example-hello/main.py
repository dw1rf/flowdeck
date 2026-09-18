"""Example FlowDeck plugin."""

from flowdeck import Plugin, command

plugin = Plugin("example-hello")


@command("hello")
def hello(ctx):
    """Say hello from the example plugin."""
    ctx.notify("👋 Hello from FlowDeck!")
