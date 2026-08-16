#ifndef TOOIE_WINDOW_H
#define TOOIE_WINDOW_H

#include <cstdint>
#include <algorithm>
#include "ultramodern/config.hpp"
#include "banjo_config.h"

namespace tooie_window {

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

} // namespace tooie_window

#endif // TOOIE_WINDOW_H
