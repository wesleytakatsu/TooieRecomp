---
title: "TooieRecomp Spec — Input and Controller Subsystem"
description: "Comprehensive technical specification for SDL2 controller mapping, stick-angle thunk override (80013B70), input helper contracts (800979D8), and attract playback."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, input, controller, sdl2, stick-angle, demo]
---

# Input and Controller Subsystem Specification

## 1. Subsystem Architecture

The input subsystem bridges modern host input devices (USB/Bluetooth controllers, keyboard/mouse) into Nintendo 64 controller structures (`OSContPad`) while providing native fixes for MIPS calling-convention defects that break analog stick normalization.

```text
Host Input Devices (SDL2 GameController / Keyboard)
                        |
                        v
RecompFrontend Input Mapper (recompinput::poll_inputs)
                        |
                        v
Native Stick-Angle Thunk (func_80013B70 Override)
                        |
                        v
Guest Controller Pad Array & Input Buffers (0x800BCE84)
                        |
                        v
Stateful Input Helpers (func_0x800979D8 / bakey_held)
                        |
                        v
Banjo-Tooie Player Physics & Attract Demo Engine
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-INP-01** | Modern Controller Mapping | SDL2 GameController maps standard modern gamepads to N64 layout. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-INP-02** | Stick Angle Normalization | `func_80013B70` converts analog stick coordinates with delay-slot $r1$ fix. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-INP-03** | Bidirectional Input | WASD keyboard and modern analog sticks navigate in all 360 degrees. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-INP-04** | Input Helper Contract | `0x800979D8` evaluates held state and ring-1 buffer `0x800BCE84`. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-INP-05** | Attract Mode Parity | Demos reproduce recorded input streams from ROM without desync. | `PARTIAL` / `OPEN_RESIDUALS` |
| **REQ-INP-06** | Camera Inversion Modes | Supports independent axis inversion for standard, 3rd, and 1st-person. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Interface & Calling Contracts

### 3.1 Analog Stick Angle Normalization (`func_80013B70`)
- **Guest Address**: `0x80013B70` (`baStick_getNormalizedAngle`).
- **Defect in Static Recompilation**:
  - Original MIPS assembly relied on a side-effect in delay-slot register `$r1` (`subu $r1, $zero, $a1`).
  - In recompiled C code, this register write was omitted or overwritten, corrupting stick angle calculations when moving left or downward, causing banjo to lock into right-facing movement.
- **Native Implementation Contract**:
  - Implemented as a strong native override in `src/game/recomp_api.cpp` / `RecompiledPatches`.
  - Normalizes input vector $(x, y)$, computes Euclidean magnitude $\sqrt{x^2 + y^2}$, applies deadzone filtering, and calculates trigonometric facing angle in radians/degrees.
- **Verification**:
  - Tested across all 4 cardinal directions, 4 diagonal quadrants, and boundary magnitudes (`tests/stick_angle_thunk_test.cpp`).
  - Independent peer review (**VERDICT A**, `p08-stick-angle-thunk-fix-rereview.md`).

### 3.2 Stateful Input Helper Contract (`0x800979D8`)
- **Guest Address**: `0x800979D8`.
- **Contract Rules**:
  1. Inspects the previous button state array: if `prev_state[button] == 0`, samples the live `0x800BCE84` ring-1 input buffer word.
  2. Delegates sustained button evaluation to `bakey_held`.
  3. Applies logical-input enable bitmask filter.
  4. Returns the validated active logical input bitmask.
- **Verification**:
  - Paired comparison with Angrylion oracle verified at USER_DEMO_4 glidx 39: pad word `0001`, ring-1 word `0x115`, `bakey_held=1`, return value `2` match reference identically (`p08-user-demo4-800979d8-independent-review.md`).

### 3.3 Recomp API Input Hooks (`src/game/recomp_api.cpp`)
- `recomp_update_inputs(uint8_t* rdram, recomp_context* ctx)`: Calls `tooie_diag::note_guest_input_update()` and `recompinput::poll_inputs()`.
- `recomp_get_gyro_deltas(uint8_t* rdram, recomp_context* ctx)`: Exposes host motion controls for first-person aiming.
- `recomp_get_mouse_deltas(uint8_t* rdram, recomp_context* ctx)`: Exposes raw mouse delta input.
- `recomp_get_inverted_axes(...)`: Returns inversion flags based on user configuration (`banjo::CameraInvertMode`).

---

## 4. Attract Demo Status and Open Residuals

Banjo-Tooie contains automated demo sequences stored as compressed controller inputs in ROM:
- **Demo 0 (Jinjo Village)**:
  - 765/765 input frames match reference.
  - Blue egg firing at glidx 322 matches Angrylion depth buffer.
  - Status: `PROVISIONAL_PRE_P09_PARITY_EVIDENCE`.
- **USER_DEMO_4 (Wooded Hollow / Pivot)**:
  - Idle follow-cam matches Angrylion glidx 1..38.
  - Walking-player `ncbapivot` jump table `80112C3C` verified in content.
  - Status: `BOUNDED_MATCH_ACCEPTED`.
- **USER_DEMO_7 (TDL Stomping Plains / Stationary Cinema)**:
  - Attract recorded input stream reproduces faithfully with zero drift: 119 pose pairs match N64 reference within quantized tolerances; 120/120 pad inputs match.
  - Glidx 38..120 is fully stationary at `(-80.46, 0.00, 4496.25)`, `velxz = 0.0`, `tspd = 0.0` following State 116 stop helper execution.
  - Status: `BOUNDED_MATCH_ACCEPTED` (Criterion 1: Demo 7 bounded stationary parity PASS).
- **Open Attract Residuals**:
  - `USER_DEMO_1`: Locomotion divergence observed by user (`p08-attract-movement-residuals.md`).
  - `USER_DEMO_5`: First locomotion divergence point remains under investigation.
  - Status: `USER_REPORTED / OPEN`. Global attract parity is **NOT CLAIMED**.
