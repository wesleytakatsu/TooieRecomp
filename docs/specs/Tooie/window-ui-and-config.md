---
title: "TooieRecomp Spec — Windowing, User Interface and Configuration"
description: "Technical specification for SDL2 window lifecycle, graphics driver failure fallback, and RecompUI configuration integration in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-31
tags: [specs, tooierecomp, window, ui, config, vulkan, fallback, menu-policy, forced-refresh, resolution, dev-mode, warp]
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
| **REQ-DEV-01** | Dev Mode tab | ESC menu tab locked behind an accepted warning; Warp saves, renames, deletes and teleports to player positions; Cheats are triggered by the Debug Button plus a D-pad direction. | `IMPLEMENTED` |

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
  - `wm_option`: `Windowed` or `Fullscreen`. F11 / Alt+Enter commits and saves the new mode immediately, so the
    next launch restores it. The shortcut writes committed graphics values only; unrelated pending edits remain
    pending until Apply. Menu changes retain their Apply/Discard workflow.
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
  `language` ("Game Language", `English`/`PortugueseBrazil`/`French`/`German`/`Spanish`, default `English`): labels are
  stored pre-uppercased because the menu's uppercase transform does not handle accented letters. A change callback
  (`tooie_language_import_ui::on_language_change`) runs the European-ROM import for FR/DE/ES; the pack itself is applied at
  ROM load, so a change made while the game runs takes effect after a restart. See [language packs](language-packs.md).
- **sound.json**: `main_volume` (frontend master volume) and `bgm_volume` (synthesizer sequence volume, 0–100).
- **mods.json**: Mod management infrastructure (`enabled_mods`, `mod_order`).
- **dev_warps.json**: positions saved by the Dev Mode warp (section 4.5); written only when the user saves, renames or
  deletes one.
- **dev_cheats.json**: Dev Mode cheat settings (section 4.5), `{"version": 1, "fly": {"enabled", "direction"},
  "blaster": {"enabled", "direction"}, "two_players": false, "character_menu": true,
  "character_menu_direction": "down"}` with `direction` one of `up`, `down`, `left`, `right`;
  written when a cheat, its direction or the experimental two-player option changes. A missing or malformed setting
  keeps its default (cheats and two players off; Fly up, Blaster right).

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
   - On Windows, the ROM-backed audio preview owns a hidden window. RT64 fullscreen/windowed transitions preserve
     its visibility and do not activate it. Changing window mode does not make an invisible preview into a second
     visible game window. Visible game windows retain their normal mode transitions and restored bounds.
   - This is enforced in `ApplicationWindow::setFullScreen` by retaining `WS_VISIBLE` and suppressing show/activation
     operations for hidden windows. It does not rewrite the user's saved fullscreen preference or change manual
     Start Game / explicit `TOOIE_AUTO_START` behavior. Regression: `rt64_hidden_window_test` (Windows only).

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

### 4.5 Dev Mode Tab

The last ESC menu tab, "Dev Mode" (`tooie_dev_mode_ui::create_tab`, `src/game/tooie_dev_mode_ui.cpp`), holds tools
that change the game's state directly.

- **Lock**: the tab first shows a warning (the tools can soft-lock the game, corrupt progress or crash it) and an
  "Accept and unlock" button. Acceptance lasts until the game is closed; it is not saved.
- **Game functions** (after unlocking): **Warp**, which opens a modal over the tab.
- **Warp modal**:
  - Shows the map player 1 is in, or that Banjo is not in a level (title screen, pause, loading, cutscene).
  - **Save current position** stores the current map, entrance, position and facing under a default name
    (`<map name> #<n>`; map names come from `decomp/include/maps.h` through `tools/generate_map_names.py`).
  - Each saved position has **Teleport**, **Rename** (inline text field) and **Delete** (confirmation prompt).
  - Teleport closes the menu. In the same map the player is moved at once; in another map the game loads it through
    the saved entrance, as warp pads do, and the player is moved 2 s after the new map has loaded (after the fade-in
    and the walk from the entrance). The load counts only after the game has taken the map change request (its
    pending-warp flag cleared), so a hitch while the old map fades out is not mistaken for it. A teleport that cannot
    start or finish within 30 s is dropped (`[tooie-warp] ... timed out` on stderr).
- **Storage**: `<config_path>/dev_warps.json`, `{"version": 1, "positions": [{"name", "map", "entrance", "x", "y",
  "z", "yaw"}]}`, written through a temporary file. Malformed files and entries without a map are ignored.
- **Game side**: a hook on `yaw_update` (`0x8009C88C`, `src/game/tooie_dev_warp_hook.cpp`) publishes player 1's
  position on every frame of a save file or attract demo (game modes 3 and 5) and runs the teleport state machine (`tooie_dev_warp::Teleporter`); the state machine,
  the JSON format and the map names are tested by `tests/tooie_dev_warp_test.cpp`. Addresses and their certainty:
  [warp and player position](../../reverse-engineering/systems/warp-and-player-position.md).
