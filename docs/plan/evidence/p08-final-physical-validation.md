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

This checklist validates the physical hardware interaction with the production executable (`build/TooieRecompiled`).
All automated tests, static call verifications, RT64 render-submission contracts, and N64 hardware-oracle motion parity tests are **COMPLETE**.

The final gates for formal **P08 COMPLETION** are:
1. **Physical Gamepad / Controller Validation**
2. **Physical Display / Multi-Monitor Validation**

> [!IMPORTANT]
> **Instructions for the User / Tester:**
> 1. Launch `./build/TooieRecompiled` on your physical desktop environment (X11 / Wayland).
> 2. Execute each action in the tables below.
> 3. Fill in the **Result** column (`PASS` / `FAIL`) and recording notes if any deviation is observed.
> 4. Record the final status at the bottom of this document.

---

## 2. Environment Information

| Field | User Value |
|:---|:---|
| **Date** | 2026-08-20 |
| **Machine / Host** | Linux x86_64 |
| **OS / Desktop** | Ubuntu 24.04 LTS (X11 / Wayland) |
| **GPU / Driver** | AMD Radeon RX 580 Series (radeonsi / Mesa 25.2.8) |
| **Controller Model** | *(e.g. Xbox One / DualShock 4 / 8BitDo / Switch Pro)* |
| **Monitor Setup** | *(e.g. 1x 1080p / 2x 1440p / 4K Primary + 1080p Secondary)* |

---

## 3. Physical Gamepad / Controller Checklist

| Test ID | Action | Expected Result | Result (PASS/FAIL) | Notes |
|:---|:---|:---|:---:|:---|
| **PAD-01** | Connect controller before launch and run `./build/TooieRecompiled` | Controller is detected and operational immediately in game | `PENDING` | |
| **PAD-02** | Title screen navigation | Press `START` on gamepad -> advances from Intro/Title to File Select | `PENDING` | |
| **PAD-03** | File-select menu navigation | Analog Stick Left / Right or D-Pad moves between file slots cleanly | `PENDING` | |
| **PAD-04** | Confirm file selection | Press `A` button on gamepad -> selects file and starts intro/gameplay transition | `PENDING` | |
| **PAD-05** | Cancel / Back buttons | Press `B` button in menus where applicable -> cancels / returns | `PENDING` | |
| **PAD-06** | Analog Stick: Cardinal Directions | Tilt Stick UP, DOWN, LEFT, RIGHT -> player moves in exact requested direction | `PENDING` | |
| **PAD-07** | Analog Stick: Diagonals | Tilt Stick NE, NW, SE, SW -> player moves along precise 45° diagonal paths | `PENDING` | |
| **PAD-08** | Analog Stick: Low Magnitude | Tilt Stick slightly (~20-40%) -> player walks slowly (proper analog scaling) | `PENDING` | |
| **PAD-09** | Analog Stick: Full Tilt | Tilt Stick fully (100%) -> player runs at full top speed (`tspd = 700`) | `PENDING` | |
| **PAD-10** | Circular Stick Movement | Rotate Analog Stick in full 360° circles -> smooth continuous facing rotation | `PENDING` | |
| **PAD-11** | No Right-Only Bias | Check that player can turn left, up, and down as freely as right (no 13B70 regression) | `PENDING` | |
| **PAD-12** | No Inverted Axes | Verify UP moves forward/up, DOWN moves backward/down (no axis inverted) | `PENDING` | |
| **PAD-13** | Release / Neutral Stick | Release Analog Stick -> player stops immediately without lingering momentum or drift | `PENDING` | |
| **PAD-14** | Keyboard / Pad Coexistence | While controller is plugged in, WASD / Space / Enter keys also function without conflict | `PENDING` | |
| **PAD-15** | Focus Loss / Regain | Alt-Tab away from game while holding stick, then return -> no stuck inputs | `PENDING` | |
| **PAD-16** | Hotplug *(Optional / Informational)* | Disconnect and reconnect gamepad during runtime -> controller resumes control | `OPTIONAL` | BanjoRecomp SDL default |

---

## 4. Physical Display / Multi-Monitor Checklist

| Test ID | Action | Expected Result | Result (PASS/FAIL) | Notes |
|:---|:---|:---|:---:|:---|
| **DISP-01** | Initial window launch | Window appears centered and correctly sized with title `"Banjo-Tooie: Recompiled"` | `PENDING` | |
| **DISP-02** | Primary display rendering | Game renders smoothly on primary monitor without tearing or black screen | `PENDING` | |
| **DISP-03** | Window dragging | Drag window across desktop -> rendering continues smoothly without hitching | `PENDING` | |
| **DISP-04** | Multi-monitor move | Drag window from Primary to Secondary monitor -> swapchain adapts cleanly | `PENDING` | |
| **DISP-05** | High-DPI / Display scaling | On displays with 125%, 150%, or 200% scaling, window renders with crisp scaling | `PENDING` | |
| **DISP-06** | Window resizing | Resize window edges -> viewport scales while preserving aspect ratio contract | `PENDING` | |
| **DISP-07** | Aspect ratio modes | Check 4:3 pillarboxing / 16:9 widescreen expansion settings -> no distorted geometry | `PENDING` | |
| **DISP-08** | Fullscreen toggle: Enter | Press `Alt+Enter` (or configure Fullscreen) -> transitions to borderless/exclusive fullscreen | `PENDING` | |
| **DISP-09** | Fullscreen toggle: Exit | Press `Alt+Enter` again -> cleanly returns to windowed mode without corruption | `PENDING` | |
| **DISP-10** | Fullscreen target monitor | In multi-monitor setup, fullscreen activates on the monitor holding the window | `PENDING` | |
| **DISP-11** | Focus loss (Alt-Tab) | Alt-Tab out of fullscreen/windowed game -> desktop is responsive, game does not crash | `PENDING` | |
| **DISP-12** | Focus regain | Switch back to game window -> graphics resume instantly without black/stale surface | `PENDING` | |
| **DISP-13** | Minimize and restore | Minimize window to taskbar, then restore -> rendering resumes normally | `PENDING` | |
| **DISP-14** | Clean exit | Close window (`Alt+F4` or close button) -> clean shutdown without hanging process | `PENDING` | |

---

## 5. Exit Decision Contract & Sign-Off

```text
PHYSICAL_GAMEPAD_RESULT:        PENDING
PHYSICAL_DISPLAY_RESULT:        PENDING
P08_MANUAL_VALIDATION_RESULT:   PENDING
```

### Sign-Off Policy:
- When both **`PHYSICAL_GAMEPAD_RESULT = PASS`** and **`PHYSICAL_DISPLAY_RESULT = PASS`**, the user updates this document. A subsequent agent session will mark **`P08 = COMPLETE`** and formally open **`P09`**.
- If any mandatory item fails, **`P08 remains ACTIVE`** and only the failed physical defect will be addressed.
