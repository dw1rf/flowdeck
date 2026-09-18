#pragma once

#include <functional>
#include <string>
#include <vector>

namespace flowdeck {

struct Command {
    std::string id;
    std::string title;
    std::string hint;  // optional, e.g. "plugin: my-plugin"
    std::function<void()> run;
};

// The command palette. Holds all commands and toggles visibility.
// For now the UI is a stub — real window is wired in next iteration.
class Palette {
 public:
    static Palette& Instance();

    Palette(const Palette&) = delete;
    Palette& operator=(const Palette&) = delete;

    void AddCommand(Command cmd);
    void RemoveCommand(const std::string& id);

    // Show/hide the palette window.
    void Toggle();

    const std::vector<Command>& Commands() const { return commands_; }

 private:
    Palette() = default;

    std::vector<Command> commands_;
    bool visible_ = false;
};

}  // namespace flowdeck
