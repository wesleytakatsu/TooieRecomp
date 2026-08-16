#include "tooie_window.h"
#include <cstdio>
#include <cmath>

namespace tooie_window {

int validate_display_index(int display_index, int num_displays) {
    if (num_displays <= 0) {
        return 0;
    }
    if (display_index < 0 || display_index >= num_displays) {
        return 0; // Safe fallback to primary display
    }
    return display_index;
}

void sanitize_window_dimensions(int& width, int& height, int min_w, int min_h) {
    if (min_w < 1) min_w = 1;
    if (min_h < 1) min_h = 1;

    if (width < min_w) {
        width = min_w;
    }
    if (height < min_h) {
        height = min_h;
    }
}

bool calculate_drawable_scale(int logical_w, int logical_h, int pixel_w, int pixel_h, float& scale_x, float& scale_y) {
    if (logical_w <= 0 || logical_h <= 0 || pixel_w <= 0 || pixel_h <= 0) {
        scale_x = 1.0f;
        scale_y = 1.0f;
        return false;
    }
    scale_x = static_cast<float>(pixel_w) / static_cast<float>(logical_w);
    scale_y = static_cast<float>(pixel_h) / static_cast<float>(logical_h);
    if (scale_x <= 0.0f || !std::isfinite(scale_x)) scale_x = 1.0f;
    if (scale_y <= 0.0f || !std::isfinite(scale_y)) scale_y = 1.0f;
    return true;
}

float calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio ar_option, int width, int height, float original_aspect) {
    if (original_aspect <= 0.0f || !std::isfinite(original_aspect)) {
        original_aspect = 4.0f / 3.0f;
    }

    switch (ar_option) {
        case ultramodern::renderer::AspectRatio::Original:
        default:
            return original_aspect;
        case ultramodern::renderer::AspectRatio::Expand:
            if (height <= 0 || width <= 0) {
                return original_aspect;
            }
            return std::max(static_cast<float>(width) / static_cast<float>(height), original_aspect);
    }
}

float calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode mode, int width, int height, float original_aspect) {
    if (original_aspect <= 0.0f || !std::isfinite(original_aspect)) {
        original_aspect = 4.0f / 3.0f;
    }

    switch (mode) {
        case banjo::CutsceneAspectRatioMode::Original:
            return original_aspect;
        case banjo::CutsceneAspectRatioMode::Clamp16x9:
        default:
            return 16.0f / 9.0f;
        case banjo::CutsceneAspectRatioMode::Full:
            if (height <= 0 || width <= 0) {
                return original_aspect;
            }
            return std::max(static_cast<float>(width) / static_cast<float>(height), original_aspect);
    }
}

} // namespace tooie_window
