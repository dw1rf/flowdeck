#include "core/python_plugin_loader.hpp"

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "core/python_runtime.hpp"
#include "core/notifications.hpp"

namespace flowdeck {

namespace fs = std::filesystem;

namespace {
std::function<void(const std::string&)> tileCallback;

// --- tiny JSON readers (flat manifest schema only) ---------------------------

std::string JsonString(const std::string& src, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    size_t k = src.find(needle);
    if (k == std::string::npos) return {};
    k = src.find(':', k + needle.size());
    if (k == std::string::npos) return {};
    const size_t open = src.find('"', k);
    if (open == std::string::npos) return {};
    const size_t close = src.find('"', open + 1);
    if (close == std::string::npos) return {};
    return src.substr(open + 1, close - open - 1);
}

std::vector<std::string> JsonStringArray(const std::string& src,
                                         const std::string& key) {
    std::vector<std::string> out;
    const std::string needle = "\"" + key + "\"";
    size_t k = src.find(needle);
    if (k == std::string::npos) return out;
    k = src.find('[', k);
    if (k == std::string::npos) return out;
    const size_t end = src.find(']', k);
    if (end == std::string::npos) return out;

    const std::string body = src.substr(k + 1, end - k - 1);
    size_t pos = 0;
    while (pos < body.size()) {
        const size_t open = body.find('"', pos);
        if (open == std::string::npos) break;
        const size_t close = body.find('"', open + 1);
        if (close == std::string::npos) break;
        out.push_back(body.substr(open + 1, close - open - 1));
        pos = close + 1;
    }
    return out;
}

std::string ReadWholeFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// --- host module exposed to plugins as `import flowdeck` ---------------------

PyObject* Host_notify(PyObject*, PyObject* args) {
    const char* msg = nullptr;
    if (!PyArg_ParseTuple(args, "s", &msg)) return nullptr;
    std::cout << "[plugin] " << msg << "\n";
    notify(QString::fromUtf8(msg));
    Py_RETURN_NONE;
}

PyObject* Host_tile(PyObject*, PyObject* args) {
    const char* preset = nullptr;
    if (!PyArg_ParseTuple(args, "s", &preset)) return nullptr;
    if (tileCallback) tileCallback(preset);
    Py_RETURN_NONE;
}

PyMethodDef kHostMethods[] = {
    {"notify", Host_notify, METH_VARARGS, "Show a message to the user."},
    {"tile", Host_tile, METH_VARARGS, "Apply a window tiling preset."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef kHostModule = {
    PyModuleDef_HEAD_INIT, "flowdeck",
    "FlowDeck host API available to plugins.", -1, kHostMethods,
    nullptr, nullptr, nullptr, nullptr,
};

PyObject* InitHostModule() { return PyModule_Create(&kHostModule); }

}  // namespace

bool RegisterHostModule() {
    static bool registered = false;
    if (registered) return true;
    if (PyImport_AppendInittab("flowdeck", InitHostModule) != 0) {
        std::cerr << "[python] failed to register host module\n";
        return false;
    }
    registered = true;
    return true;
}
void SetHostTileCallback(std::function<void(const std::string&)> callback) {
    tileCallback = std::move(callback);
}

std::unique_ptr<PythonPlugin> PythonPlugin::Load(const std::wstring& folder) {
    const fs::path dir(folder);
    const fs::path manifest_path = dir / "manifest.json";
    if (!fs::exists(manifest_path)) return nullptr;

    const std::string body = ReadWholeFile(manifest_path);

    Manifest m;
    m.name = JsonString(body, "name");
    m.version = JsonString(body, "version");
    m.author = JsonString(body, "author");
    m.description = JsonString(body, "description");
    const std::string entry = JsonString(body, "entry");
    if (!entry.empty()) m.entry = entry;
    if (m.entry.size() < 3 || m.entry.substr(m.entry.size() - 3) != ".py") return nullptr;
    m.permissions = JsonStringArray(body, "permissions");

    if (m.name.empty()) {
        std::wcerr << L"[python-plugin] manifest without a name: " << folder
                   << L"\n";
        return nullptr;
    }
    if (!python::IsReady()) {
        std::cerr << "[python-plugin] runtime not ready\n";
        return nullptr;
    }

    std::unique_ptr<PythonPlugin> plugin(new PythonPlugin(std::move(m)));
    plugin->folder_ = folder;

    python::GilGuard gil;
    python::AddSysPath(folder);

    std::string module_name = plugin->manifest_.entry;
    if (module_name.size() > 3 &&
        module_name.compare(module_name.size() - 3, 3, ".py") == 0) {
        module_name.resize(module_name.size() - 3);
    }

    PyObject* module = PyImport_ImportModule(module_name.c_str());
    if (!module) {
        std::cerr << "[python-plugin] import failed: " << module_name << "\n";
        if (PyErr_Occurred()) PyErr_Print();
        return nullptr;
    }
    plugin->module_ = module;

    if (PyObject* on_load = PyObject_GetAttrString(module, "on_load")) {
        PyObject* res = PyObject_CallNoArgs(on_load);
        Py_XDECREF(res);
        Py_DECREF(on_load);
        if (PyErr_Occurred()) PyErr_Print();
    } else {
        PyErr_Clear();
    }

    std::cout << "[python-plugin] loaded " << plugin->manifest_.name << " v"
              << plugin->manifest_.version << "\n";
    return plugin;
}

PythonPlugin::~PythonPlugin() {
    if (!module_ || !python::IsReady()) return;

    python::GilGuard gil;
    PyObject* module = static_cast<PyObject*>(module_);

    if (PyObject* on_unload = PyObject_GetAttrString(module, "on_unload")) {
        PyObject* res = PyObject_CallNoArgs(on_unload);
        Py_XDECREF(res);
        Py_DECREF(on_unload);
        if (PyErr_Occurred()) PyErr_Print();
    } else {
        PyErr_Clear();
    }
    Py_DECREF(module);
    module_ = nullptr;
}

void PythonPlugin::RegisterCommands(Palette& palette) {
    if (!module_ || !python::IsReady()) return;

    python::GilGuard gil;
    PyObject* module = static_cast<PyObject*>(module_);

    PyObject* list = nullptr;
    if (PyObject* getter = PyObject_GetAttrString(module, "get_commands")) {
        list = PyObject_CallNoArgs(getter);
        Py_DECREF(getter);
        if (!list && PyErr_Occurred()) PyErr_Print();
    } else {
        PyErr_Clear();
        list = PyObject_GetAttrString(module, "COMMANDS");
        if (!list) PyErr_Clear();
    }

    if (!list || !PyList_Check(list)) {
        Py_XDECREF(list);
        std::cout << "[python-plugin] " << manifest_.name
                  << ": no commands exposed\n";
        return;
    }

    const Py_ssize_t n = PyList_Size(list);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* item = PyList_GetItem(list, i);  // borrowed
        if (!PyDict_Check(item)) continue;

        PyObject* py_id = PyDict_GetItemString(item, "id");
        PyObject* py_title = PyDict_GetItemString(item, "title");
        if (!py_id || !py_title) continue;
        if (!PyUnicode_Check(py_id) || !PyUnicode_Check(py_title)) continue;

        const std::string id = PyUnicode_AsUTF8(py_id);
        const std::string title = PyUnicode_AsUTF8(py_title);
        const auto translated = [item](const char* key) {
            PyObject* value = PyDict_GetItemString(item, key);
            return value && PyUnicode_Check(value) ? std::string(PyUnicode_AsUTF8(value)) : std::string{};
        };

        // Resolve the callable: explicit "run" key, else run_<id>.
        PyObject* callable = PyDict_GetItemString(item, "run");  // borrowed
        if (callable && PyCallable_Check(callable)) {
            Py_INCREF(callable);
        } else {
            const std::string fn = "run_" + id;
            callable = PyObject_GetAttrString(module, fn.c_str());
            if (!callable) {
                PyErr_Clear();
                std::cerr << "[python-plugin] " << manifest_.name << ": " << id
                          << " has no handler (" << fn << ")\n";
                continue;
            }
        }

        const std::string plugin_name = manifest_.name;
        palette.AddCommand({
            plugin_name + ":" + id,
            title,
            "plugin: " + plugin_name,
            [callable, plugin_name, id] {
                if (!python::IsReady()) return;
                python::GilGuard call_gil;
                PyObject* res = PyObject_CallNoArgs(callable);
                if (!res) {
                    std::cerr << "[python-plugin] " << plugin_name << ":" << id
                              << " raised\n";
                    PyErr_Print();
                    return;
                }
                Py_DECREF(res);
            },
            translated("title_ru"),
            translated("title_en"),
        });
    }

    Py_DECREF(list);
}

PythonPluginLoader& PythonPluginLoader::Instance() {
    static PythonPluginLoader instance;
    return instance;
}

void PythonPluginLoader::LoadAll(const std::wstring& dir) {
    if (!python::IsReady()) {
        std::cerr << "[python-loader] runtime not ready, skipping\n";
        return;
    }
    if (!fs::exists(dir)) {
        std::wcout << L"[python-loader] no plugin directory: " << dir << L"\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        if (!fs::exists(entry.path() / "manifest.json")) continue;

        if (auto plugin = PythonPlugin::Load(entry.path().wstring())) {
            plugins_.push_back(std::move(plugin));
        }
    }
    std::cout << "[python-loader] " << plugins_.size() << " plugin(s) loaded\n";
}

void PythonPluginLoader::RegisterCommands(Palette& palette) {
    for (auto& p : plugins_) p->RegisterCommands(palette);
}

void PythonPluginLoader::UnloadAll() { plugins_.clear(); }

}  // namespace flowdeck
