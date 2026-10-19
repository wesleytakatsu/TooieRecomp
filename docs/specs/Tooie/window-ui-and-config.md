---
title: "TooieRecomp Spec — Windowing, User Interface and Configuration"
description: "Comprehensive technical specification for SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, window, ui, config, vulkan, fallback]
---

# Windowing, User Interface and Configuration Specification

## 1. Subsystem Architecture

The presentation and configuration framework integrates `SDL2`, `ultramodern`, and `RecompUI` to provide display initialization, error recovery, and in-game configuration menus.

```text
Host Display Server (X11 / Wayland)
                 |
                 v
SDL2 Window Lifecycle (src/game/tooie_window.cpp)
                 |
                 +---> Vulkan Driver Check & Error Fallback (src/game/tooie_error.cpp)
                 |
                 v
RecompUI In-Game Menu & Event Loop
                 |
                 +---> User Settings (graphics.json / input.json)
                 |
                 v
RT64 Viewport & Swapchain Presentation
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-WIN-01** | Multi-Display Enumeration | `validate_display_index` clamps index within `[0, num_displays - 1]`. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-WIN-02** | Window Dimension Sanitizer | `sanitize_window_dimensions` enforces min resolution (320x240). | `IMPLEMENTED` / `VALIDATED` |
| **REQ-WIN-03** | DPI / Drawable Scaling | `calculate_drawable_scale` computes correct logical-to-pixel scale. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-WIN-04** | Aspect Ratio Safeguards | Prevents zero-division; maintains 4:3 baseline for original mode. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-WIN-05** | Vulkan Error Fallback | Displays structured error dialog on Vulkan failure without crashing. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-WIN-06** | Configuration Persistence| Serializes settings as JSON and restores them on startup. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Interface Contracts & C++ Declarations

### 3.1 Window Management Interface (`include/tooie_window.h`)
```cpp
namespace tooie_window {
    // Validates display index against available display count.
    int validate_display_index(int display_index, int num_displays);

    // Sanitizes dimensions to ensure strictly positive values (min 320x240).
    void sanitize_window_dimensions(int& width, int& height, int min_w = 320, int min_h = 240);

    // Computes DPI / drawable scale factor (pixel size / logical size).
    bool calculate_drawable_scale(int logical_w, int logical_h, int pixel_w, int pixel_h, float& scale_x, float& scale_y);

    // Computes target aspect ratio for 3D game rendering.
    float calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio ar_option, int width, int height, float original_aspect = 4.0f / 3.0f);

    // Computes aspect ratio for cutscenes according to configured mode.
    float calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode mode, int width, int height, float original_aspect = 4.0f / 3.0f);
}
```
*Verification*: Validated by `tests/window_display_test.cpp` (38/38 assertions PASS).

### 3.2 Error Reporting & Driver Fallback (`include/tooie_error.h`)
```cpp
namespace tooie_error {
    enum class ErrorClass {
        WINDOW_INIT_FAILED,
        NO_VULKAN,
        NO_COMPATIBLE_GPU,
        VULKAN_DEVICE_INIT_FAILED,
        RT64_INIT_FAILED,
        AUDIO_INIT_FAILED,
        UNKNOWN_GRAPHICS_ERROR
    };

    struct ErrorInfo {
        ErrorClass error_class;
        std::string user_title;
        std::string user_message;
        std::string technical_detail;
    };

    ErrorInfo format_error(ErrorClass error_class, const char* detail = nullptr);
    ErrorInfo format_setup_result_error(ultramodern::renderer::SetupResult result, const char* detail = nullptr);
    void report_error(const ErrorInfo& info, bool show_dialog = true);
    void report_window_init_failure(const char* detail = nullptr);
    void report_graphics_init_failure(ErrorClass error_class, const char* detail = nullptr);
    const char* error_class_to_string(ErrorClass error_class);
}
```
*Verification*: Validated by `tests/graphics_failure_test.cpp` (exit 0) and simulated driver failure injection (`p08-t07-t08-frontend-system-validation.md`).

---

## 4. RecompUI Configuration System

### 4.1 Configuration Files & Layout
Configuration files are stored in the user application data directory:
- `graphics.json`:
  - `display_mode`: `Fullscreen`, `Borderless`, `Windowed`.
  - `resolution`: Target window dimensions (e.g. `1920x1080`).
  - `aspect_ratio`: `Original (4:3)`, `16:9`, `16:10`, `21:9`.
  - `refresh_rate_target`: Manual integer (20..240 step 1, default 60).
  - `vsync`: Boolean.
  - `high_precision_fb`: Boolean.
- `input.json`: Controller bindings, stick deadzones, camera inversion modes.

### 4.2 Invariants & Pacing Boundaries
- The UI manual selector allows inputs between 20 and 240 FPS.
- As specified in [[graphics-and-presentation]], setting a value below native VI rates (e.g. 20 or 30) currently presents at native speed because RT64 does not implement presentation decimation for below-native targets.
