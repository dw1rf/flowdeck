#include "ui/launcher_window.hpp"

#include <windowsx.h>
#include <commctrl.h>

#include <algorithm>
#include <string>
#include <vector>

#include "core/palette.hpp"
#include "core/window_tiler.hpp"

namespace flowdeck {
namespace {

constexpr int kWidth = 900;
constexpr int kHeight = 620;
constexpr int kSearchId = 101;
constexpr int kTrayShow = 201;
constexpr int kTrayExit = 202;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr int kRowTop = 186;
constexpr int kRowHeight = 46;
constexpr int kVisibleRows = 8;

COLORREF Background() { return RGB(17, 20, 27); }
COLORREF Surface() { return RGB(27, 32, 42); }
COLORREF SurfaceLight() { return RGB(35, 42, 55); }
COLORREF Accent() { return RGB(104, 166, 255); }
COLORREF PrimaryText() { return RGB(236, 241, 250); }
COLORREF MutedText() { return RGB(152, 163, 183); }

std::wstring Wide(const std::string& utf8) {
    if (utf8.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                          static_cast<int>(utf8.size()),
                                          nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                        static_cast<int>(utf8.size()), result.data(), count);
    return result;
}

std::string Utf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                          static_cast<int>(wide.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string result(static_cast<size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                        static_cast<int>(wide.size()), result.data(), count,
                        nullptr, nullptr);
    return result;
}

void Box(HDC dc, const RECT& bounds, COLORREF fill, COLORREF border,
         int radius = 12) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    const auto old_brush = SelectObject(dc, brush);
    const auto old_pen = SelectObject(dc, pen);
    RoundRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom,
              radius, radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void Label(HDC dc, const std::wstring& value, RECT bounds, HFONT font,
           COLORREF color, UINT align = DT_LEFT) {
    const auto old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, value.c_str(), -1, &bounds,
              DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | align);
    SelectObject(dc, old_font);
}

std::string PresetFor(const Command* command) {
    if (!command) return {};
    constexpr char prefix[] = "core:tile-";
    if (command->id.rfind(prefix, 0) != 0) return {};
    return command->id.substr(sizeof(prefix) - 1);
}

std::wstring PresetDescription(const std::string& preset) {
    if (preset == "coding") return L"Main window 70% left; others stack on the right.";
    if (preset == "trading") return L"All windows in equal vertical columns.";
    if (preset == "chill") return L"Large 16:9 main area, with a narrow side stack.";
    return L"Windows in a balanced grid.";
}

}  // namespace

LauncherWindow::~LauncherWindow() {
    if (tray_.hWnd) Shell_NotifyIconW(NIM_DELETE, &tray_);
    if (hwnd_) DestroyWindow(hwnd_);
    if (font_) DeleteObject(font_);
    if (font_small_) DeleteObject(font_small_);
    if (font_title_) DeleteObject(font_title_);
    if (search_brush_) DeleteObject(search_brush_);
}

bool LauncherWindow::Create(HINSTANCE instance, HWND quit_window) {
    quit_window_ = quit_window;
    WNDCLASSW window_class{};
    window_class.hInstance = instance;
    window_class.lpfnWndProc = WindowProc;
    window_class.lpszClassName = L"FlowDeckLauncher";
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&window_class);

    font_ = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH, L"Segoe UI");
    font_small_ = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, L"Segoe UI");
    font_title_ = CreateFontW(-22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, L"Segoe UI");
    search_brush_ = CreateSolidBrush(Surface());

    const int x = (GetSystemMetrics(SM_CXSCREEN) - kWidth) / 2;
    const int y = std::max(50, (GetSystemMetrics(SM_CYSCREEN) - kHeight) / 3);
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                            window_class.lpszClassName, L"FlowDeck",
                            WS_POPUP, x, y, kWidth, kHeight, nullptr, nullptr,
                            instance, this);
    if (!hwnd_) return false;

    search_ = CreateWindowExW(0, L"EDIT", L"",
                              WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                              56, 81, 785, 37, hwnd_,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSearchId)), instance,
                              nullptr);
    if (!search_) return false;
    SendMessageW(search_, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    SendMessageW(search_, EM_SETCUEBANNER, TRUE,
                 reinterpret_cast<LPARAM>(L"Search commands or choose a layout"));
    SetWindowLongPtrW(search_, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(this));
    original_search_proc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
        search_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SearchProc)));

    tray_.cbSize = sizeof(tray_);
    tray_.hWnd = hwnd_;
    tray_.uID = 1;
    tray_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tray_.uCallbackMessage = kTrayMessage;
    tray_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(tray_.szTip, L"FlowDeck - click to open");
    Shell_NotifyIconW(NIM_ADD, &tray_);
    return true;
}

