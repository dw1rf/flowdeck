#include "core/lua_plugin_loader.hpp"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <filesystem>
#include <iostream>
#include "core/palette.hpp"
#include "core/notifications.hpp"

namespace flowdeck {
namespace {
int notify(lua_State* state) {
    const char* value = luaL_checkstring(state, 1);
    std::cout << "[lua-plugin] " << value << '\n';
    flowdeck::notify(QString::fromUtf8(value));
    return 0;
}
}
struct LuaPluginLoader::Plugin {
    std::string name;
    lua_State* state = nullptr;
    std::vector<std::string> commands;
    ~Plugin() {
        if (!state) return;
        lua_getglobal(state, "on_unload");
        if (lua_isfunction(state, -1)) {
            if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
                std::cerr << lua_tostring(state, -1) << '\n';
                lua_pop(state, 1);
            }
        } else lua_pop(state, 1);
        for (const auto& id : commands) Palette::Instance().RemoveCommand(id);
        lua_close(state);
    }
};
LuaPluginLoader& LuaPluginLoader::instance() { static LuaPluginLoader loader; return loader; }
void LuaPluginLoader::loadAll(const std::wstring& directory) {
    namespace fs = std::filesystem;
    if (!fs::exists(directory)) return;
    for (const auto& item : fs::directory_iterator(directory)) {
        if (!item.is_directory()) continue;
        QFile manifest(QString::fromStdWString((item.path()/"manifest.json").wstring()));
        if (!manifest.open(QIODevice::ReadOnly)) continue;
        const auto doc = QJsonDocument::fromJson(manifest.readAll());
        if (!doc.isObject()) continue;
        const auto json = doc.object();
        const auto entry = json.value("entry").toString();
        if (!entry.endsWith(".lua") || entry.contains("..") || entry.contains('/') || entry.contains('\\')) continue;
        const auto script = item.path()/entry.toStdWString();
        if (!fs::exists(script)) continue;
        auto plugin = std::make_unique<Plugin>();
        plugin->name = json.value("name").toString().toStdString();
        if (plugin->name.empty()) continue;
        plugin->state = luaL_newstate();
        if (!plugin->state) continue;
        luaL_openlibs(plugin->state);
        lua_newtable(plugin->state);
        lua_pushcfunction(plugin->state, notify);
        lua_setfield(plugin->state, -2, "notify");
        lua_setglobal(plugin->state, "flowdeck");
        const auto scriptPath = QString::fromStdWString(script.wstring()).toUtf8();
        if (luaL_dofile(plugin->state, scriptPath.constData()) != LUA_OK) {
            std::cerr << "[lua-plugin] " << lua_tostring(plugin->state,-1) << '\n';
            continue;
        }
        lua_getglobal(plugin->state, "on_load");
        if (lua_isfunction(plugin->state,-1)) {
            if (lua_pcall(plugin->state,0,0,0) != LUA_OK) std::cerr << lua_tostring(plugin->state,-1) << '\n';
        } else lua_pop(plugin->state,1);
        lua_getglobal(plugin->state, "COMMANDS");
        if (lua_istable(plugin->state,-1)) {
            const auto count = lua_rawlen(plugin->state,-1);
            for (size_t i=1;i<=count;++i) {
                lua_rawgeti(plugin->state,-1,static_cast<lua_Integer>(i));
                if (!lua_istable(plugin->state,-1)) { lua_pop(plugin->state,1); continue; }
                lua_getfield(plugin->state,-1,"id");
                const auto id = lua_isstring(plugin->state,-1) ? std::string(lua_tostring(plugin->state,-1)) : "";
                lua_pop(plugin->state,1);
                lua_getfield(plugin->state,-1,"title");
                const auto title = lua_isstring(plugin->state,-1) ? std::string(lua_tostring(plugin->state,-1)) : "";
                lua_pop(plugin->state,1);
                lua_getfield(plugin->state,-1,"title_ru");
                const auto titleRu = lua_isstring(plugin->state,-1) ? std::string(lua_tostring(plugin->state,-1)) : "";
                lua_pop(plugin->state,1);
                lua_getfield(plugin->state,-1,"title_en");
                const auto titleEn = lua_isstring(plugin->state,-1) ? std::string(lua_tostring(plugin->state,-1)) : "";
                lua_pop(plugin->state,1);
                if (!id.empty() && !title.empty()) {
                    const auto fullId = plugin->name + ":" + id;
                    auto* state = plugin->state;
                    Palette::Instance().AddCommand({fullId,title,"lua: "+plugin->name,[state,id] {
                        lua_getglobal(state,("run_"+id).c_str());
                        if (lua_isfunction(state,-1)) {
                            if (lua_pcall(state,0,0,0)!=LUA_OK) {
                                std::cerr << "[lua-plugin] " << lua_tostring(state,-1) << '\n';
                                lua_pop(state,1);
                            }
                        } else lua_pop(state,1);
                    },titleRu,titleEn});
                    plugin->commands.push_back(fullId);
                }
                lua_pop(plugin->state,1);
            }
        }
        lua_pop(plugin->state,1);
        std::cout << "[lua-plugin] loaded " << plugin->name << '\n';
        plugins_.push_back(std::move(plugin));
    }
}
void LuaPluginLoader::unloadAll() { plugins_.clear(); }
}
