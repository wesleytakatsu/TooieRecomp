#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "tooie_window.h"

static int g_test_count = 0;
static int g_pass_count = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_test_count++; \
    if (cond) { \
        g_pass_count++; \
    } else { \
        std::fprintf(stderr, "FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, msg); \
        std::exit(1); \
    } \
} while(0)

static void test_display_index_validation() {
    // Zero or negative total displays -> always fallback to 0
    TEST_ASSERT(tooie_window::validate_display_index(0, 0) == 0, "0 displays fallback to 0");
    TEST_ASSERT(tooie_window::validate_display_index(1, 0) == 0, "invalid display on 0 displays fallback to 0");
    TEST_ASSERT(tooie_window::validate_display_index(-1, 0) == 0, "negative display on 0 displays fallback to 0");
    TEST_ASSERT(tooie_window::validate_display_index(0, -5) == 0, "negative total displays fallback to 0");

    // Single display (num_displays = 1)
    TEST_ASSERT(tooie_window::validate_display_index(0, 1) == 0, "display 0 on 1 display is valid");
    TEST_ASSERT(tooie_window::validate_display_index(1, 1) == 0, "display 1 on 1 display out of bounds -> fallback to 0");
    TEST_ASSERT(tooie_window::validate_display_index(-1, 1) == 0, "display -1 on 1 display -> fallback to 0");

    // Multi-display (num_displays = 3)
    TEST_ASSERT(tooie_window::validate_display_index(0, 3) == 0, "display 0 on 3 displays is valid");
    TEST_ASSERT(tooie_window::validate_display_index(1, 3) == 1, "display 1 on 3 displays is valid");
    TEST_ASSERT(tooie_window::validate_display_index(2, 3) == 2, "display 2 on 3 displays is valid");
    TEST_ASSERT(tooie_window::validate_display_index(3, 3) == 0, "display 3 on 3 displays out of bounds -> fallback to 0");
    TEST_ASSERT(tooie_window::validate_display_index(99, 3) == 0, "display 99 on 3 displays -> fallback to 0");
    TEST_ASSERT(tooie_window::validate_display_index(-2, 3) == 0, "display -2 on 3 displays -> fallback to 0");
}

static void test_window_dimension_sanitization() {
    // Valid dimensions remain unchanged
    int w = 1600, h = 900;
    tooie_window::sanitize_window_dimensions(w, h);
    TEST_ASSERT(w == 1600 && h == 900, "1600x900 remains valid");

    w = 1920; h = 1080;
    tooie_window::sanitize_window_dimensions(w, h);
    TEST_ASSERT(w == 1920 && h == 1080, "1920x1080 remains valid");

    // Zero dimensions clamped to min (320x240 default)
    w = 0; h = 0;
    tooie_window::sanitize_window_dimensions(w, h);
    TEST_ASSERT(w == 320 && h == 240, "0x0 clamped to 320x240");

    // Negative dimensions clamped to min
    w = -100; h = -50;
    tooie_window::sanitize_window_dimensions(w, h);
    TEST_ASSERT(w == 320 && h == 240, "-100x-50 clamped to 320x240");

    // Sub-minimum dimensions clamped to min
    w = 100; h = 100;
    tooie_window::sanitize_window_dimensions(w, h);
    TEST_ASSERT(w == 320 && h == 240, "100x100 clamped to 320x240");

    // Custom minimum dimensions
    w = 50; h = 50;
    tooie_window::sanitize_window_dimensions(w, h, 640, 480);
    TEST_ASSERT(w == 640 && h == 480, "50x50 clamped to custom 640x480");
}

static void test_drawable_scale_calculation() {
    float sx = 0.0f, sy = 0.0f;

    // Standard 1:1 scale (100% DPI)
    bool ok = tooie_window::calculate_drawable_scale(1600, 900, 1600, 900, sx, sy);
    TEST_ASSERT(ok && std::abs(sx - 1.0f) < 1e-4f && std::abs(sy - 1.0f) < 1e-4f, "1:1 scale is 1.0");

    // 125% DPI scale
    ok = tooie_window::calculate_drawable_scale(1600, 900, 2000, 1125, sx, sy);
    TEST_ASSERT(ok && std::abs(sx - 1.25f) < 1e-4f && std::abs(sy - 1.25f) < 1e-4f, "125% DPI scale is 1.25");

    // 150% DPI scale
    ok = tooie_window::calculate_drawable_scale(1600, 900, 2400, 1350, sx, sy);
    TEST_ASSERT(ok && std::abs(sx - 1.5f) < 1e-4f && std::abs(sy - 1.5f) < 1e-4f, "150% DPI scale is 1.5");

    // 200% Retina / HiDPI scale
    ok = tooie_window::calculate_drawable_scale(1600, 900, 3200, 1800, sx, sy);
    TEST_ASSERT(ok && std::abs(sx - 2.0f) < 1e-4f && std::abs(sy - 2.0f) < 1e-4f, "200% HiDPI scale is 2.0");

    // Invalid logical dimension (0 or negative) -> safe fallback to 1.0
    ok = tooie_window::calculate_drawable_scale(0, 900, 1600, 900, sx, sy);
    TEST_ASSERT(!ok && sx == 1.0f && sy == 1.0f, "0 logical width fallback to 1.0");

    ok = tooie_window::calculate_drawable_scale(1600, -10, 1600, 900, sx, sy);
    TEST_ASSERT(!ok && sx == 1.0f && sy == 1.0f, "negative logical height fallback to 1.0");
}

static void test_aspect_ratio_safety_and_modes() {
    constexpr float orig_4_3 = 4.0f / 3.0f;

    // AspectRatio::Original always yields original 4:3 regardless of host window
    float ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Original, 1600, 900, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Original yields 4:3 on 16:9 window");

    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Original, 3840, 1080, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Original yields 4:3 on ultrawide window");

    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Original, 0, 0, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Original yields 4:3 on 0x0 window");

    // AspectRatio::Expand yields window aspect when wider than 4:3
    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Expand, 1600, 900, orig_4_3);
    TEST_ASSERT(std::abs(ar - (16.0f / 9.0f)) < 1e-4f, "AspectRatio::Expand yields 16:9 on 16:9 window");

    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Expand, 800, 600, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Expand yields 4:3 on 4:3 window");

    // AspectRatio::Expand never yields less than original aspect (e.g. portrait window clamped to 4:3)
    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Expand, 600, 800, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Expand on portrait clamped to 4:3");

    // Zero / negative window dimensions under Expand -> safe fallback without crash or NaN
    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Expand, 0, 0, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Expand on 0x0 safely returns 4:3");

    ar = tooie_window::calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio::Expand, -100, -50, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "AspectRatio::Expand on negative dimensions safely returns 4:3");
}

