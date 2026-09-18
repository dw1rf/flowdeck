function on_load()
    flowdeck.notify("Lua plugin ready")
end

COMMANDS = {
    { id = "hello", title = "Say Hello (Lua)" }
}

function run_hello()
    flowdeck.notify("Hello from Lua!")
end