- Not restored by a teleport: enemies, objects, timers and the camera, which follows the player after a few frames.
- **Debug Button**: a `recompinput` input (`GameInput::DEBUG`, patch `recompfrontend-debug-input`) listed with the
  other controls and bound like them; defaults R3 on a controller and T on the keyboard (`src/game/config.cpp`). It never
  reaches the N64 controller, but a physical button bound to it keeps its other bindings (R3 is also N64 L by default).
  `recompinput::profiles::get_game_input_held` reads it with the issuing player's N64 input profile. Physical R3 is also
  accepted directly for the selector and Fly/Blaster shortcuts, including profiles without a Debug Button binding.
- **Cheats** (below Warp after unlocking): one line per cheat with a switch, its name and a D-pad direction selector.
  Holding the Debug Button and pressing that direction triggers the cheat once per press, only while the tab is
  unlocked in this session and the cheat is switched on (`tooie_dev_cheats`, `src/game/tooie_dev_cheats.cpp`; chord,
  request queue, allowed states and settings tested by `tests/tooie_dev_cheats_test.cpp`). The input callback in
  `src/main/main.cpp` detects the chord separately for J1/J2; the per-player update (`0x800991B0`) applies each request
  to its owner's `PlayerState` through `tooie_dev_cheats_hook.cpp`. Settings/unlock are shared; button-edge history,
  pending actions and Blaster exit cleanup are per player. Simultaneous shortcuts do not consume each other's requests.
  The character selector takes input priority. New maps/session reset discard pending actions/cleanup; held-button
  history is retained until release so resetting does not replay the chord. Settings changes discard queued requests,
  and execution rechecks unlock/enable. A Blaster exit already in progress still finishes its model cleanup after the
  cheat is disabled. Requests during loading, Warp or a pending map transition are dropped.
  This uses the per-player update because the game skips `yaw_update` in the Blaster. Disallowed states are logged as
  `[tooie-cheat] player=<1|2> <cheat> ignored (transformation 0x.., state 0x..)` on stderr.
  - **Fly** (default D-pad up): puts the issuing player in the launch state a flight pad uses (`0x23`), and the game goes on to
    flying by itself. Allowed only for Banjo and Kazooie (transformation `0x1`) idle or walking (`0x1`, `0x2`, `0x4`).
    J1 validated by the user on Windows (2026-10-30); J2 accepted by the user on Windows (2026-10-31), with host routing tests.
  - **Breegull Blaster** (default D-pad right): switches the first-person Blaster on or off. On: the game's enter state
    (`0xC8`) from Banjo and Kazooie idle or walking. Off: the game's exit state (`0xC9`) from the Blaster's idle or
    movement states (`0xCA`, `0xCC`, `0xCE`, `0xD1`); once the exit has restored transformation `0x1`, the hook removes
    the Blaster's second model and reloads the model of the current transformation, because the game normally leaves
    the Blaster only through a map change, which builds the player again. The Warp does not step while the Blaster is
    on (it runs from `yaw_update`). The exit cleanup removes/reloads only the issuing player's models. J1 validated by
    the user on Windows (2026-10-30); J2 accepted by the user on Windows (2026-10-31), with host routing/cleanup tests.