static void test_cutscene_aspect_ratio_modes() {
    constexpr float orig_4_3 = 4.0f / 3.0f;
    constexpr float clamp_16_9 = 16.0f / 9.0f;

    // CutsceneAspectRatioMode::Original
    float ar = tooie_window::calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode::Original, 1600, 900, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "Cutscene Original mode returns 4:3");

    // CutsceneAspectRatioMode::Clamp16x9
    ar = tooie_window::calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode::Clamp16x9, 1600, 900, orig_4_3);
    TEST_ASSERT(std::abs(ar - clamp_16_9) < 1e-4f, "Cutscene Clamp16x9 returns 16:9");

    ar = tooie_window::calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode::Clamp16x9, 3840, 1080, orig_4_3);
    TEST_ASSERT(std::abs(ar - clamp_16_9) < 1e-4f, "Cutscene Clamp16x9 clamps ultrawide to 16:9");

    // CutsceneAspectRatioMode::Full
    ar = tooie_window::calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode::Full, 2560, 1080, orig_4_3);
    TEST_ASSERT(std::abs(ar - (2560.0f / 1080.0f)) < 1e-4f, "Cutscene Full expands to 21:9");

    // Zero / negative dimension safety
    ar = tooie_window::calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode::Full, 0, 0, orig_4_3);
    TEST_ASSERT(std::abs(ar - orig_4_3) < 1e-4f, "Cutscene Full on 0x0 returns 4:3");
}

static void test_window_events_and_focus_state() {
    tooie_window::WindowState state{};
    state.focused = true;
    state.visible = true;
    state.minimized = false;
    state.current_display = 0;

    // Focus lost event
    SDL_WindowEvent event{};
    event.event = SDL_WINDOWEVENT_FOCUS_LOST;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(!state.focused, "FOCUS_LOST updates focused to false");

    // Focus gained event
    event.event = SDL_WINDOWEVENT_FOCUS_GAINED;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(state.focused, "FOCUS_GAINED updates focused to true");

    // Minimize event
    event.event = SDL_WINDOWEVENT_MINIMIZED;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(state.minimized, "MINIMIZED updates minimized to true");

    // Restore event
    event.event = SDL_WINDOWEVENT_RESTORED;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(!state.minimized, "RESTORED clears minimized state");

    // Resize event
    event.event = SDL_WINDOWEVENT_RESIZED;
    event.data1 = 1280;
    event.data2 = 720;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(state.logical_width == 1280 && state.logical_height == 720, "RESIZED updates logical dimensions to 1280x720");

    // Resize to 0x0 clamped
    event.event = SDL_WINDOWEVENT_SIZE_CHANGED;
    event.data1 = 0;
    event.data2 = 0;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(state.logical_width == 320 && state.logical_height == 240, "SIZE_CHANGED 0x0 clamped to 320x240");

    // Display changed event
    event.event = SDL_WINDOWEVENT_DISPLAY_CHANGED;
    event.data1 = 2;
    tooie_window::handle_window_event(event, state);
    TEST_ASSERT(state.current_display == 2, "DISPLAY_CHANGED updates current_display to 2");
}

int main() {
    std::printf("=== Running tooie_window / P08-T07 Unit Tests ===\n");
    test_display_index_validation();
    test_window_dimension_sanitization();
    test_drawable_scale_calculation();
    test_aspect_ratio_safety_and_modes();
    test_cutscene_aspect_ratio_modes();
    test_window_events_and_focus_state();
    std::printf("=== PASS: %d/%d assertions passed ===\n", g_pass_count, g_test_count);
    return 0;
}
