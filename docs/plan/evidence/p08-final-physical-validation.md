---
title: "P08 — Final Physical Hardware Validation Checklist"
description: "User manual validation protocol for physical gamepad controller and physical multi-monitor display / window integration on Linux hardware."
status: evidence
type: validation
project: TooieRecomp
phase: P08
date: 2026-08-20
tags: [p08, physical-validation, manual-validation, gamepad, controller, display, multi-monitor, fullscreen, checklist]
---

# P08 — Final Physical Hardware Validation Checklist

## 1. Scope & Execution Rules

This checklist validates physical hardware interaction with the production executable (`build/TooieRecompiled`).
All automated unit tests, static call validations, RT64 render-submission contracts, and N64 hardware-oracle motion parity tests are **COMPLETE**.

The final gates for formal **P08 COMPLETION** are:
1. **Physical Gamepad / Controller Validation**
2. **Physical Display / Multi-Monitor Validation**

> [!IMPORTANT]
> **Instructions for the User / Tester:**
> 1. Launch `./build/TooieRecompiled` on your physical desktop environment (X11 / Wayland).
> 2. Fill in the **Environment Information** table below for your setup.
> 3. Execute each action in the test tables.
> 4. Fill in the **Result** column (`PASS`, `FAIL`, or `N/A` where permitted).
> 5. Record the final status at the bottom of this document.

---

## 2. Environment Information

| Field | User Value |
|:---|:---|
| **Date** | *(e.g. 2026-08-20)* |
| **Machine / Host** | *(e.g. Desktop x86_64)* |
| **OS** | *(e.g. Ubuntu 24.04 / Arch / Fedora)* |
| **Desktop / Session** | *(e.g. GNOME Wayland / KDE X11 / XFCE)* |
| **GPU** | *(e.g. AMD Radeon RX 580 / NVIDIA RTX 3070 / Intel Iris Xe)* |
| **Driver** | *(e.g. Mesa 25.2.8 radv/radeonsi / NVIDIA 550.xx)* |
| **Controller Model** | *(e.g. Xbox One / DualShock 4 / 8BitDo Ultimate / Switch Pro)* |
| **Monitor Setup** | *(e.g. 1x 1080p 60Hz / 2x 1440p 144Hz / 4K + 1080p)* |

---

## 3. Physical Gamepad / Controller Checklist

| Test ID | Category | Action | Expected Result | Result (PASS/FAIL/N/A) | Notes |
|:---|:---:|:---|:---|:---:|:---|
| **PAD-01** | `MANDATORY` | Connect controller before launch and run `./build/TooieRecompiled` | Controller is detected and operational immediately in game menus and gameplay | `PENDING` | |
| **PAD-02** | `MANDATORY` | Title screen navigation | Press `START` on gamepad -> advances from Intro/Title to File Selection | `PENDING` | |
| **PAD-03** | `MANDATORY` | File-select menu navigation | Analog Stick Left / Right or D-Pad moves between file slots cleanly | `PENDING` | |
| **PAD-04** | `MANDATORY` | Confirm file selection | Press `A` button on gamepad -> selects file slot and starts intro/gameplay transition | `PENDING` | |
| **PAD-05** | `MANDATORY` | Cancel / Back buttons | Press `B` button in menus where applicable -> cancels / returns | `PENDING` | |
| **PAD-06** | `MANDATORY` | Analog Stick: Cardinal Directions | Tilt Stick UP, DOWN, LEFT, RIGHT -> player moves in exact requested direction | `PENDING` | |
| **PAD-07** | `MANDATORY` | Analog Stick: Diagonals | Tilt Stick NE, NW, SE, SW -> all four diagonal directions produce expected diagonal movement without cardinal bias, dead directions, inversion, or right-only behavior | `PENDING` | |
| **PAD-08** | `MANDATORY` | Analog Stick: Low Magnitude | Tilt Stick slightly (~20-40%) -> player walks slowly (proper analog scaling) | `PENDING` | |
| **PAD-09** | `MANDATORY` | Analog Stick: Full Deflection | Tilt Stick fully (100%) -> player runs at full speed without artificial clamping or asymmetry (informational: `tspd ≈ 700` in running state) | `PENDING` | |
| **PAD-10** | `MANDATORY` | Circular Stick Movement | Rotate Analog Stick in smooth 360° circles -> continuous, fluid facing rotation without snapping | `PENDING` | |
| **PAD-11** | `MANDATORY` | Directional Symmetry (No Right-Only Bias) | Player turns left, up, and down as freely and symmetrically as right (verifies `func_80013B70` fix) | `PENDING` | |
| **PAD-12** | `MANDATORY` | Axis Orientation | UP moves forward/up, DOWN moves backward/down, LEFT moves left, RIGHT moves right (no inverted axes) | `PENDING` | |
| **PAD-13** | `MANDATORY` | Stick Neutral / Release | Releasing stick returns host/N64 input to neutral (0,0); player does not continue receiving directional input or drift indefinitely (normal ORIGINAL_GAME deceleration is expected) | `PENDING` | |
| **PAD-14** | `MANDATORY` | Keyboard / Pad Coexistence | While controller is plugged in, keyboard keys (WASD / Space / Enter) also operate without conflict | `PENDING` | |
| **PAD-15** | `MANDATORY` | Focus Loss / Regain Input Safety | Alt-Tab away while holding stick, then return -> game releases held input; no stuck directional inputs | `PENDING` | |
| **PAD-16** | `OPTIONAL` | Hotplug Detection | Disconnect and reconnect gamepad during runtime -> game resumes controller input | `OPTIONAL` | RecompFrontend / SDL default |