void LauncherWindow::Show() {
    if (!hwnd_) return;
    if (!visible_) {
        Palette::Instance().Show();
        visible_ = true;
        first_visible_ = 0;
        SetWindowTextW(search_, L"");
    }
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd_);
    SetFocus(search_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void LauncherWindow::Hide() {
    if (!visible_) return;
    visible_ = false;
    Palette::Instance().Hide();
    ShowWindow(hwnd_, SW_HIDE);
}

void LauncherWindow::Toggle() {
    if (visible_) Hide(); else Show();
}

void LauncherWindow::OpenPreset(const std::string& name) {
    Show();
    SetWindowTextW(search_, L"");
    const auto ranked = Palette::Instance().Ranked();
    for (size_t i = 0; i < ranked.size(); ++i) {
        if (ranked[i]->id == "core:tile-" + name) {
            Palette::Instance().MoveSelection(static_cast<int>(i));
            if (i >= kVisibleRows) first_visible_ = static_cast<int>(i) - kVisibleRows + 1;
            break;
        }
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void LauncherWindow::ExecuteSelected() {
    if (!visible_) { Show(); return; }
    const auto ranked = Palette::Instance().Ranked();
    if (ranked.empty()) return;
    const auto selected = Palette::Instance().selection();
    if (selected < 0 || selected >= static_cast<int>(ranked.size())) return;
    Palette::Instance().ExecuteSelected();
    Hide();
}

void LauncherWindow::UpdateQuery() {
    const int length = GetWindowTextLengthW(search_);
    std::wstring query(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(search_, query.data(), length + 1);
    query.resize(static_cast<size_t>(length));
    Palette::Instance().SetQuery(Utf8(query));
    first_visible_ = 0;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void LauncherWindow::MoveSelection(int delta) {
    Palette::Instance().MoveSelection(delta);
    const int selected = Palette::Instance().selection();
    if (selected < first_visible_) first_visible_ = selected;
    if (selected >= first_visible_ + kVisibleRows)
        first_visible_ = selected - kVisibleRows + 1;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void LauncherWindow::SelectRow(int row) {
    const auto ranked = Palette::Instance().Ranked();
    const int index = first_visible_ + row;
    if (index < 0 || index >= static_cast<int>(ranked.size())) return;
    Palette::Instance().MoveSelection(index - Palette::Instance().selection());
    SetFocus(search_);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void LauncherWindow::DrawLayout(HDC dc, const RECT& bounds,
                                const std::string& preset, bool detailed) {
    Box(dc, bounds, Surface(), RGB(58, 68, 85), 12);
    auto preview = WindowTiler::Instance().PreviewPreset(preset);
    const int inset = detailed ? 20 : 4;
    RECT inner{bounds.left + inset, bounds.top + inset,
               bounds.right - inset, bounds.bottom - inset};
    const int available_w = inner.right - inner.left;
    const int available_h = inner.bottom - inner.top;
    if (available_w <= 0 || available_h <= 0) return;
    const int work_w = std::max(1, preview.work_area.w);
    const int work_h = std::max(1, preview.work_area.h);
    const double scale = std::min(double(available_w) / work_w,
                                  double(available_h) / work_h);
    const int drawn_w = static_cast<int>(work_w * scale);
    const int drawn_h = static_cast<int>(work_h * scale);
    const int origin_x = inner.left + (available_w - drawn_w) / 2;
    const int origin_y = inner.top + (available_h - drawn_h) / 2;
    const int gap = detailed ? 5 : 2;
    if (preview.windows.empty()) {
        if (detailed) Label(dc, L"No windows available to arrange",
                            inner, font_, MutedText(), DT_CENTER);
        return;
    }
    for (size_t i = 0; i < preview.windows.size(); ++i) {
        const auto& window = preview.windows[i];
        const auto& slot = window.rect;
        RECT tile{
            origin_x + static_cast<int>((slot.x - preview.work_area.x) * scale) + gap,
            origin_y + static_cast<int>((slot.y - preview.work_area.y) * scale) + gap,
            origin_x + static_cast<int>((slot.x + slot.w - preview.work_area.x) * scale) - gap,
            origin_y + static_cast<int>((slot.y + slot.h - preview.work_area.y) * scale) - gap
        };
        if (tile.right <= tile.left || tile.bottom <= tile.top) continue;
        Box(dc, tile, i == 0 ? RGB(44, 77, 125) : RGB(41, 53, 72),
            i == 0 ? Accent() : RGB(75, 95, 126), detailed ? 10 : 3);
        if (detailed && tile.right - tile.left > 78 &&
            tile.bottom - tile.top > 30) {
            RECT title{tile.left + 9, tile.top + 6, tile.right - 8,
                       tile.top + 32};
            Label(dc, window.title, title, font_small_, PrimaryText());
        }
    }
}

void LauncherWindow::Paint(HDC dc) {
    RECT entire{0, 0, kWidth, kHeight};
    HBRUSH background = CreateSolidBrush(Background());
    FillRect(dc, &entire, background);
    DeleteObject(background);
    Box(dc, RECT{0, 0, kWidth, kHeight}, Background(), RGB(66, 76, 93), 16);

    Label(dc, L"FlowDeck", RECT{28, 13, 225, 55}, font_title_, PrimaryText());
    Label(dc, L"COMMANDS  /  LAYOUTS", RECT{235, 15, 540, 52},
          font_small_, MutedText());
    Box(dc, RECT{849, 18, 878, 46}, Surface(), RGB(58, 68, 85), 7);
    Label(dc, L"×", RECT{850, 18, 877, 45}, font_, PrimaryText(), DT_CENTER);
    Box(dc, RECT{28, 68, 872, 130}, Surface(), RGB(68, 83, 108), 11);
    Label(dc, L"⌕", RECT{39, 79, 59, 118}, font_title_, Accent());

    Label(dc, L"COMMANDS", RECT{31, 151, 290, 177},
          font_small_, MutedText());
    const auto ranked = Palette::Instance().Ranked();
    if (ranked.empty()) {
        Label(dc, L"No matching commands", RECT{32, 200, 300, 250},
              font_, MutedText());
    }
    for (int row = 0; row < kVisibleRows; ++row) {
        const int index = first_visible_ + row;
        if (index >= static_cast<int>(ranked.size())) break;
        const Command* command = ranked[static_cast<size_t>(index)];
        const int top = kRowTop + row * kRowHeight;
        const bool selected = index == Palette::Instance().selection();
        if (selected) Box(dc, RECT{22, top, 310, top + 43},
                          RGB(39, 57, 82), RGB(67, 106, 158), 8);
        const auto preset = PresetFor(command);
        if (!preset.empty()) {
            DrawLayout(dc, RECT{32, top + 8, 78, top + 35}, preset, false);
        } else {
            Box(dc, RECT{39, top + 11, 71, top + 33}, SurfaceLight(),
                RGB(75, 95, 126), 7);
            Label(dc, L"›", RECT{40, top + 10, 70, top + 32},
                  font_small_, Accent(), DT_CENTER);
        }
        Label(dc, Wide(command->title), RECT{90, top + 2, 297, top + 42},
              font_, selected ? PrimaryText() : MutedText());
    }

    HBRUSH divider = CreateSolidBrush(RGB(57, 65, 79));
    RECT line{323, 156, 324, 550};
    FillRect(dc, &line, divider);
    DeleteObject(divider);

    const Command* chosen = nullptr;
    if (!ranked.empty() && Palette::Instance().selection() <
                               static_cast<int>(ranked.size())) {
        chosen = ranked[static_cast<size_t>(Palette::Instance().selection())];
    }
    const std::string preset = PresetFor(chosen);
    Label(dc, preset.empty() ? L"COMMAND" : L"LAYOUT PREVIEW",
          RECT{344, 151, 850, 178}, font_small_, Accent());
    Label(dc, chosen ? Wide(chosen->title) : L"Choose a command",
          RECT{344, 176, 860, 211}, font_title_, PrimaryText());

    if (!preset.empty()) {
        DrawLayout(dc, RECT{344, 218, 868, 481}, preset, true);
        const auto preview = WindowTiler::Instance().PreviewPreset(preset);
        std::wstring count = std::to_wstring(preview.windows.size()) +
                             L" window(s) will move";
        if (preview.omitted) count += L" · " +
            std::to_wstring(preview.omitted) + L" left unchanged";
        Label(dc, count, RECT{345, 493, 860, 520}, font_small_, Accent());
        Label(dc, PresetDescription(preset), RECT{345, 518, 865, 545},
              font_small_, MutedText());
    } else {
        Box(dc, RECT{344, 218, 868, 481}, Surface(), RGB(58, 68, 85), 12);
        Label(dc, chosen ? Wide(chosen->hint) : L"Search or select a command",
              RECT{370, 285, 835, 355}, font_, MutedText(), DT_CENTER);
        Label(dc, L"Select a layout to see exactly where open windows go.",
              RECT{347, 492, 865, 543}, font_small_, MutedText());
    }

    Label(dc, L"↑ ↓  Select     Enter  Apply     Esc  Close",
          RECT{29, 567, 645, 604}, font_small_, MutedText());
    Box(dc, RECT{711, 556, 870, 600}, chosen ? Accent() : SurfaceLight(),
        chosen ? Accent() : SurfaceLight(), 10);
    Label(dc, preset.empty() ? L"Run command" : L"Apply layout",
          RECT{715, 560, 866, 597}, font_, RGB(12, 23, 37), DT_CENTER);
}

void LauncherWindow::ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kTrayShow, L"Open FlowDeck");
    AppendMenuW(menu, MF_STRING, kTrayExit, L"Exit");
    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

LRESULT LauncherWindow::HandleMessage(UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
        case WM_COMMAND:
            if (LOWORD(wp) == kSearchId && HIWORD(wp) == EN_CHANGE) {
                UpdateQuery();
                return 0;
            }
            if (LOWORD(wp) == kTrayShow) { Show(); return 0; }
            if (LOWORD(wp) == kTrayExit) {
                PostMessageW(quit_window_, WM_CLOSE, 0, 0);
                return 0;
            }
            break;
        case kTrayMessage:
            if (lp == WM_LBUTTONUP || lp == WM_LBUTTONDBLCLK) {
                Show(); return 0;
            }
            if (lp == WM_RBUTTONUP) { ShowTrayMenu(); return 0; }
            break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
            SetTextColor(reinterpret_cast<HDC>(wp), PrimaryText());
            SetBkColor(reinterpret_cast<HDC>(wp), Surface());
            return reinterpret_cast<LRESULT>(search_brush_);
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC screen = BeginPaint(hwnd_, &paint);
            HDC memory = CreateCompatibleDC(screen);
            HBITMAP bitmap = CreateCompatibleBitmap(screen, kWidth, kHeight);
            HGDIOBJ old = SelectObject(memory, bitmap);
            Paint(memory);
            BitBlt(screen, 0, 0, kWidth, kHeight, memory, 0, 0, SRCCOPY);
            SelectObject(memory, old);
            DeleteObject(bitmap);
            DeleteDC(memory);
            EndPaint(hwnd_, &paint);
            return 0;
        }
        case WM_MOUSEWHEEL:
            MoveSelection(GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -1 : 1);
            return 0;
        case WM_LBUTTONDOWN: {
            const int x = GET_X_LPARAM(lp);
            const int y = GET_Y_LPARAM(lp);
            if (x >= 849 && y >= 18 && y <= 47) { Hide(); return 0; }
            if (y < 55) {
                SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                return 0;
            }
            if (x >= 22 && x <= 310 && y >= kRowTop &&
                y < kRowTop + kVisibleRows * kRowHeight) {
                SelectRow((y - kRowTop) / kRowHeight);
                return 0;
            }
            if (x >= 711 && x <= 870 && y >= 556 && y <= 600) {
                ExecuteSelected(); return 0;
            }
            break;
        }
        case WM_ACTIVATE:
            if (LOWORD(wp) == WA_INACTIVE && visible_) Hide();
            return 0;
        case WM_CLOSE:
            Hide();
            return 0;
    }
    return DefWindowProcW(hwnd_, message, wp, lp);
}

LRESULT CALLBACK LauncherWindow::WindowProc(HWND hwnd, UINT message,
                                             WPARAM wp, LPARAM lp) {
    if (message == WM_NCCREATE) {
        auto* creation = reinterpret_cast<CREATESTRUCTW*>(lp);
        auto* self = static_cast<LauncherWindow*>(creation->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return DefWindowProcW(hwnd, message, wp, lp);
    }
    auto* self = reinterpret_cast<LauncherWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return self ? self->HandleMessage(message, wp, lp) :
                  DefWindowProcW(hwnd, message, wp, lp);
}

LRESULT CALLBACK LauncherWindow::SearchProc(HWND hwnd, UINT message,
                                             WPARAM wp, LPARAM lp) {
    auto* self = reinterpret_cast<LauncherWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, message, wp, lp);
    if (message == WM_KEYDOWN) {
        if (wp == VK_RETURN) { self->ExecuteSelected(); return 0; }
        if (wp == VK_ESCAPE) { self->Hide(); return 0; }
        if (wp == VK_DOWN) { self->MoveSelection(1); return 0; }
        if (wp == VK_UP) { self->MoveSelection(-1); return 0; }
    }
    return CallWindowProcW(self->original_search_proc_, hwnd, message, wp, lp);
}

}  // namespace flowdeck
