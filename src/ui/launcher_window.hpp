#pragma once

#include <windows.h>
#include <shellapi.h>

#include <string>

namespace flowdeck {

class LauncherWindow {
 public:
    ~LauncherWindow();
    bool Create(HINSTANCE instance, HWND quit_window);
    void Show();
    void Hide();
    void Toggle();
    void OpenPreset(const std::string& name);
    bool visible() const { return visible_; }
    void ExecuteSelected();

 private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wp,
                                       LPARAM lp);
    static LRESULT CALLBACK SearchProc(HWND hwnd, UINT message, WPARAM wp,
                                       LPARAM lp);
    LRESULT HandleMessage(UINT message, WPARAM wp, LPARAM lp);
    void Paint(HDC dc);
    void DrawLayout(HDC dc, const RECT& bounds, const std::string& preset,
                    bool detailed);
    void UpdateQuery();
    void MoveSelection(int delta);
    void SelectRow(int row);
    void ShowTrayMenu();

    HWND hwnd_ = nullptr;
    HWND search_ = nullptr;
    HWND quit_window_ = nullptr;
    WNDPROC original_search_proc_ = nullptr;
    HFONT font_ = nullptr;
    HFONT font_small_ = nullptr;
    HFONT font_title_ = nullptr;
    HBRUSH search_brush_ = nullptr;
    NOTIFYICONDATAW tray_{};
    bool visible_ = false;
    int first_visible_ = 0;
};

}  // namespace flowdeck
