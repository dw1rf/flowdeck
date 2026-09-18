function on_load()
    flowdeck.notify("Lua plugin ready")
end

COMMANDS = {
    { id = "hello", title = "Say Hello (Lua)", title_ru = "Поздороваться (Lua)" }
}

function run_hello()
    flowdeck.notify("Hello from Lua!")
end
