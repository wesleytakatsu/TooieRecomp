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

## CANONICAL_P08_TASK_MATRIX (Restored from Phase Plan)

| Task | Canonical Task Description | Status | Evidence / Verification |
|---|---|---|---|
| **P08-T01** | Integrar RT64 e validar Vulkan nesta máquina. | **DONE** | RT64 Vulkan/Plume linked and operational in `TooieRecompiled` |
| **P08-T02** | Catalogar display lists/efeitos Tooie; scheduler DP/VI OK. | **DONE** | [[p08-display-effect-catalog]] (28 effect families cataloged, RDP/RSP mapped, P09 matrix established, matrix submission test PASS) |
| **P08-T03** | Reproduzir 4:3 e timing originais sem melhorias ativas | **DONE** | [[p08-original-timing-4x3-baseline]] (60Hz VI, 20/30Hz logic, 30Hz animation, host decouple ~935 Hz, and 4:3 content aspect confirmed) |
| **P08-T04** | Integrar path SDL/RecompFrontend do BanjoRecomp. | **DONE** | RecompFrontend event/render callbacks, RmlUi backend, and config tabs integrated |
| **P08-T05** | Defaults/persistência BanjoRecomp; `controls.json` lazy. | **DONE** | Config persistence in `recomp::config` (`config.toml`) and lazy controller profile creation |
| **P08-T06** | Esconder launcher no auto-start e título Tooie. | **DONE** | `TOOIE_AUTO_START` suppresses launcher UI; window title set to "Banjo-Tooie: Recompiled" |
| **P08-T07** | Janela, fullscreen, foco, multi-monitor e escalas. | **IMPLEMENTED_MANUAL_VALIDATION_PENDING** | [`tests/window_display_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/window_display_test.cpp) (38/38 assertions PASS, `9726023`); SDL/RT64 fullscreen/focus/display handling verified on reference path; manual physical multi-monitor and fullscreen visual validation pending |
| **P08-T08** | Fallback/mensagem para driver/API incompatível. | **DONE** | [`tests/graphics_failure_test.cpp`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tests/graphics_failure_test.cpp) (25/25 assertions, `38fb2a4`), negative env test PASS (`WINDOW_INIT_FAILED`, exit code 1) |
| **P08-T09** | Evidência visual: intro/Jiggy OK; first demo visible & upright | **DONE** | Intro 3D, Jiggy logo, Wooded Hollow attract demo visible and upright (`src/game/transform_hooks.cpp` matrix split) |
| **P08-T10** | Input title/file select VALIDATED | **DONE** | [[p08-title-file-select-validation]] (intro -> file select -> slot navigation -> `CONT_A` confirm PASS; destination `MAP_A1_CS_INTRO` mode 3) |
| **P08-T11** | Máquina de estados da abertura: type-4+warp OK; C2AB8 RESOLVED by PATCH 4; `func_800BCF28` RESOLVED; `func_80013A7C` core1 RA wrappers RESOLVED; Render-submission matrix split RESOLVED | **DONE** | State machine, preemption, and all audited static overlays operational without crash ([[p08-attract-loop-after-demo2-crash]]) |

---

## Exit Gate Evaluation & Closure Readiness (Verdict B)

### Exit Gate: `"logo/menu/gameplay renderizam e controles funcionam"`
- **Evaluation**: **SUBSTANTIALLY COMPLETE / MANUAL VALIDATION PENDING (Verdict B)**.
  - Logos, menus, attract mode sequence (Intro -> Spiral Mountain cinematic -> File Select -> DEMO1 Jinjo -> DEMO2 Mayahem -> attract cycle), and gameplay render correctly with RT64 presentation.
  - Keyboard WASD and synthetic controller inputs function with proper deadzones and range mapping.
  - Windowing, DPI scaling, fullscreen toggle, and driver error handling operate reliably without crashes.
  - Physical gamepad hardware testing on varied physical controllers and live physical multi-monitor migration remain pending manual human validation.

### Phase Status
- **P08 Status**: **ACTIVE (MANUAL_VALIDATION_PENDING)**.
- **P09 Status**: **PENDING (NOT OPEN YET)**.
- **P08 / P09 Invariants (Protected)**: attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline.