---

## 4. Physical Display / Multi-Monitor Checklist

| Test ID | Category | Action | Expected Result | Result (PASS/FAIL/N/A) | Notes |
|:---|:---:|:---|:---|:---:|:---|
| **DISP-01** | `MANDATORY` | Initial window launch | Window appears centered and correctly sized with title `"Banjo-Tooie: Recompiled"` | `PENDING` | |
| **DISP-02** | `MANDATORY` | Primary display rendering | Game presents a valid rendered image without black/stale surface, crash, or broken swapchain (compositor VSync/tearing behavior is informational) | `PENDING` | |
| **DISP-03** | `MANDATORY` | Window dragging | Drag window across desktop -> rendering continues smoothly without crash or deadlock | `PENDING` | |
| **DISP-04** | `MANDATORY` | Multi-monitor move | Drag window from Primary to Secondary monitor -> swapchain adapts cleanly (`N/A` if single monitor) | `PENDING` | `N/A` allowed if single monitor |
| **DISP-05** | `MANDATORY` | High-DPI / Display scaling | Window renders with crisp high-DPI scaling on fractional/scaled displays (`N/A` if standard 100% DPI only) | `PENDING` | `N/A` allowed if standard DPI |
| **DISP-06** | `MANDATORY` | Window resizing | Resize window borders -> viewport scales cleanly while preserving aspect ratio contract | `PENDING` | |
| **DISP-07A** | `MANDATORY` | 4:3 Baseline Aspect Ratio | Game renders in correct 4:3 proportion with proper pillarboxing; no distorted geometry | `PENDING` | Baseline fidelity contract |
| **DISP-07B** | `OPTIONAL` | 16:9 Widescreen Expansion | If 16:9 widescreen is enabled in config, horizontal FOV expands cleanly without breaking UI | `OPTIONAL` | Optional enhancement |
| **DISP-08** | `MANDATORY` | Fullscreen toggle: Enter | Press `Alt+Enter` (or configure Fullscreen) -> transitions cleanly to fullscreen | `PENDING` | |
| **DISP-09** | `MANDATORY` | Fullscreen toggle: Exit | Press `Alt+Enter` again -> cleanly returns to windowed mode without corruption | `PENDING` | |
| **DISP-10** | `MANDATORY` | Fullscreen target monitor | In multi-monitor setup, fullscreen activates on the monitor currently holding the window (`N/A` if single monitor) | `PENDING` | `N/A` allowed if single monitor |
| **DISP-11** | `MANDATORY` | Focus loss (Alt-Tab) | Alt-Tab out of game -> desktop remains responsive, game does not crash or hang | `PENDING` | |
| **DISP-12** | `MANDATORY` | Focus regain | Switch back to game window -> graphics and audio resume immediately | `PENDING` | |
| **DISP-13** | `MANDATORY` | Minimize and restore | Minimize window to taskbar, then restore -> rendering resumes normally | `PENDING` | |
| **DISP-14** | `MANDATORY` | Clean exit | Close window (`Alt+F4` or window close button) -> clean process termination (code 0) without hangs | `PENDING` | |

---

## 5. Exit Decision Contract & Sign-Off

```text
MANDATORY_GAMEPAD_TESTS_PASS:   PENDING (requires 15/15 MANDATORY tests PASS)
MANDATORY_DISPLAY_TESTS_PASS:   PENDING (requires 14/14 MANDATORY tests PASS or N/A)
PHYSICAL_GAMEPAD_RESULT:        PENDING
PHYSICAL_DISPLAY_RESULT:        PENDING
P08_MANUAL_VALIDATION_RESULT:   PENDING
```

### Sign-Off Policy:
- **`PHYSICAL_GAMEPAD_RESULT = PASS`** requires all 15 `MANDATORY` gamepad tests to be marked `PASS`. (`PAD-16` is `OPTIONAL` and does not block pass).
- **`PHYSICAL_DISPLAY_RESULT = PASS`** requires all 14 `MANDATORY` display tests to be marked `PASS` (or `N/A` where hardware is unavailable: `DISP-04`, `DISP-05`, `DISP-10`). (`DISP-07B` is `OPTIONAL` and does not block pass).
- When both **`PHYSICAL_GAMEPAD_RESULT = PASS`** and **`PHYSICAL_DISPLAY_RESULT = PASS`**, a subsequent agent session will mark **`P08_MANUAL_VALIDATION_RESULT = PASS`**, mark **`P08 = COMPLETE`**, and formally open **`P09`**.
- If any mandatory item fails, **`P08 remains ACTIVE`** and only the specific physical defect will be addressed.
