#include "core/palette.hpp"

#include <algorithm>
#include <iostream>

namespace flowdeck {

Palette& Palette::Instance() {
    static Palette instance;
    return instance;
}

void Palette::AddCommand(Command cmd) {
    commands_.push_back(std::move(cmd));
}

void Palette::RemoveCommand(const std::string& id) {
    commands_.erase(
        std::remove_if(commands_.begin(), commands_.end(),
                       [&](const Command& c) { return c.id == id; }),
        commands_.end());
}

void Palette::Toggle() {
    visible_ = !visible_;
    // TODO: create/show the palette window (Win32 or DirectX overlay).
    // For now, list commands to the console so the pipeline is testable.
    std::cout << "[palette] " << (visible_ ? "open" : "closed") << " — "
              << commands_.size() << " commands\n";
    if (visible_) {
        for (const auto& c : commands_) {
            std::cout << "  - " << c.title << "  [" << c.id << "]\n";
        }
    }
}

}  // namespace flowdeck
