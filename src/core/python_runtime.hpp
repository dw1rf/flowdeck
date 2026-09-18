#pragma once

// Embedded CPython runtime.
//
// Owns a single interpreter for the whole process. Every call into the
// CPython C API from C++ must happen under a GilGuard.

#include <string>

namespace flowdeck::python {

// Initialise the interpreter. Idempotent; returns true when ready.
// `plugin_root` is prepended to sys.path so plugin folders are importable.
bool Initialise(const std::wstring& plugin_root);

// Finalise the interpreter. Safe to call when not initialised.
void Shutdown();

// True once Initialise() has succeeded.
bool IsReady();

// RAII wrapper around PyGILState_Ensure / PyGILState_Release.
class GilGuard {
 public:
    GilGuard();
    ~GilGuard();
    GilGuard(const GilGuard&) = delete;
    GilGuard& operator=(const GilGuard&) = delete;

 private:
    int state_;  // stores PyGILState_STATE without leaking Python.h
};

// Append a directory to sys.path. Must be called with the GIL held.
bool AddSysPath(const std::wstring& dir);

// Execute a Python source string. Returns false and prints the traceback
// on error.
bool RunString(const std::string& source);

}  // namespace flowdeck::python
