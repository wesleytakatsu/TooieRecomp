---
title: "TooieRecomp Spec — Windowing, User Interface and Configuration"
description: "Comprehensive technical specification for SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-19
tags: [specs, tooierecomp, window, ui, config, vulkan, fallback, menu-policy, forced-refresh, resolution]
---

# Windowing, User Interface and Configuration Specification

## 1. Subsystem Architecture

The presentation and configuration framework integrates `SDL2`, `ultramodern`, and `RecompUI` to provide display initialization, error recovery, and in-game configuration menus.

```text
Host Display Server (X11 / Wayland)
                 |
                 v
SDL2 Window Lifecycle (src/game/tooie_window.cpp, src/main/main.cpp)
                 |
                 +---> Vulkan Driver Check & Error Fallback (src/game/tooie_error.cpp)
                 |
                 v
RecompUI In-Game Menu & Event Loop (include/tooie_menu_policy.h, src/game/config.cpp)
                 |
                 +---> Policy Enforcement & Dynamic Config Resolution
                 +---> Atomic Window Resize Queue (UI Thread -> SDL Event Thread)
                 +---> User Settings (graphics.json / controls.json)
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
| **REQ-WIN-07** | Launcher Wordmark & Asset Isolation | Host launcher presents a native two-line "Banjo-Tooie" / "Recompiled" wordmark from a repository-owned SVG (`ui_assets/TooieLogo.svg`), copied to `build/ui_assets/` via CMake to preserve shared BanjoRecomp assets without in-place mutation. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/menu-title-tooie-wordmark-20261019]]) |
| **REQ-WIN-08** | Aspect Ratio Window Auto-Resize | Selecting `Original` (4:3) or `Expand` (16:9) in windowed mode resizes the window, preserving height and clamping within display usable bounds on the SDL event thread. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/p08-settings-functionality-20261019]]) |
| **REQ-CFG-01** | Menu Policy Enforcement & Disabled Protection | Disabled options cannot be stepped/clicked via widgets; forced 30 FPS cap is enforced on load/set/save paths, surviving frontend config vector relocations. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/p08-settings-functionality-20261019]]) |
| **REQ-CFG-02** | Resolution 3x/4x & Downsample Multiplier | Exposes `Original3x` (720p) and `Original4x` (960p) in UI and config serialization, multiplying downsampling correctly while disabling DS in Auto. | `IMPLEMENTED` / `VALIDATED` (see [[../../plan/evidence/p08-settings-functionality-20261019]]) |

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

### 3.3 Menu Policy & Cross-Thread Window Resizing (`include/tooie_menu_policy.h`)
```cpp
namespace tooie_menu {
    constexpr uint32_t kForcedRefreshRate = 30;

    struct WindowSize {
        int w;
        int h;
    };

    // Computes target aspect ratio for window resizing; returns 0 for non-resizing modes.
    double window_aspect_target(ultramodern::renderer::AspectRatio ar);

    // Derives width from aspect keeping height, clamping to usable display bounds.
    WindowSize aspect_window_size(double aspect, int cur_w, int cur_h, int usable_w, int usable_h);

    // Cross-thread window aspect request queue (UI thread stores, SDL event thread consumes).
    std::atomic<double>& window_aspect_request_slot();
    void request_window_aspect(double target);
    double take_window_aspect_request();

