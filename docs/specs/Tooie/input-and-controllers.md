---
title: "TooieRecomp Spec — Input and Controller Subsystem"
description: "Technical specification for SDL2 controller mapping, stick-angle thunk override (80013B70), input helper contracts (800979D8), and attract playback."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, input, controller, sdl2, stick-angle, demo]
---

# Input and Controller Subsystem Specification

## 1. Subsystem Architecture

The input subsystem bridges modern host input devices (USB/Bluetooth controllers, keyboard/mouse) into Nintendo 64 controller structures (`OSContPad`) while providing native fixes for generator stub omissions.

```text
Host Input Devices (SDL2 GameController / Keyboard)
                        |
                        v
RecompFrontend Input Mapper (recompinput::poll_inputs)
                        |
                        v
Native Stick-Angle Thunk (func_80013B70 Strong Override)
                        |
                        v
Guest Controller Pad Array & Input Buffers (0x800BCE84)
                        |
                        v
Stateful Input Helpers (func_800979D8 / bakey_held)
                        |
                        v
Banjo-Tooie Player Physics & Attract Demo Engine
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-INP-01** | Modern Controller Mapping | SDL2 GameController maps standard modern gamepads to N64 layout. | `IMPLEMENTED` |
| **REQ-INP-02** | Stick Angle Normalization | Strong override `func_80013B70` restores missing stub execution. | `IMPLEMENTED` |
| **REQ-INP-03** | Bidirectional Input | WASD keyboard and analog sticks navigate in all directions without quadrant bias. | `IMPLEMENTED` |
| **REQ-INP-04** | Input Helper Contract | `0x800979D8` evaluates held state and ring buffer `0x800BCE84`. | `IMPLEMENTED` |
| **REQ-INP-05** | Attract Parity | Attract sequences serve as regression detection suite. | `PARTIAL` |
| **REQ-INP-06** | Camera Controls | Camera inversion and analog camera bindings in guest engine. | `NOT_IMPLEMENTED` |
| **REQ-INP-07** | Local Multiplayer (up to 4) | 4-slot controller/keyboard routing, per-player profiles, and N64 port policy. | `IMPLEMENTED` (Windows, user-validated; Linux pending) |

---

## 3. Interface & Calling Contracts

### 3.1 Analog Stick Angle Normalization (`func_80013B70`)
- **Guest Address**: `0x80013B70` (`baStick_getNormalizedAngle`).
- **Defect in Static Recompilation**:
  - `func_80013B70` is a 12-byte secondary entrypoint listed in `stubs[]` within `tooie.us.rev0.recomp.toml`, causing N64Recomp to emit an empty function body.
  - Original MIPS assembly loaded the distance passed by the caller into coprocessor 1 (`mtc1 a2, $f16`) and jumped into the shared tail at `0x80013B90`.
- **Native Implementation**:
  - Implemented as a strong native symbol `extern "C" void func_80013B70(...)` in `src/game/boot_hw_stubs.cpp:1310`.
  - Replaces the empty generated stub, reproducing the shared tail with `$f16 = a2`.
- **Verification**: Host test `tests/stick_angle_thunk_test.cpp`.

### 3.2 Stateful Input Helper Contract (`0x800979D8`)
- **Guest Address**: `0x800979D8`.
- **Contract & Wrappers**:
  - Handled by `tooie_bainput_func_800979D8` (registered via `add_loaded_function` in `boot_hw_stubs.cpp`), which delegates button held evaluation to `bakey_held`.
  - Input buffer `0x800BCE84` is monitored via wrapper `tooie_wrap_func_800BCE84`.
- **Verification**: Host test `tests/input_direction_test.cpp`.

### 3.3 Recomp API Input Hooks & Controller Defaults
- **SDL2 Polling**:
  - `recompinput::poll_inputs()` processes host gamepad and keyboard events.
  - Default controller bindings configured in `set_control_defaults` (`src/game/config.cpp`):
    - Gamepad shoulder buttons map to C-up / C-down.
    - Face NORTH button maps to C-left.
- **Registered Guest Input Exports (`src/main/main.cpp`)**:
  - `recomp_get_inverted_axes`: Registered via `REGISTER_FUNC(recomp_get_inverted_axes)`. Reads `get_camera_invert_mode()`.
  - `recomp_get_analog_inverted_axes`, `recomp_get_analog_cam_enabled`, `recomp_get_right_analog_inputs`, `recomp_get_bgm_volume`.
  - *Dormant Hooks*: `recomp_get_gyro_deltas` and `recomp_get_mouse_deltas` are defined in `recomp_api.cpp` but registration remains commented out in `main.cpp`. Guest-side camera consumer hooks remain unimplemented in the Tooie engine, so camera options remain disabled in the user interface.

### 3.4 Local Multiplayer

- **N64 Controller Port Policy (`include/tooie_controller_ports.h`)**:
  - Implemented as pure logic header validated via `tests/tooie_controller_ports_test.cpp`.
  - *Single-Player Mode*: All 4 N64 ports report connected and receive inputs merged from active controller profiles.
  - *Multiplayer Mode*: Port i corresponds directly to player slot i (0–3).
    - Port 0 is unconditionally connected; when slot 0 has no assigned device, it falls back to serving merged
      single-player input via `profiles::get_merged_n64_input`.
    - Ports 1–3 report connected only when an active device is assigned to that slot; otherwise `get_input` returns
      `false` (no response).
- **Player Slots & Assignment Model (`recompinput/include/recompinput/player_slots.h`)**:
  - 4 fixed player slots managed in RecompFrontend (`tests/recompinput_player_slots_test.cpp`).
  - Each slot can be assigned at most one controller AND/OR the keyboard. Gaps between assigned slots are permitted.
  - A controller belongs to at most one slot, and the keyboard belongs to at most one slot. Selecting an already
    assigned controller swaps assignments between the two slots.
  - Disconnecting a controller closes and clears the handle to prevent dangling references while retaining device
    identity for automatic reconnect.
  - Keyboard input is strictly isolated to its assigned slot and does not leak to other players.
- **Assignment Persistence & Device Correlation (`controls.json`)**:
  - Player slot configurations persist under the `players` key in `controls.json`, storing controller identity
    (serial number or model GUID hash) and keyboard attachment state.
  - On connection, controllers match against saved slots first by hardware serial number, then by model GUID hash in
    order of connection.
  - *Hardware Limitation*: Identical controllers lacking unique serial numbers (such as multiple Bluetooth Xbox One S
    controllers on Windows) cannot be distinguished across host restarts and are reassigned based on connection order.
- **Per-Player Mapping Profiles**:
  - Profiles `mp_player_<N>_controller` and `mp_player_<N>_keyboard` (N = 1–4) are allocated on demand with
    device defaults.
  - Player 1 profiles are copied from single-player default configurations on initial creation.
- **Rumble Testing & Driver Verification**:
  - `test_rumble(controller, 1000)` invokes `SDL_GameControllerRumble` at full strength for 1000 ms while temporarily
    suspending guest game rumble on that device.
  - On Windows with Bluetooth controllers, SDL's RAWINPUT driver requires at least one initial button input from the
    gamepad before accepting rumble ("Controller isn't correlated yet, try hitting a button first"); the UI reports
    "Press a button on it first" until correlated. Additional outcomes include "Rumbling…" (1 s), "No rumble", and
    "Rumble failed".
  - Tracks per-device input activity timestamps (~250 ms pulse).
- **Guest Read Contract & Absence Semantics (`librecomp`)**:
  - Patched in `vendor/patches-local/n64modernruntime-cont-port-fidelity.patch` (libultra semantics):
    - `osContGetReadData_recomp` writes `err_no` for every port that ultramodern read and copies button/stick only when
      `err_no == 0`, so absent or disconnected ports report non-zero `err_no` instead of keeping stale buttons.
    - Ports beyond the channel count set by `osContSetCh` (the game calls `osContSetCh(4)` then `osContSetCh(2)` when it
      enters its multiplayer mode) are left untouched in guest memory; before, they received uninitialized stack data and
      the game saw phantom players.
    - `__osContGetInitData` ORs one bit per connected port into the `osContInit` bitpattern (`pattern=03` for ports 0+1;
      single-player mode reports all 4 ports, `0F`).
  - Measured (`TOOIE_DIAG_CONT_PORTS`): `D_80079D6C[0..3]` follows per-frame port presence (`err_no == 0`).

---

## 4. Attract Mode Parity & Verification Scope

Banjo-Tooie includes automated attract demo sequences with controller inputs stored in ROM:
- Attract sequences (Demos 0, 1, 4, 5, 7) are utilized as regression detection benchmarks against the software-rendered reference oracle (`tools/oracle/angrylion-control/`).
- In attract mode (`0x80127632 == 5`), widescreen native planes are automatically deactivated in favor of isolated per-test expansions to preserve recorded demo alignment.
- Global gameplay and attract parity across the entirety of the campaign is not claimed as complete.