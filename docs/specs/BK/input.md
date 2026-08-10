---
title: "BK — Input & Controllers"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - input
  - controller
---

# BK — Input & Controllers

How `BanjoRecomp` sees controllers, maps the original N64 pad to modern
devices, and answers the game's input queries. The heavy lifting lives in the
`recompinput` framework (N64ModernRuntime / RecompFrontend); `src/main` and
`src/game` wire BK-specific bindings and exports.

## SDL setup (`src/main/main.cpp`)

`create_gfx()` / `main()` configure SDL2 for the whole session:

- SDL hints: `SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS=0`,
  `SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE=1`, PS5 rumble,
  `SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH=1`,
  `SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1`, DPI awareness.
- `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC)`.
- Controller mappings loaded from `recompcontrollerdb.txt` next to the
  executable (`SDL_GameControllerAddMappingsFromFile`).

## Gamepad database (`recompcontrollerdb.txt`)

SDL 2.0.16 `SDL_GameControllerDB` format, pre-filtered from
gabomdq/SDL_GameControllerDB to only **N64 and GameCube** controllers plus
whatever else was considered necessary. Each line is
`<usb-guid>,<name>,<mapping>,platform:<OS>`:

```text
03000000c82d00000290000000000000,8BitDo N64,+rightx:b9,+righty:b3,-rightx:b4,...
03000000260900008888000000000000,Cyber Gadget GameCube Controller,a:b0,...
```

Notable entries: 8BitDo N64 / 8BitDo 64 BT, official GameCube Controller,
GameCube adapters (Mayflash, Hyperkin, Nyko, NEXILUX), N64 μstein/
Hori-style pads, RetroUSB N64 RetroPort, PDP Fightpad, PowerA, Memory from
SD1/2 → emulated `C` buttons.

## `recompinput` callbacks (registered in `main`)

`ultramodern::input::callbacks_t`:

| Callback | Implementation |
| ---------- | -------------- |
| `poll_input` | `recompinput::poll_inputs()` |
| `get_input` | `recompinput::profiles::get_n64_input` |
| `end_frame` (vi) | `recompinput::update_rumble` |
| `set_rumble` | `recompinput::set_rumble` |
| `get_connected_device_info` | single-player -> `Controller` + `RumblePak`, else `None` |

`update_gfx()` runs `recompinput::handle_events()` once per frame. A single
player profile is forced with
`recompinput::players::set_single_player_mode(true)`.

## Default controller bindings (`config.cpp` `set_control_defaults`)

Made for players who don't remap. C-pad behavior is emulated on the right
stick / face buttons; the original `L` button maps to R3:

| GameInput | Default SDL binding |
| --------- | -------------------- |
| `C_DOWN`  | right-analog `Y-` named; `LEFTSHOULDER` |
| `C_UP`    | right-analog `Y+` | `RIGHTSHOULDER` |
| `C_LEFT`  | right-analog `X+` | `SDL_CURRENTTER_NORTH (X)` |
| `C_RIGHT` | right-analog `X+` | `SDL_GAMEPAD_BUTTON_EAST (B)` |
| `L` | `SDL_CONTROLLER_BUTTON_RIGHTSTICK` (R3), "unused in BK, usable by mods" |

`set_control_descriptions()` attaches the per-command help text (move,
jump/select, Z-crouch, L skip dialog, R center camera, C-Buttons, etc.) shown
in the Controls tab.

## Host exports for input (`src/game/recomp_api.cpp`)

`extern "C"` host functions callable from recompiled code / patches
(registered via `REGISTER_FUNC` → `register_base_export` in `main`):

- `recomp_update_inputs` — calls `recompinput::poll_inputs()`.
- `recomp_get_gyro_deltas(x*, y*)` / `recomp_get_mouse_deltas(x*, y*)` —
  present in code but **commented out** in `main` (`// REGISTER_FUNC`), so not
  exposed at runtime; gyro/mouse are not active in v1.0.2.
- `recomp_get_right_analog_inputs(x*, y*)` — returns 0 while
  `recompinput::game_input_disabled()`, else right analog of controller 0;
  used for the dual-analog camera.
- `recomp_set_right_analog_suppressed(s32)` — forwards to
  `recompinput::set_right_analog_suppressed` (Lets patch hooks pause the
  analog camera while e.g. aiming).

## Camera / text flags exported to patches

`patches/input.h` defines the patch-facing surface (with `DECLARE_FUNC` dummy
addresses wired by `syms.ld`):

- `RecompCameraMode` — `RECOMP_CAMERA_NORMAL` /
  `RECOMP_CAMERA_DUALANALOG`.
- `RecompAimingOverideMode` — Off / disable left stick / force right stick.
- Getters: `recomp_get_gyro_deltas`, `recomp_get_mouse_deltas`,
  `recomp_get_inverted_axes`, `recomp_get_analog_cam_sensitivity`,
  `recomp_get_analog_cam_enabled`, `recomp_get_analog_inverted_axes`,
  `recomp_get_flying_and_swimming_inverted_axes`,
  `recomp_get_first_person_inverted_axes`, `recomp_get_right_analog_inputs`,
  `recomp_set_right_analog_suppressed`.

The camera / axis-inversion patch file consuming these is
`camera_and_axis_inversion_patches.c`.

## Related

- [Runtime](runtime.md) — full `recomp_*` host export table
- [Config](config.md) — `banjo.us.rev0.toml` (recompiler config, unrelated to
  this)
- [Features](features.md) — user-facing dual analog / menus
- [Mods](mods.md) — mods can remap inputs? available via RecompUI