- **Per-player transformation selector** (after Dev Mode unlock): the **Character selection** section contains an
  **Enable character selection** toggle (default on, persisted as `character_menu`), a D-pad direction dropdown
  (`character_menu_direction`, default `down`) and
  **Open for Player 1/2** buttons. These enable the selector, close ESC and queue opening for the selected active
  player, consuming the button's closing input until release. No map reload is required after unlocking/enabling.
  All three shortcut rows use switch/name/description/direction alignment. Dropdowns reserve 224 dp for text and caret;
  player actions use equal-width columns and wrap on narrow layouts. The Dev Mode content scrolls vertically when
  needed. Player status is a snapshot when the tab is built; no continuous status polling is added.
  Holding physical R3 or the mapped Debug Button (default R3/T) and pressing
  the configured D-pad direction opens the issuing player's selector, independently for J1/J2. This chord takes
  priority over a cheat configured to the same direction. Missing or invalid saved directions retain the Down default.
  Holding the opening Up/Down press does not immediately navigate the list. D-pad up/down or the left stick navigates; A confirms and B cancels. Both players can open
  selectors simultaneously. Only the owner's game input is consumed, including the closing button until release;
  there is no global game pause. The selector uses a non-capturing overlay clipped to the native viewport layout.
  A single player's viewport covers the full game screen. ESC/global UI temporarily hides the selector.
  - Catalog: 17 entries. Banjo/Kazooie is first and Banjo/Dragon Kazooie second, followed by Banjo, Kazooie,
    solo Dragon Kazooie, Mumbo, Snowball, Bee, Washing Machine, Stony, Breegull Blaster, Submarine, Golden Goliath,
    Detonator, Van and Small/Large T. rex. Clockwork Kazooie is excluded after a reported crash when replacing the owner player;
    native Clockwork egg/remote mechanics are unchanged. There is no map availability filter in this catalog.
    The overlay has a full-width header, seven fixed-height non-wrapping name rows, and a separate A/B footer.
  - Selecting Golden Goliath seeds its native entry duration with 120 seconds. The temporary transition seed is
    restored after entry; the native countdown/expiry continues without a per-frame refill. Humba/Mumbo's original
    transformation flows and spell-range rules are unchanged.
    Simultaneous Goliaths still use the game's shared HUD timer 2; separate countdowns are not implemented.
  - The game-thread request changes the owner's native form, models, animation and default behavior state. Each
    player's native transition data is captured and selected during the next player allocation/setup. Native changes
    and reversions remain authoritative; the selector does not continuously reapply a previous selection.
    Humba's native spell range is unchanged. The selected form is session state; only the menu enable switch is saved.
    Combined normal/Dragon selections that retain primary model `0x607` explicitly refresh the owner's auxiliary
    variant through the native selector, since the primary-model setter skips same-ID reloads. This runs only on
    menu confirmation; it does not reload the other player or continuously reapply the choice.
  - New maps/session reset discard open menus and old confirmations. Requests during loading are dropped. Native
    multiplayer and attract mode do not activate the selector. `TOOIE_TEST_PLAYER_FORMS=0` disables the new integration;
    `TOOIE_DIAG_PLAYER_FORMS=1` enables allocation/selection/transition events on stderr (default OFF).
  - RE and proof boundary: [player transformations](../../reverse-engineering/systems/player-transformations.md).
    The user approved the revised selector on Windows. Host harness/build validation and that approval do not prove
    all special characters' gameplay in arbitrary maps; exhaustive manual testing remains pending.
- **Experimental: Two players (split screen)**: default off, persisted as the boolean `two_players`. Requires the
  session warning to be accepted, Multiplayer enabled in General and two players assigned in Controls. Player creation
  applies the option on the next eligible map load; game select and the excluded cutscene maps remain unchanged.
  The map-creation hook (`0x800F73C4`) changes an original request for one player to two, adds a second viewport when
  the map has one and enables reads from two controller ports. Player 2 is placed beside player 1, then placed again
  2 s later once player 1's position has been still for 0.2 s, 60 game units above player 1; on its next update it leaves
  waiting state `0x74` for its own native default state (`0x1` for Banjo/Kazooie). The active-player hook
  (`0x800F65D0`) admits player 2 for collisions while the option is active.
  Transforming J2 through the menu before the delayed release cancels the later repositioning.
  Warp also places player 2 beside player 1 while the option is active. Turning the option off does not destroy
  player 2 or its viewport in the current map; creation is reconsidered on the next map load, while these update,
  collision and Warp hooks consult the current option.
  - **Lighting outputs**: when this map's original one-player creation request is expanded to two, primary, LOD and
    secondary player-model setup request one native private output vertex buffer for CPU-lit models that originally
    requested none. The scope also covers initialization before the players enter the slot table. Existing private
    buffers and native instance teardown/defragmentation retain their behavior. Other callers, unlit models and
    original multiplayer creation requests retain their allocation arguments. This prevents one player's CPU lighting
    from overwriting the other's output colors; `TOOIE_TEST_2P_PRIVATE_VERTICES=0` disables it for A/B reproduction.
    Mechanism and proof boundary: [player lighting](../../reverse-engineering/systems/player-lighting.md).
    `TOOIE_DIAG_2P_LIGHTING_LOG=<file>` optionally logs map/model allocation events and output pointers; default OFF,
    without per-frame probes. `tools/windows/Test-SplitScreenLighting.ps1` starts a manual capture with backups,
    stdout/stderr and build metadata, preserving normal session unlock and player-assignment requirements.
  - User playtests recorded in the [split-screen handoff](../../future/two-player-split-screen.md) report two
    controlled players, shared map changes, item collection and attacks by player 2. This is an experimental checkpoint,
    with no complete cooperative-gameplay guarantee.
  - Known limits: enemies do not target player 2; player 2 cannot trigger doors/map changes; player 2's death kills the
    shared player. Debug Button cheats act on their issuing player. Camera orientation needs rechecking. Private lighting
    outputs passed the instrumented Windows attract-map discriminator; the user confirmed the graphical lighting
    problem resolved in story mode on Windows (2026-10-31). The secondary-model/transform playtest and Linux remain
    `NOT_RUN`.