    // Enforces forced refresh rate on load and modification callbacks.
    // resolve functor looks up Config on each call to prevent dangling references across std::vector reallocations.
    void install_forced_refresh(std::function<recomp::config::Config&()> resolve, const std::string& rr_id, const std::string& rr_manual_id);
}
```
*Verification*: Validated by `tests/tooie_menu_policy_test.cpp` and `tests/tooie_menu_config_test.cpp` (both PASS exit 0).

---

## 4. RecompUI Configuration System

### 4.1 Configuration Files & Layout

- **graphics.json**:
  - `res_option`: `Original`, `Original2x`, `Original3x`, `Original4x`, `Auto`. IDs legados 0, 1 e 2 preservados; 3x e 4x adicionados ao final. Rótulos nominais baseados em 240p (3x: 720p, 4x: 960p); geometria real depende de VI e enquadramento.
  - `ds_option`: fator `0`, `2`, `4`, multiplica a base escolhida; desativado em `Auto`.
  - `ar_option`: na UI, `Original` e `Expand`; aplicação em modo janela redimensiona para 4:3 ou 16:9.
  - `wm_option`: `Windowed` ou `Fullscreen`.
  - `rr_option` e `rr_manual_value`: temporariamente forçados em `Manual` e `30`.
  - `msaa_option`: configuração de MSAA.
  - `hr_option`: HUD Placement, desativado.
  - `cutscene_aspect_ratio_mode`: proporção de cutscenes, desativada.
  - `show_fps_counter`: booleano.
  - Configurações internas de frontend ocultas: `api_option`, `hpfb_option`, `developer_mode`.
- **controls.json**: perfis de controle e mapeamentos de entrada.
- **general.json**: `rumble_strength`, `joystick_deadzone`, `background_input_mode`, além de chaves legadas preservadas (ocultas/desativadas).
- **sound.json**: `main_volume` e `bgm_volume` (0–100), com controle atômico espelhado para BGM.
- **mods.json**: `enabled_mods`, `mod_order` e `latest_game_mode` (infraestrutura; sem reivindicação de QA para mods instalados).


### 4.2 Invariants & Policy Boundaries
- **30 FPS Policy Lock**: Refresh rate is forced to Manual 30 FPS on all config load, change, and save operations. This is a user policy cap due to the CURRENT `tfPos` workaround (`tooie_rsp_endpoint_current`), which repeats vertex poses above native rates. RT64 is capable of 60+ Hz presentation, but higher rates are capped until continuous interpolation is implemented.
- **Config Pointer Relocation Protection**: RecompUI stores configuration sections in a `std::vector<std::pair<std::string, Config>>`. Capturing a `Config&` reference at registration time leads to a dangling pointer when the vector reallocates as subsequent tabs are added. Enforcement routines must resolve `recompui::config::get_graphics_config()` dynamically on every invocation.
- **Disabled Option Guard & Numeric Style Fix**: Options flagged as disabled are intercepted by `ConfigPageOptionsMenu::on_set_option_value` and `Slider::do_step`, preventing values from changing on click, drag, or keyboard navigation and snapping UI elements back. Additionally, `ConfigOptionNumber::update_disabled` calls `slider->set_enabled(!disabled)` to guarantee that numeric sliders visually render disabled rather than retaining active styling.
- **Cross-Thread Window Operations**: UI callbacks enqueue window aspect requests via `tooie_menu::request_window_aspect(target)`. SDL window functions (`SDL_SetWindowSize`, `SDL_SetWindowPosition`, `SDL_RestoreWindow`) are executed strictly on the SDL event thread in `update_gfx` (`src/main/main.cpp`). Fullscreen and borderless modes ignore window resize requests.

### 4.3 Launcher Wordmark & Asset Isolation Contract
- **Wordmark Identity**: The host launcher title displays exactly two lines: "Banjo-Tooie" (line 1, blue with yellow outline) and "Recompiled" (line 2, red with yellow outline), with artist-styled caps.
- **Repository Asset Ownership**: The vector asset is defined in `ui_assets/TooieLogo.svg` (viewBox `0 0 7300 2450`), generated via `tools/make_tooie_launcher_logo.py`.
- **Isolation Contract**: Upstream assets in `BanjoRecomp/assets/` remain pristine. A CMake custom command copies `ui_assets/TooieLogo.svg` to `${CMAKE_CURRENT_BINARY_DIR}/ui_assets/TooieLogo.svg` under target `TooieLauncherLogo`, and `src/main/launcher_animation.cpp` references `"../ui_assets/TooieLogo.svg"` with scaled dimensions `7300.0f * 0.106f` x `2450.0f * 0.106f`.
- **Evidence**: Validated for settled layouts at 1600x900 and 1024x768, and entry animation composition bounded PASS across discrete frames (smoothness/easing NOT_TESTED), in [[../../plan/evidence/menu-title-tooie-wordmark-20261019]].

### 4.4 Menu Option Availability & Status
The following options are specifically managed in `src/game/config.cpp`:
- `note_saving_mode`: Hidden from UI (`hidden = true`, default `Off`, schema preserved for config file backward compatibility).
- `analog_cam_mode`, `analog_camera_sensitivity`: Disabled in UI (`UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`).
- `third_person_camera_invert_mode`, `first_person_invert_mode`, `flying_and_swimming_invert_mode`: Disabled in UI (`UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`).
- `cutscene_aspect_ratio_mode`: Disabled in UI (`UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`).
- `hr_option`: HUD Placement, disabled in UI (`UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`).
- Full evidence and verification screenshots documented in [[../../plan/evidence/p08-settings-functionality-20261019]].
