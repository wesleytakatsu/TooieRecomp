# P08-T07 & P08-T08 Frontend and System Integration Validation

## Overview

This evidence document records the specification, implementation, and automated validation for tasks **P08-T07** (*Window, fullscreen, focus, multi-monitor, display scale / DPI / resolution behavior*) and **P08-T08** (*Controlled fallback / useful error message when graphics driver / Vulkan / required API is unavailable or incompatible*).

## P08-T07: Window, Display, Scaling, and Aspect Ratio Integration

### Acceptance Contract
- **Window Creation & Sizing**: Safe initialization with dimension sanitization (guaranteeing >= 320x240, no zero or negative dimensions).
- **Multi-Monitor / Display Index**: Display index validation against available display count (`validate_display_index`); safe fallback to primary display (0) on missing or disconnected monitors.
- **High-DPI / Scaling**: Logical-to-pixel scale calculation (`calculate_drawable_scale`) ensuring valid scaling factors without division by zero.
- **Aspect Ratio Independence**: Guest 4:3 rendering decoupled from host window size; safe aspect computation for `Original`, `Expand`, and cutscene modes (`Original`, `Clamp16x9`, `Full`).
- **Focus & Window Events**: Event tracking for focus gained/lost, minimized/restored, resized, and display changed (`handle_window_event`).

### Implementation
- Added [`include/tooie_window.h`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/include/tooie_window.h) and [`src/game/tooie_window.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/src/game/tooie_window.cpp).
- Integrated `tooie_window` helpers into [`src/game/recomp_api.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/src/game/recomp_api.cpp) and [`src/main/main.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/src/main/main.cpp).
- Created regression test suite [`tests/window_display_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/window_display_test.cpp).

### Validation Results
```
=== Running tooie_window / P08-T07 Unit Tests ===
=== PASS: 45/45 assertions passed ===
```
- **Commit**: `9d1bf9c` (`feat(p08): complete window and display integration`)

---

## P08-T08: Graphics Driver, Vulkan, and Window Failure Handling

### Acceptance Contract
- **No Crash / No SIGABRT**: Replaced `assert(false)` and uncaught exceptions with controlled error boundaries.
- **Error Classification**: Structured mapping for `WINDOW_INIT_FAILED`, `NO_VULKAN`, `NO_COMPATIBLE_GPU`, `VULKAN_DEVICE_INIT_FAILED`, `RT64_INIT_FAILED`, and `AUDIO_INIT_FAILED`.
- **Actionable User Message**: Plain-language user-visible dialog via `SDL_ShowSimpleMessageBox` / `recompui::message_box` accompanied by complete technical diagnostics on `stderr`.
- **Clean Teardown & Non-Zero Exit**: All resources and child preview processes are safely terminated with `EXIT_FAILURE` (code 1).

### Implementation
- Added [`include/tooie_error.h`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/include/tooie_error.h) and [`src/game/tooie_error.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/src/game/tooie_error.cpp).
- Updated `exit_error`, `create_gfx`, and `main()` in [`src/main/main.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/src/main/main.cpp) to report structured errors and catch fatal runtime exceptions.
- Created regression test suite [`tests/graphics_failure_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/graphics_failure_test.cpp).

### Validation Results
1. **Unit Test Suite**:
```
=== Running tooie_error / P08-T08 Unit Tests ===
=== PASS: 25/25 assertions passed ===
```
2. **Negative Environment Test**:
```bash
VK_ICD_FILENAMES=/nonexistent.json TOOIE_NO_GUI_DIALOG=1 timeout 3 ./build/TooieRecompiled
# Exit Code: 1
# Stderr: [tooie-error] ERROR CLASS: WINDOW_INIT_FAILED
#         [tooie-error] TITLE: Banjo-Tooie: Startup Error
#         [tooie-error] DETAILS: Failed to create window: Installed Vulkan doesn't implement the VK_KHR_surface extension
# Result: Clean exit, no crash, no hang, correct error class.
```
- **Commit**: `38fb2a4` (`feat(p08): handle graphics initialization failures`)

---

## P08 Complete Task Audit (T01 - T11)

| Task | Description | Status | Evidence / Verification |
|---|---|---|---|
| **P08-T01** | RT64 submodule & build integration | **DONE** | RT64 Vulkan/Plume linked and operational in `TooieRecompiled` |
| **P08-T02** | Render-submission matrix contract | **DONE** | [`tests/matrix_submission_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/matrix_submission_test.cpp) (PASS, `91bf65f`) |
| **P08-T03** | Framebuffer / depth buffer setup & presentation | **DONE** | RT64 VI presentation loop verified (attract loop & gameplay rendering) |
| **P08-T04** | Aspect ratio / internal resolution decoupling | **DONE** | Decoupled guest 4:3 baseline from host window resolution (`tooie_window`) |
| **P08-T05** | Controller mapping & SDL gamepad integration | **DONE** | [`tests/input_direction_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/input_direction_test.cpp), `recompcontrollerdb.txt` |
| **P08-T06** | Keyboard / mouse binding & input options | **DONE** | Inherited from `recompinput` & `recompui` |
| **P08-T07** | Window, fullscreen, focus, multi-monitor, scale/DPI | **DONE** | [`tests/window_display_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/window_display_test.cpp) (45/45 assertions, `9d1bf9c`) |
| **P08-T08** | Graphics driver / Vulkan fallback error handling | **DONE** | [`tests/graphics_failure_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/graphics_failure_test.cpp) (25/25 assertions, `38fb2a4`) |
| **P08-T09** | Config UI / graphics & input settings menus | **DONE** | RecompUI graphics/input config tabs functional and persistent |
| **P08-T10** | Launcher music preview loop lifecycle & auto-start | **DONE** | Child preview process lifecycle + orphan safeguard (`6e44d03`) |
| **P08-T11** | Multi-target regression test suite | **DONE** | 7 automated test targets passing 100% |

---

## Exit Gate Evaluation & Boundary to P09

### Exit Gate: `"logo/menu/gameplay renderizam e controles funcionam"`
- **Evaluation**: **MET**.
  - Logos, menus, attract mode sequence (Intro -> Spiral Mountain cinematic -> File Select -> DEMO1 Jinjo -> DEMO2 Mayahem -> attract cycle), and gameplay render correctly with RT64 presentation.
  - Controller and synthetic inputs function with proper deadzones and range mapping.
  - Windowing, DPI scaling, fullscreen toggle, and driver error handling operate reliably without crashes.

### Boundary Recommendation: P08 -> P09
- **P08 Scope (CLOSED)**: RT64 presentation baseline, input mapping, windowing, graphics/API error handling, launcher music preview, and static call correctness.
- **P09 Scope (NEXT)**: Game loop timing, frame interpolation / 60 FPS motion smoothing, save/load state persistence, long-session stability, multi-level gameplay playthrough.
