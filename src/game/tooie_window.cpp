#include "tooie_window.h"
#include <cstdio>
#include <cmath>

namespace tooie_window {

static WindowState g_window_state{};

WindowState& get_window_state() {
    return g_window_state;
}

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

void handle_window_event(const SDL_WindowEvent& event, WindowState& state) {
    switch (event.event) {
        case SDL_WINDOWEVENT_SHOWN:
            state.visible = true;
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            state.visible = false;
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            state.minimized = true;
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
            state.minimized = false;
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            state.focused = true;
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            state.focused = false;
            break;
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            state.logical_width = event.data1;
            state.logical_height = event.data2;
            sanitize_window_dimensions(state.logical_width, state.logical_height);
            break;
        case SDL_WINDOWEVENT_MOVED:
            // Query display index from window or coordinates
            break;
        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
            state.current_display = event.data1;
            break;
        default:
            break;
    }
}

} // namespace tooie_window
