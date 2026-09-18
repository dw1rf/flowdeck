#pragma once

#include <functional>
#include <string>
#include <vector>

namespace flowdeck {

struct Command {
    std::string id;
    std::string title;
    std::string hint;  // e.g. "plugin: notes"
    std::function<void()> run;
};

// Holds every registered command, ranks them against the current query
// and runs the selected one.
class Palette {
 public:
    static Palette& Instance();

    Palette(const Palette&) = delete;
    Palette& operator=(const Palette&) = delete;

    void AddCommand(Command cmd);
    void RemoveCommand(const std::string& id);
    void Clear();

    void Toggle();
    void Show();
    void Hide();
    bool visible() const { return visible_; }

    // Update the search query and re-rank.
    void SetQuery(const std::string& query);
    const std::string& query() const { return query_; }

    // Move the highlighted row.
    void MoveSelection(int delta);
    int selection() const { return selection_; }

    // Run the highlighted command (what Enter does).
    bool ExecuteSelected();

    // Run a command directly by its full id, e.g. "notes:new".
    bool ExecuteById(const std::string& id);

    // Ranked view of commands for the UI.
    std::vector<const Command*> Ranked() const;

    const std::vector<Command>& All() const { return commands_; }

 private:
    Palette() = default;
    void Rerank();

    std::vector<Command> commands_;
    std::vector<int> ranked_;
    std::string query_;
    int selection_ = 0;
    bool visible_ = false;
};

}  // namespace flowdeck
