// Real RT64 fullscreen transitions on Win32 windows in a private test desktop.
// No ROM, game runtime, input injection or window on the user's desktop.
#define SDL_MAIN_HANDLED
#include "hle/rt64_application_window.h"
#include <cstdio>
#include <stdexcept>

static void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Desktop {
    HDESK original = GetThreadDesktop(GetCurrentThreadId());
    HDESK isolated = CreateDesktopW(L"TooieRT64WindowRegression", nullptr, nullptr, 0, GENERIC_ALL, nullptr);
    Desktop() {
        check(isolated != nullptr, "create isolated desktop");
        check(SetThreadDesktop(isolated) != 0, "select isolated desktop");
    }
    ~Desktop() { SetThreadDesktop(original); CloseDesktop(isolated); }
};

static void transition_test(bool visible) {
    HWND handle = CreateWindowExW(0, L"STATIC", L"RT64 visibility regression",
        WS_OVERLAPPEDWINDOW | (visible ? WS_VISIBLE : 0), 80, 80, 480, 320,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    check(handle != nullptr, "create test window");
    struct Close { HWND window; ~Close() { DestroyWindow(window); } } close{handle};
    RT64::ApplicationWindow window;
    window.windowHandle = handle;
    RECT original{};
    GetWindowRect(handle, &original);
    for (int iteration = 0; iteration < 2; ++iteration) {
        window.setFullScreen(true);
        check(window.fullScreen, "entered fullscreen");
        check(bool(IsWindowVisible(handle)) == visible, "fullscreen changed window visibility");
        check(bool(GetWindowLongPtrW(handle, GWL_STYLE) & WS_VISIBLE) == visible, "fullscreen changed WS_VISIBLE");
        window.setFullScreen(false);
        check(!window.fullScreen, "left fullscreen");
        check(bool(IsWindowVisible(handle)) == visible, "windowed transition changed visibility");
        RECT restored{};
        GetWindowRect(handle, &restored);
        check(EqualRect(&original, &restored) != 0, "windowed bounds were not restored");
    }
}

int main() {
    try {
        Desktop desktop;
        transition_test(false);
        transition_test(true);
        std::puts("PASS: hidden and visible Win32 windows retain visibility across real RT64 fullscreen/windowed transitions");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
