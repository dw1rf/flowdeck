#include "core/palette.hpp"

#include <algorithm>
#include <iostream>

#include "core/fuzzy.hpp"

namespace flowdeck {

Palette& Palette::Instance() {
    static Palette instance;
    return instance;
}

void Palette::AddCommand(Command cmd) {
    commands_.push_back(std::move(cmd));
    Rerank();
}

void Palette::RemoveCommand(const std::string& id) {
    commands_.erase(std::remove_if(commands_.begin(), commands_.end(),
                                   [&](const Command& c) { return c.id == id; }),
                    commands_.end());
    Rerank();
}

void Palette::Clear() {
    commands_.clear();
    ranked_.clear();
    selection_ = 0;
}

void Palette::SetQuery(const std::string& query) {
    query_ = query;
    selection_ = 0;
    Rerank();
}

void Palette::Rerank() {
    std::vector<std::string> haystack;
    haystack.reserve(commands_.size());
    for (const auto& c : commands_) {
        haystack.push_back(c.hint.empty() ? c.title : c.title + " " + c.hint);
    }
    ranked_ = FuzzyRank(query_, haystack);
    if (selection_ >= static_cast<int>(ranked_.size())) selection_ = 0;
}

void Palette::MoveSelection(int delta) {
    if (ranked_.empty()) return;
    const int n = static_cast<int>(ranked_.size());
    selection_ = ((selection_ + delta) % n + n) % n;
}

std::vector<const Command*> Palette::Ranked() const {
    std::vector<const Command*> out;
    out.reserve(ranked_.size());
    for (int i : ranked_) out.push_back(&commands_[static_cast<size_t>(i)]);
    return out;
}

bool Palette::ExecuteSelected() {
    if (ranked_.empty()) {
        std::cout << "[palette] nothing to run\n";
        return false;
    }
    if (selection_ < 0 || selection_ >= static_cast<int>(ranked_.size())) {
        return false;
    }

    const Command& cmd = commands_[static_cast<size_t>(ranked_[static_cast<size_t>(selection_)])];
    std::cout << "[palette] run — " << cmd.title << "  [" << cmd.id << "]\n";
    if (cmd.run) cmd.run();
    Hide();
    return true;
}

bool Palette::ExecuteById(const std::string& id) {
    auto it = std::find_if(commands_.begin(), commands_.end(),
                           [&](const Command& c) { return c.id == id; });
    if (it == commands_.end()) {
        std::cout << "[palette] unknown command: " << id << "\n";
        return false;
    }
    std::cout << "[palette] run — " << it->title << "  [" << it->id << "]\n";
    if (it->run) it->run();
    return true;
}

void Palette::Show() {
    visible_ = true;
    selection_ = 0;
    SetQuery("");

    // Console-rendered palette until the Direct2D window lands.
    std::cout << "\n[palette] open — " << commands_.size() << " commands\n";
    int i = 0;
    for (const Command* c : Ranked()) {
        std::cout << "  " << (i == selection_ ? ">" : " ") << " " << c->title;
        if (!c->hint.empty()) std::cout << "   (" << c->hint << ")";
        std::cout << "\n";
        ++i;
    }
}

void Palette::Hide() {
    visible_ = false;
    std::cout << "[palette] closed\n";
}

void Palette::Toggle() {
    if (visible_) {
        Hide();
    } else {
        Show();
    }
}

}  // namespace flowdeck
