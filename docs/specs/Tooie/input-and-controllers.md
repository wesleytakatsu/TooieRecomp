---
title: "TooieRecomp Spec — Input and Controller Subsystem"
description: "Technical specification for SDL2 controller mapping, stick-angle thunk override (80013B70), input helper contracts (800979D8), and attract playback."
status: current
area: "specs/Tooie"
updated: 2026-11-02
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
| **REQ-INP-06** | Camera Controls | Proportional third-person right-stick camera, logical Z+C priority and per-player ownership. | `IMPLEMENTED`; Windows user-validated. First-person/flying inversion remains unavailable; Linux pending. |
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
  - *Dormant Hooks*: `recomp_get_gyro_deltas` and `recomp_get_mouse_deltas` are defined in `recomp_api.cpp` but registration remains commented out in `main.cpp`. The legacy guest analog exports are not the consumer used by Tooie's host integration below.

### 3.3.1 Analog Camera

General -> **Analog Camera** is Off by default. **Analog Camera Sensitivity** (1–10, default 3) and
**Invert Camera** (None/X/Y/Both, existing default X) configure its two axes. Applied/load values are
published independently of UI objects; temporary values in a confirmation-based config do not affect gameplay.
The ordinary General tab retains its existing immediate-save behavior.

- Normal eligible story cameras consume continuous right-stick X/Y values. Radial deadzone preserves the
  direction/proportion of diagonals, with normalized maximum magnitude. Rotation speed scales with deflection
  and sensitivity; a short exponential ramp is integrated using native simulation time, independent of present FPS.
- Neutral axes stop manual rotation immediately. R retains native recenter priority.
- Logical N64 **Z**, including remapped gamepad or keyboard bindings, restores the original right-stick C bindings
  at controller read time. No hardcoded LT check is used. Native Z is checked again before moving the camera.
- First-person aiming releases the right stick's original C bindings for that player, including C-Left/C-Right
  zoom. The native `BA_FLAG_17_FIRST_PERSON_VIEW` is checked independently of the camera controller. Leaving aim
  resumes analog orbit only from a fresh reserved sample; revoked eligibility clears the previous sample and
  that player's smoothing, even when aiming does not execute an orbit update. Zoom retains native bindings and
  behavior rather than analog-orbit sensitivity or inversion. The other split-screen player remains independent.
- Reservation filters only right-axis bindings belonging to C-Up/Down/Left/Right. Physical C buttons, keyboard C
  bindings, other actions, the left stick, and user mapping profiles remain unchanged.
- Story mode, live owner, native player input availability, ba-camera class and supported manual-camera callbacks
  are required. The supported callbacks are string/pan/pivot. Locked/scripted cameras, first person, Breegull
  Blaster, remote control, attract, native multiplayer and dedicated minigame/race rooms keep original controls.
  Flight/swim and other unsupported camera controllers also retain native inputs.
- Eligibility is published by each player's update and rechecked at the camera update. The host controller read
  uses the latest publication; map reset clears publications, samples and smoothing. A camera never moves from a
  sample that was routed as ordinary C input. Closing/opening ESC or a character selector blocks that owner's sample.
- Each camera resolves its owner from its native node, then the owner's native controller index. Samples are indexed
  by controller; smoothing and update scopes are indexed by player. Single-player merged input and port-0 fallback
  use the same device source as N64 input. Global player focus is unchanged. The unused global right-stick
  suppression mechanism and its unregistered guest setter were removed; reservation is per read/player only.
- Native yaw/pitch restrictions and subsequent collision resolution remain in the path. Manual pitch additionally
  stays within -75..75 degrees. Projection, rendered matrices and interpolation are unchanged.

Implementation: `tooie_analog_camera*.cpp`, the scoped filters in `boot_hw_stubs.cpp`, per-player publication in
`tooie_dev_warp_hook.cpp`, and `recompfrontend-per-player-camera-input.patch`. No runtime logger or counters added.
Permanent host regressions cover the policy/config, native string controller and native orbit filters with synthetic
memory/math/collision services, plus actual SDL virtual controllers and mapping profiles. This does not establish
exhaustive map/transformation/minigame coverage. The user confirmed the feature working on Windows (2026-11-01),
limited to the cases tested; the first-person zoom adjustment was also confirmed on Windows (2026-11-02).
Linux: `NOT_RUN`.
Native contracts: [camera controls](../../reverse-engineering/systems/camera-controls.md).

### 3.4 Local Multiplayer

The Dev Mode transformation selector reads Debug Button and game buttons from each issuing player's profile in
`tooie_get_n64_input`. Physical R3 or Debug Button + D-pad down opens that player's overlay; the direct R3 fallback
also supports old profiles without a Debug Button binding. Navigation/confirmation are consumed
only for that player until the closing buttons/stick are released. The other port continues supplying game input.
The selector does not create a globally capturing UI context. Details: [window/UI contract](window-ui-and-config.md).
Fly/Blaster shortcuts also accept physical R3 or the mapped Debug Button for J1/J2, with separate button-edge history
and owner-indexed requests. Their shared enable/direction settings apply to both players. Physical D-pad directions
are read only for host shortcuts; saved bindings and N64 game input remain unchanged. An open/closing character
selector suppresses cheats for that owner until its input is released.

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

### Story co-op challenge boundaries

Host form/cheat shortcuts use the published native owner-to-controller binding rather than assuming slot equals
port. A port token changes at player recreation, retirement or binding change; held Debug/R3, D-pad, A/B or menu
navigation must return to neutral before a fresh shortcut is admitted. Creation/retirement cancellation and input
admission share a mutex. The input callback reads no mutable guest pointers. A suspended/auxiliary J2 receives no
host shortcut publication. Native multiplayer keeps its port assignments and original controller values.
Unavailable host menus discard pending opens and confirmation drain instead of consuming native input.
Analog Camera resets samples, reservations and smoothing at every recreation, including same-room retries;
Mini-Sub/Saucer segment rooms retain native C controls. Native auxiliary cameras cannot inherit story J2 ownership.
Co-op suspension changes player creation, not physical connectivity/polling or controller profiles. See
[[window-ui-and-config]] and [[reverse-engineering/systems/minigames]].

## 4. Attract Mode Parity & Verification Scope

Banjo-Tooie includes automated attract demo sequences with controller inputs stored in ROM:
- Attract sequences (Demos 0, 1, 4, 5, 7) are utilized as regression detection benchmarks against the software-rendered reference oracle (`tools/oracle/angrylion-control/`).
- In attract mode (`0x80127632 == 5`), widescreen native planes are automatically deactivated in favor of isolated per-test expansions to preserve recorded demo alignment.
- Global gameplay and attract parity across the entirety of the campaign is not claimed as complete.
