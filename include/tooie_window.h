#ifndef TOOIE_WINDOW_H
#define TOOIE_WINDOW_H

#include <cstdint>
#include <algorithm>
#include "ultramodern/config.hpp"
#include "banjo_config.h"

#ifdef _WIN32
#include "SDL.h"
#else
#include "SDL2/SDL.h"
#endif

namespace tooie_window {

struct WindowState {
    bool focused = true;
    bool visible = true;
    bool minimized = false;
    int current_display = 0;
    int logical_width = 1600;
    int logical_height = 900;
    int pixel_width = 1600;
    int pixel_height = 900;
    float scale_x = 1.0f;
    float scale_y = 1.0f;
};

// Validates display index against available display count.
// Returns a valid display index in [0, num_displays - 1], or 0 if num_displays <= 0.
int validate_display_index(int display_index, int num_displays);

// Sanitizes width and height to ensure strictly positive, non-zero dimensions (at least min_w x min_h).
void sanitize_window_dimensions(int& width, int& height, int min_w = 320, int min_h = 240);

// Computes DPI / drawable scale factor (pixel size / logical size).
// Returns true on valid dimensions, false on fallback (e.g. logical <= 0).
bool calculate_drawable_scale(int logical_w, int logical_h, int pixel_w, int pixel_h, float& scale_x, float& scale_y);

// Computes target aspect ratio for guest game rendering.
// Guarantees no division by zero or negative dimensions, preserving 4:3 baseline.
float calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio ar_option, int width, int height, float original_aspect = 4.0f / 3.0f);

// Computes cutscene aspect ratio according to configured mode.
// Guarantees safe aspect computation without zero division.
float calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode mode, int width, int height, float original_aspect = 4.0f / 3.0f);

// Handles SDL_WINDOWEVENT events to track focus, resize, minimize, display change, etc.
void handle_window_event(const SDL_WindowEvent& event, WindowState& state);

// Global window state accessor for TooieRecomp frontend.
WindowState& get_window_state();

} // namespace tooie_window

#endif // TOOIE_WINDOW_H
