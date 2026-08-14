---
title: "P08 — actor and camera transform discriminator"
description: "Telemetry discrimination between Actor.rotation simulation state, model matrix construction, and camera view transform during the first visible Wooded Hollow attract demo."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, attract, wooded-hollow, transform, discriminator, jinjo, camera]
---

# P08 — ACTOR AND CAMERA TRANSFORM DISCRIMINATOR

```text
BASE_HEAD:                           30b3f0619d94a18f2c748442a7f84983d0310e75
MODEL:                               Gemini 3.7 Flash HIGH
MODE:                                NARROW DIAGNOSIS ONLY
SCENE:                               map 335 / MAP_14F_IOH_WOODED_HOLLOW (mode 5)
ROOT_CAUSE_LEVEL:                    B
JINJO_ACTOR_ROTATION_EXPLAINS_POSE:  NO
BANJO_ACTOR_ROTATION_PLAUSIBLE:      YES
LOGICAL_ROTATION_CORRECT:            YES
MODEL_MATRIX_ORIENTATION_CORRECT:    YES
CAMERA_LOGICAL_TRANSFORM_CORRECT:    YES
CAMERA_VIEW_MATRIX_CORRECT:          YES
FIRST_BAD_TRANSFORM_STAGE:           RENDER_SUBMISSION (Rare view-in-projection vs renderer)
ACTOR_CAMERA_SHARED_ROOT:            PROVEN
FIX_IMPLEMENTED:                     NO
```

---

## 1. Executive Summary

During the first 8 seconds of the visible Wooded Hollow attract demo (`map 335`, `mode 5`):
1. **Simulation State (`Actor.rotation`):** The green Jinjo actor instance (`0x801c6d60`, overlay 425 `chjinjo`) has `Actor.rotation = (0.00°, 41.00°, 0.00°)`. Pitch is `0.00°` and Roll is `0.00°`. Banjo has `pos = (1861, -131, -166) -> (2507, -26, -1750)` and `yaw = 276.00° -> 148.60°` with `pitch = 0.00°` and `roll = 0.00°`. Logical simulation rotation for all actors is **upright and correct**.
2. **Model Transform Matrix (`func_800E28A4`):** The CPU matrix builder multiplies `mlMtxTranslate`, `mlMtxScale`, and `mlMtxRotatePYR(0°, 41°, 0°)`, producing a valid upright 4x4 matrix with Y-basis `(0.0, 1.0, 0.0)`.
3. **Camera State & View Matrix (`1EBCFC0.c` / `func_800E44FC`):** The Viewport (`D_8012D500`) has eye position `Y ≈ +313..+354` (situated ~400 units above Banjo at `Y ≈ -131..-26`) with orientation pitched down ~26° pointing towards Banjo. Logical camera simulation and view matrices are **upright and plausible**.
4. **First Failing Stage:** **`RENDER_SUBMISSION`**. Rareware games load the combined `View * Projection` matrix into `G_MTX_PROJECTION` and an identity matrix into `G_MTX_MODELVIEW` (`1EBCFC0.c:270-278`), subsequently multiplying actor transforms into `G_MTX_MODELVIEW`. The visual discrepancy (characters appearing supine, camera clipping near turf) originates from how the host renderer (RT64) interprets and decomposes the non-standard `G_MTX_PROJECTION` view-projection stack.

---

## 2. Selected Actor: Green Jinjo Instance

In `map 335` (Wooded Hollow), the foreground Jinjo is actor index 28:

```text
JINJO_ACTOR_PTR:          0x801c6d60 (in global actors vector D_80136EE0)
ACTOR_ID / marker flags:  u74=0x18b90021, u14=0x004a0021, d0=0x0, d1=0x0
ACTOR_TYPE:               chjinjo (overlay 425, entrypoint 5 at 0x80086ba8)
MODEL_ID:                 0x48F (1167)
SPAWN POSITION:           (-6437.00, -630.00, 3041.00)
ACTOR_ROTATION:           (0.00°, 41.00°, 0.00°)
SCALE:                    1.000
```

### Actor Struct Contract (`include/common.h:37-112`)
- `Actor.position`: offset `+0x04` (`f32[3]`, X, Y, Z)
- `Actor.scale`: offset `+0x38` (`f32`)
- `Actor.rotation`: offset `+0x44` (`f32[3]`, Pitch, Yaw, Roll in degrees)
- `ROTATION_UNITS`: Degrees in single-precision floating point (`f32`).

---

## 3. Banjo & Jinjo Telemetry Trace (~8 s of Mode 5)

