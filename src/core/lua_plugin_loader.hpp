#pragma once
#include <memory>
#include <string>
#include <vector>

namespace flowdeck {
class LuaPluginLoader {
 public:
    static LuaPluginLoader& instance();
    void loadAll(const std::wstring& directory);
    void unloadAll();
    size_t count() const { return plugins_.size(); }
 private:
    struct Plugin;
    std::vector<std::unique_ptr<Plugin>> plugins_;
};
}
