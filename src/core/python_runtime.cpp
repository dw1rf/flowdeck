#include "core/python_runtime.hpp"

#include <windows.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <iostream>
#include <string>

namespace flowdeck::python {

namespace {

bool g_ready = false;

std::string Narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
                                  static_cast<int>(w.size()), nullptr, 0,
                                  nullptr, nullptr);
    std::string out(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()),
                        out.data(), len, nullptr, nullptr);
    return out;
}

}  // namespace

bool Initialise(const std::wstring& plugin_root) {
    if (g_ready) return true;

    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.parse_argv = 0;
    config.install_signal_handlers = 0;

    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);

    if (PyStatus_Exception(status)) {
        std::cerr << "[python] init failed: "
                  << (status.err_msg ? status.err_msg : "unknown") << "\n";
        return false;
    }

    g_ready = true;

    {
        GilGuard gil;
        AddSysPath(plugin_root);
    }

    std::cout << "[python] ready — " << Py_GetVersion() << "\n";
    return true;
}

void Shutdown() {
    if (!g_ready) return;
    PyGILState_Ensure();
    Py_Finalize();
    g_ready = false;
    std::cout << "[python] shut down\n";
}

bool IsReady() { return g_ready; }

GilGuard::GilGuard() : state_(static_cast<int>(PyGILState_Ensure())) {}

GilGuard::~GilGuard() {
    PyGILState_Release(static_cast<PyGILState_STATE>(state_));
}

bool AddSysPath(const std::wstring& dir) {
    if (!g_ready) return false;
    PyObject* sys_path = PySys_GetObject("path");  // borrowed
    if (!sys_path || !PyList_Check(sys_path)) return false;

    const std::string narrow = Narrow(dir);
    PyObject* item = PyUnicode_FromString(narrow.c_str());
    if (!item) {
        PyErr_Clear();
        return false;
    }
    int rc = PyList_Insert(sys_path, 0, item);
    Py_DECREF(item);
    if (rc != 0) {
        PyErr_Clear();
        return false;
    }
    return true;
}

bool RunString(const std::string& source) {
    if (!g_ready) return false;
    GilGuard gil;
    if (PyRun_SimpleString(source.c_str()) != 0) {
        if (PyErr_Occurred()) PyErr_Print();
        return false;
    }
    return true;
}

}  // namespace flowdeck::python