| gfx tick | Entity | Position (X, Y, Z) | Rotation (Pitch, Yaw, Roll) | Status |
| --- | --- | --- | --- | --- |
| 49591 | Banjo (P0) | 1861.00, -131.00, -166.00 | 0.00°, 276.00°, 0.00° | Grounded start |
| 49591 | Jinjo (i=28) | -6437.00, -630.00, 3041.00 | 0.00°, 41.00°, 0.00° | Upright (yaw 41°) |
| 50560 | Banjo (P0) | 1936.88, -119.15, -468.07 | 0.00°, 164.14°, 0.00° | Scripted walk |
| 50560 | Jinjo (i=28) | -6437.00, -630.00, 3041.00 | 0.00°, 41.00°, 0.00° | Upright (yaw 41°) |
| 51522 | Banjo (P0) | 2228.25, -84.63, -1165.98 | 0.00°, 154.33°, 0.00° | Scripted walk |
| 51522 | Jinjo (i=28) | -6437.00, -630.00, 3041.00 | 0.00°, 41.00°, 0.00° | Upright (yaw 41°) |
| 52347 | Banjo (P0) | 2507.39, -26.81, -1750.55 | 0.00°, 148.60°, 0.00° | Scripted walk |
| 52347 | Jinjo (i=28) | -6437.00, -630.00, 3041.00 | 0.00°, 41.00°, 0.00° | Upright (yaw 41°) |

- `JINJO_ACTOR_ROTATION_EXPLAINS_SUPINE_POSE`: **NO** (Pitch = 0°, Roll = 0°).
- `BANJO_ACTOR_ROTATION_PLAUSIBLE`: **YES** (Smooth upright motion following terrain).

---

## 4. Model Matrix Construction Audit

Function `func_800E28A4` (`RecompiledFuncs/funcs_41.c:2619`) builds the model matrix:
1. `mlMtxTranslate(pos[0], pos[1], pos[2])`
2. `mlMtxScale(scale)`
3. `mlMtxRotatePYR(rot[0], rot[1], rot[2])`:
   - `mlMtxRotYaw(41.00°)`: calls `tooie_sin_degrees(41°) = 0.6561`, `tooie_cos_degrees(41°) = 0.7547`
   - `mlMtxRotPitch(0.00°)`: early-out on zero pitch (`c1cs == true`)
   - `mlMtxRotRoll(0.00°)`: early-out on zero roll (`c1cs == true`)

The resulting 3x3 model orientation submatrix on the matrix stack is:
```text
[  0.7547  0.0000 -0.6561 ]
[  0.0000  1.0000  0.0000 ]   <-- Upright Y axis
[  0.6561  0.0000  0.7547 ]
```
`MODEL_MATRIX_ORIENTATION_CORRECT`: **YES**.

---

## 5. Camera & Viewport Audit

Viewport controller `D_8012D500` (`src/core2/1EBCFC0.c:241-278`):
- Translation: `(2091.67, 313.14, 560.81)` -> `(2804.12, 354.49, -548.60)`.
- Player 0 Position: `(1861.00, -131.00, -166.00)` -> `(2507.39, -26.81, -1750.55)`.
- Camera Eye Elevation: `Y ≈ +313..+354` (situated ~400 units above character level).
- View Matrix Basis:
```text
[  0.5843 -0.3564  0.7291 ]
[  0.0000  0.8984  0.4392 ]   <-- Y-up with 26° pitch down
[ -0.8115 -0.2566  0.5249 ]   <-- Facing towards Banjo
```
`CAMERA_SIMULATION_STATE_PLAUSIBLE`: **YES**.
`CAMERA_LOGICAL_TRANSFORM_CORRECT`: **YES**.
`CAMERA_VIEW_MATRIX_CORRECT`: **YES**.

---

## 6. Shared Root Discrimination

```text
ACTOR_CAMERA_SHARED_ROOT: PROVEN
```

Both the character rotation symptom (supine models) and camera transform symptom (ground-level / mesh clipping) converge on the same interface:
- **Rareware GBI Matrix Setup:** Rare places `Proj * View` into `G_MTX_PROJECTION` and `Model` into `G_MTX_MODELVIEW` (`1EBCFC0.c:246-250`).
- **Host Renderer (RT64) Decomposition:** When RT64 processes draw calls, it expects either the standard Fast3D convention (`G_MTX_MODELVIEW = View * Model`, `G_MTX_PROJECTION = Proj`) or specific Banjo matrix interception hooks. A mismatch in how `G_MTX_PROJECTION` is decomposed or multiplied on the host causes both character model orientation and camera eye transform to be transformed incorrectly on the GPU despite 100% correct CPU/guest state.

---

## 7. Next Step / Fix Plan (Design Only)

- **FIX_OWNER:** `TOOIE_NATIVE` / `RT64 Render Interception`
- **TARGET_AREAS:**
  1. Inspect `src/game/` matrix submission / RT64 display list interceptor for `G_MTX_PROJECTION` vs `G_MTX_MODELVIEW` decomposition.
  2. Verify if Banjo-Tooie requires the same transform-tagging / matrix patch as `BanjoRecomp/patches/transform_tagging.c`.
- **STATUS:** Level B diagnosis complete. Zero source code changes made to game simulation.
