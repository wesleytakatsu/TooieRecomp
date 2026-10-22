// Host-test stand-in for the one recompui entry point that boot_hw_stubs.cpp calls.
// The real recompui::get_window_size lives in the UI library, which needs the game's
// SDL window and launcher globals from main.cpp; the host tests have no window, so
// they see the 4:3 base size (the original N64 presentation) instead.
#include "recompui/recompui.h"

namespace recompui {

void get_window_size(int& width, int& height) {
    width = 640;
    height = 480;
}

} // namespace recompui
