---
title: "TooieRecomp Spec — Windowing, User Interface and Configuration"
description: "Technical specification for SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, window, ui, config, vulkan, fallback, menu-policy, forced-refresh, resolution]
---

# Windowing, User Interface and Configuration Specification

## 1. Subsystem Architecture

The presentation and configuration framework integrates `SDL2`, `ultramodern`, and `RecompUI` to manage windowing, error recovery, and in-game configuration on Linux (X11/Wayland) and Windows (Win32).

```text
Host Windowing System (X11 / Wayland / Win32)
                 |
                 v
SDL2 Window Lifecycle (src/game/tooie_window.cpp, src/main/main.cpp)
                 |
                 +---> Driver Check & Error Fallback (src/game/tooie_error.cpp)
                 |
                 v
RecompUI In-Game Menu & Event Loop (include/tooie_menu_policy.h, src/game/config.cpp)
                 |
                 +---> Policy Enforcement & Dynamic Config Resolution
                 +---> Atomic Window Resize Queue (UI Thread -> SDL Event Thread)
                 +---> User Settings (graphics.json / controls.json / sound.json)
                 |
                 v
RT64 Viewport & Swapchain Presentation
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-WIN-01** | Multi-Display Enumeration | `validate_display_index` clamps index within `[0, num_displays - 1]`. | `IMPLEMENTED` |
| **REQ-WIN-02** | Window Dimension Sanitizer | `sanitize_window_dimensions` enforces min resolution (320x240). | `IMPLEMENTED` |
| **REQ-WIN-03** | DPI / Drawable Scaling | `calculate_drawable_scale` computes correct logical-to-pixel scale. | `IMPLEMENTED` |
| **REQ-WIN-04** | Aspect Ratio Safeguards | Prevents zero-division; maintains 4:3 baseline for original mode. | `IMPLEMENTED` |
| **REQ-WIN-05** | Graphics Error Fallback | Displays structured error dialog on initialization failure without crashing. | `IMPLEMENTED` |
| **REQ-WIN-06** | Configuration Persistence| Serializes settings as JSON and restores them on startup. | `IMPLEMENTED` |
| **REQ-WIN-07** | Launcher Wordmark & UI | Host launcher renders restored vector wordmark and vendored UI assets. | `IMPLEMENTED` |
| **REQ-WIN-08** | Aspect Ratio Auto-Resize | Selecting 4:3 or 16:9 in windowed mode resizes window on SDL event thread. | `IMPLEMENTED` |
| **REQ-CFG-01** | Menu Policy Enforcement | Interpolation OFF enforces Manual 30; ON unlocks user framerate controls. | `IMPLEMENTED` |
| **REQ-CFG-02** | Resolution Scaling | Exposes `Original3x` and `Original4x` in UI and config serialization. | `IMPLEMENTED` |

---

## 3. Interface Contracts & C++ Declarations

### 3.1 Window Management Interface (`include/tooie_window.h`)
```cpp
namespace tooie_window {
    int validate_display_index(int display_index, int num_displays);
    void sanitize_window_dimensions(int& width, int& height, int min_w = 320, int min_h = 240);
    bool calculate_drawable_scale(int logical_w, int logical_h, int pixel_w, int pixel_h, float& scale_x, float& scale_y);
    float calculate_target_aspect_ratio(ultramodern::renderer::AspectRatio ar_option, int width, int height, float original_aspect = 4.0f / 3.0f);
    float calculate_cutscene_aspect_ratio(banjo::CutsceneAspectRatioMode mode, int width, int height, float original_aspect = 4.0f / 3.0f);
}
```
Validated via `tests/window_display_test.cpp`.

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
    void report_error(const ErrorInfo& info, bool show_dialog = true);
    void report_window_init_failure(const char* detail = nullptr);
    void report_graphics_init_failure(ErrorClass error_class, const char* detail = nullptr);
    const char* error_class_to_string(ErrorClass error_class);
}
```
- Suppressing GUI Dialogs: Setting `TOOIE_NO_GUI_DIALOG` suppresses host GUI error dialogs and routes diagnostics directly to console (`tooie_error.cpp`).
- Validated via `tests/graphics_failure_test.cpp`.

### 3.3 Menu Policy & Cross-Thread Window Resizing (`include/tooie_menu_policy.h`)
```cpp
namespace tooie_menu {
    constexpr uint32_t kForcedRefreshRate = 30;
    constexpr uint32_t kInterpolationDefaultRate = 60;

    struct WindowSize {
        int w;
        int h;
    };

    double window_aspect_target(ultramodern::renderer::AspectRatio ar);
    WindowSize aspect_window_size(double aspect, int cur_w, int cur_h, int usable_w, int usable_h);

    std::atomic<double>& window_aspect_request_slot();
    void request_window_aspect(double target);
    double take_window_aspect_request();

    RefreshChoice coerce_refresh(bool interpolation_on, RefreshChoice cur);
    void install_interpolation_policy(std::function<recomp::config::Config&()> resolve,
                                      const std::string& rr_id, const std::string& rr_manual_id,
                                      const std::string& interp_id, std::function<void(bool)> sink);
}
```
Validated via `tests/tooie_menu_policy_test.cpp` and `tests/tooie_menu_config_test.cpp`.

---

## 4. RecompUI Configuration System

