---
title: "P08 — Tooie render-submission matrix fix"
description: "Implementation of the proven incremental projection and view matrix submission split in TooieRecomp, avoiding RT64 viewproj decomposition and restoring correct camera/actor view extraction."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, render-submission, matrix-fix, transform-hooks, rt64, viewproj]
---

# P08 — TOOIE RENDER-SUBMISSION MATRIX FIX

```text
BASE_HEAD:                           32a9aa738aba3d6eb81248817b80c887d5914c6c
MODEL:                               Gemini 3.7 Flash HIGH
MODE:                                NARROW IMPLEMENTATION + VALIDATION
SCENE:                               map 335 / MAP_14F_IOH_WOODED_HOLLOW (mode 5)
ROOT_CAUSE_LEVEL:                    A (PROVEN & VALIDATED)
FIX_IMPLEMENTED:                     YES
FILES_ADDED:                         src/game/transform_hooks.cpp
                                     tests/matrix_submission_test.cpp
FILES_MODIFIED:                      CMakeLists.txt
UNIT_TEST_RESULT:                    PASS (MAX_MATRIX_ERROR: 0.000000e+00)
REGRESSION_TESTS:                    bcf28_helpers_test (PASS), core1_ra_wrappers_test (PASS)
GREEN_JINJO_UPRIGHT:                 YES
BANJO_UPRIGHT:                       YES
BANJO_PATH_STILL_PLAUSIBLE:          YES
CAMERA_ELEVATED_CORRECTLY:           YES
CAMERA_INSIDE_ACTOR_MESH_IN_SAME_SHOT: NO
FIRST_VISIBLE_ATTRACT_DEMO_RENDER_CORRECT: YES
```

---

## 1. Executive Summary

In Rare's Banjo-Tooie F3DEX2 display-list pipeline, the CPU constructs a combined View-Projection matrix (`VP = T_S * V_rot * P`) in `func_800E443C` / `func_800E44FC` and originally submitted it as a single `G_MTX_PROJECTION | G_MTX_LOAD` command.

When intercepted by RT64 (`RSP::matrixCommon`), any `G_MTX_PROJECTION | G_MTX_LOAD` command with non-zero translation is classified as a view-projection candidate (`isMatrixViewProj == true`) and subjected to `matrixDecomposeViewProj`. Because the decomposer assumes standard unscaled perspective (`P[2][3] = -1.0`), but Rareware games scale perspective by `0.5` (`P[2][3] = -0.5`), RT64 extracted an unnormalized view matrix with its Z basis and Z camera translation halved, causing the camera to render at turf level or inside character meshes and distorting actor orientation.

This fix implements a narrow, native override for `func_800E44FC`, `func_800E4640`, and `func_800E4628` in `src/game/transform_hooks.cpp`:
1. **Pure Projection Submission**: Submits `P` as `G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH`. Because `P[3][3] == 0.0`, RT64 classifies it as a pure projection (`isMatrixViewProj == false`) and sets `proj = P, view = Identity`.
2. **Incremental View Submission**: Submits `V_view = T_S * V_rot` as `G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH`. Because `V_view` is affine (`isMatrixAffine == true`), RT64 accumulates `view = V_view`.
3. **Preserves Guest Semantics**: Computes and stores the original combined `T_S * V_rot * P` fixed-point matrix at `D_8012D558` and preserves `D_8012D554` / `D_8012D508` behavior.

---

## 2. Re-Verification of Functions & Globals (PROVEN)

```text
P_MATRIX_SOURCE:
  func_800CB124(D_8012D500, &proj)
  Builds perspective projection matrix P with Rare scale 0.5.

VIEW_ROT_SOURCE:
  func_800CA7AC(D_8012D500) / D_8012D500 + 0xA4
  Extracts 4x4 camera rotation matrix V_rot.

VIEW_TRANSLATION_SOURCE:
  func_800CA7E4(D_8012D500, translation) and func_800CA6E8(D_8012D500) (world scale).
  Extracts translation (eye_x, eye_y, eye_z) and uniform scale.

COMBINED_CACHE_SEMANTICS:
  D_8012D558 stores a pointer to the combined fixed-point Mtx (T_S * V_rot * P).
  Written in func_800E44FC, cleared in func_800E4628, read in func_800E4640.

800E4640_SEMANTICS:
  If D_8012D508 != 0: emits gSPDisplayList(0x08000000).
  If D_8012D508 == 0: re-submits the cached projection representation.
  Does NOT change any guest simulation state.

D_8012D558_GUEST_SEMANTICS_PRESERVED:
  YES.
```

---

## 3. Mathematical Proof of Equivalence

Matrix multiplication is strictly associative:
$$\text{VP}_{\text{combined}} = T_S \times (V_{\text{rot}} \times P) = (T_S \times V_{\text{rot}}) \times P = V_{\text{view}} \times P$$

In `tests/matrix_submission_test.cpp`:
- `MAX_MATRIX_ERROR: 0.000000e+00`
- `MATRIX_EQUIVALENCE: PASS`
- `PURE_P_IS_VIEWPROJ: FALSE` (RT64 bad decomposition path avoided)
- `V_VIEW_IS_AFFINE: TRUE` (RT64 affine view path selected)
- `REVERSED_ORDER_ERROR: 978.4684` (proves test is strictly order-sensitive)

---

## 4. Implementation Details

- Created `src/game/transform_hooks.cpp` with strong C++ overrides for `func_800E44FC`, `func_800E4640`, `func_800E4628`.
- Properly handles 64-bit sign-extended MIPS register state and preserves callee-saved registers (`$s0..$s7`, `$ra`, `$sp`).
- Dynamic display list buffer allocation in `GfxContext` advances `cur_mtx` by 4 matrices (256 bytes) and `cur_gfx` by 4 GBI packets (32 bytes).
- Added `tests/matrix_submission_test.cpp` and updated `CMakeLists.txt`.

---

## 5. Runtime Validation

Deterministic run:
```sh
TOOIE_DIAG_MOVEMENT=1 TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json timeout -k 2s 75s ./build/TooieRecompiled
```

Results:
- Wooded Hollow mode 5 attract demo ran smoothly for all 75 seconds without crashing.
- Banjo walked continuously along the terrain path from `(1861, -131, -166)` to `(2507, -26, -1750)`.
- Green Jinjo actor (`act=801c3880`) remained upright at `rot=(0.00, 41.00, 0.00)`.
- Camera elevation tracking remained elevated at `Y = 411.56..463.69` pitched down ~27 degrees.
- Regression tests `bcf28_helpers_test` and `core1_ra_wrappers_test` both PASS.
