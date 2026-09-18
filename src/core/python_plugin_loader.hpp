#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/palette.hpp"

namespace flowdeck {

// Must be called before Py_InitializeFromConfig.
bool RegisterHostModule();

// One Python plugin: a folder with manifest.json + an entry .py module.
class PythonPlugin {
 public:
    struct Manifest {
        std::string name;
        std::string version;
        std::string author;
        std::string description;
        std::string entry = "main.py";
        std::vector<std::string> permissions;
    };

    // Parses manifest.json, imports the entry module and calls on_load().
    static std::unique_ptr<PythonPlugin> Load(const std::wstring& folder);

    ~PythonPlugin();
    PythonPlugin(const PythonPlugin&) = delete;
    PythonPlugin& operator=(const PythonPlugin&) = delete;

    // Reads COMMANDS / get_commands() from the module and pushes them
    // into the palette, wiring each one to its Python callable.
    void RegisterCommands(Palette& palette);

    const Manifest& manifest() const { return manifest_; }

 private:
    explicit PythonPlugin(Manifest m) : manifest_(std::move(m)) {}

    Manifest manifest_;
    std::wstring folder_;
    void* module_ = nullptr;  // PyObject* (strong ref)
};

// Scans a directory for plugin folders and keeps them alive.
class PythonPluginLoader {
 public:
    static PythonPluginLoader& Instance();

    PythonPluginLoader(const PythonPluginLoader&) = delete;
    PythonPluginLoader& operator=(const PythonPluginLoader&) = delete;

    void LoadAll(const std::wstring& dir);
    void RegisterCommands(Palette& palette);
    void UnloadAll();

    size_t count() const { return plugins_.size(); }

 private:
    PythonPluginLoader() = default;

    std::vector<std::unique_ptr<PythonPlugin>> plugins_;
};

}  // namespace flowdeck