### 4.1 Configuration Files & Layout
Configuration files are stored under `<config_path>/` (on Windows: `%LOCALAPPDATA%\TooieRecompiled`):
- **graphics.json**:
  - `res_option`: `Original`, `Original2x`, `Original3x` (720p), `Original4x` (960p), `Auto`.
  - `ds_option`: Downsample factor (`0`, `2`, `4`), disabled when `res_option` is `Auto`.
  - `ar_option`: `Original` (4:3) and `Expand` (16:9).
  - `wm_option`: `Windowed` or `Fullscreen`.
  - `rr_option` & `rr_manual_value`: Locked to Manual 30 while `interpolation` is false; unlocked when `interpolation` is true.
  - `interpolation`: Boolean (default `false`). When false, forces CURRENT endpoint fallback; when true, unlocks high-framerate interpolation.
  - `show_fps_counter`: Boolean toggle for top-left present FPS counter.
  - Hidden options: `api_option`, `hpfb_option`, `developer_mode`.
- **controls.json**: Controller mapping profiles, bindings, and `players` array persisting per-slot assignments
  (controller identity with hardware serial/GUID hash, or null, and keyboard toggle).
- **general.json**: `rumble_strength`, `joystick_deadzone`, `background_input_mode`, and `multiplayer_mode`
  ("Multiplayer (up to 4 players)", `Off`/`On`, default `Off`). Evaluated once at startup in `src/main/main.cpp`:
  `On` activates `recompinput::players::set_single_player_mode(false)` and `set_player_count_range(1, 4)`;
  `Off` preserves default single-player mode (merging all host controllers and keyboard into player 1).
- **sound.json**: `main_volume` (frontend master volume) and `bgm_volume` (synthesizer sequence volume, 0–100).
- **mods.json**: Mod management infrastructure (`enabled_mods`, `mod_order`).

### 4.2 Invariants & Operational Boundaries
1. **Config Pointer Safety**:
   - RecompUI stores configuration sections in dynamically growing vectors. Policy enforcement functions resolve configuration objects dynamically on each callback to avoid holding dangling references across vector reallocations.
2. **Disabled Option Guard**:
   - Disabled options intercept value changes in `ConfigPageOptionsMenu::on_set_option_value` and `Slider::do_step`, preventing modifications via drag or keypresses.
3. **Cross-Thread Window Resizing**:
   - UI callbacks push requested aspect changes to `request_window_aspect`. Dimensions are applied strictly on the SDL event thread in `update_gfx` (`src/main/main.cpp`), clamped to display usable bounds (`SDL_GetDisplayUsableBounds`).
4. **Launcher Wordmark & UI Assets**:
   - Restored vector wordmark is loaded from `ui_assets/TooieLogo.svg`.
   - UI assets (icons, character art, fonts) are vendored in `assets/` and synchronized to the build directory by CMake target `TooieUIAssets`.
5. **Launcher Music Controls**:
   - Launcher background music preview is controlled via command-line flags and environment switches:
     - `TOOIE_AUTO_START`: Skips launcher GUI.
     - `TOOIE_DISABLE_LAUNCHER_MUSIC`: Disables launcher preview audio.
     - `--launcher-music-child` and `--launcher-music-control`: Coordinates music playback process.

### 4.3 Menu Option Availability
- `note_saving_mode`: Hidden from UI (`hidden = true`).
- `analog_cam_mode`, `analog_camera_sensitivity`: Disabled (`UI_DISABLED`).
- `third_person_camera_invert_mode`, `first_person_invert_mode`, `flying_and_swimming_invert_mode`: Disabled (`UI_DISABLED`).
- `cutscene_aspect_ratio_mode`: Disabled (`UI_DISABLED`).
- `hr_option`: HUD placement option disabled (`UI_DISABLED`).

### 4.4 Multiplayer Controls Interface

When `multiplayer_mode` is enabled (`On`), the Controls configuration page switches to a dedicated 4-player configuration view:
- **Player Cards (2x2 Grid)**:
  - 4 cards representing player slots 1 through 4.
  - Each card provides:
    - **Controller Selector**: Dropdown listing "None" and all detected gamepads formatted as `<ordinal>: <SDL name>`.
    - **Rumble 1 s Button**: Triggers a 1000 ms rumble test at full strength, updating an adjacent status readout
      ("Rumbling…", "No rumble", "Rumble failed", or "Press a button on it first" when SDL's RAWINPUT driver requires
      initial button correlation on Windows Bluetooth pads).
    - **Keyboard Toggle**: Checkbox attaching host keyboard inputs to the selected player slot.
    - **Input Activity Line**: Displays "Input: Controller N" or "Input: Keyboard", illuminated for ~250 ms upon
      receiving input from the respective device.
    - **Edit Mapping**: Navigation button opening the per-player mapping configuration screen.
- **Connected Devices List**:
  - Displayed below the player cards, enumerating Keyboard and each connected physical controller, assigned player
    slot, and real-time input activity.
- **Mapping View & Profile Reset Scope**:
  - For player slots configured with both a controller and keyboard, the mapping view provides a device toggle to
    switch between controller and keyboard layouts.
  - "Reset to defaults" strictly resets the profile currently being edited (`mp_player_<N>_controller` or
    `mp_player_<N>_keyboard`) without altering the player's other device profile.
- **Assignment Modal & Focus Safety**:
  - The "Assign players" modal remains available as a quick-setup workflow to populate unassigned slots.
  - `ContextId::get_focused_element` skips empty slots and non-element resources during UI element construction,
    preventing crashes when navigating back from mapping screens.