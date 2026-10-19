#pragma once

namespace tooie {
    // Registers the top-left present-rate overlay with the frontend. Call once after the frontend is set up.
    void register_fps_overlay();
    // Thread-safe. Off by default; driven by the graphics option.
    void set_fps_overlay_enabled(bool enabled);
}
