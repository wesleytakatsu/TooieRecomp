---
title: "P08 — player direction / facing diagnosis"
description: "Root-cause analysis of the user report that all directional input produces right-on-screen movement and that character facing is wrong in attract/cinematic contexts. WASD runtime collapse is now RESOLVED; full controller still PENDING."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, direction, facing, stick-angle, atan2, bastick, func_80013B70]
---

# P08 — PLAYER DIRECTION / FACING DIAGNOSIS

```text
BASE_HEAD:                    2a41007950bba10c43c6b268f396ee714a70f4b4
MODEL:                        DeepSeek V4 Flash HIGH
MODE:                         DYNAMIC ROOT-CAUSE DIAGNOSIS (static + limited dynamic)
RIGHT_ONLY_PLAYER_MOVEMENT:   RESOLVED (manual WASD on HEAD 6a0a696)
ATTRACT_PLAYER_DIRECTION_WRONG: CORRECTED/SUPERSEDED — gameplay demo player
                              uses bastick via glrecord; see
                              [[p08-demo-movement-precision-diagnosis]]
CINEMATIC_FACING_WRONG:       DIAGNOSED_ONE_ACTOR — see
                              [[p08-forced-cinematic-facing-diagnosis]]
BOTS_BEHAVIOR_PLAUSIBLE:      YES — bots bypass bastick/yaw pipeline
USER_BUG_A_FUNCTION_LEVEL:    RESOLVED
USER_BUG_A_WASD_RUNTIME:      RESOLVED
USER_BUG_A_FULL_CONTROLLER:   PENDING
ROOT_CAUSE_LEVEL:             A
ROOT_CAUSE_CLASS:             A (stick angle calculation broken)
FIX_IMPLEMENTED:              YES — see [[p08-stick-angle-thunk-fix]]
```

## 1. Symptom-to-mechanism mapping

| user symptom | mechanism |
|---|---|
| any non-zero input → movement toward right side of screen | stick angle always ≈0–1° (distance), so desired_yaw ≈ camera_yaw + 90° regardless of direction |
| neutral → stops correctly | distance=0 → angle update skipped in bastick_update |
| changing requested direction doesn't change movement | all directions give same tiny angle ≈ distance (0.2–1.0) |
| bots appear correct | bots set `yaw->ideal` directly from AI structs; bypass `bastick_update`/`bastick_getAngleRelativeToBanjo` entirely |
| attract player also wrong | **CORRECTED/SUPERSEDED:** gameplay-demo player uses bastick via `glrecord` recorded pad ([[p08-demo-movement-precision-diagnosis]]). Cinematic/forced facing remains USER_BUG_B. |

## 2. Root cause — specific

**FIRST_BAD_FUNCTION:** `func_80013B70` (VRAM 0x80013B70)

**FUNCTION_ROLE:** Stick-angle atan2 thunk. Receives `(y, x, distance)` and should:
1. Set `f16 = distance` (third arg, passed in `$a2` / `ctx->r6`)
2. Jump to `func_80013B7C + 0x14` (the shared body after the sqrt preamble)

**SIGNATURE:** `f32 func_80013B70(f32 y, f32 x, f32 amplitude)` (from `bastick.c` call: `func_80013B70(stick->value[1], stick->value[0], stick->distance)`)

**GENERATED_BODY** (`RecompiledFuncs/funcs_152.c:2644`):
```c
RECOMP_FUNC void func_80013B70(uint8_t* rdram, recomp_context* ctx) {
    uint64_t hi = 0, lo = 0, result = 0;
    int c1cs = 0;
    ;}
```
The body is **completely empty** — no code beyond dead declarations. Neither f16 is set from r6, nor is func_80013B7C called.

**COMPARISON:** The sister function `func_80013B7C` (which shares the same body at offset +0x14) has a full correct generated body at `RecompiledFuncs/funcs_74.c:1175`.

**NO NATIVE OVERRIDE EXISTS:** `boot_hw_stubs.cpp` overrides `func_800137D4`, `func_800137F4`, `func_80013A5C`, `func_80013A7C`, `func_80013A5C` but NOT `func_80013B70`.

## 3. Downstream error chain

```
bastick_update (0x8009EFF8):
  distance = sqrtf(x² + y²)          ✓ (correct, sqrtf result → f0 → distance field)
  if distance != 0:
    ctx->r6 = distance bits           ✓ (mfc1 $a2, $f2)
    ctx->f12 = Y                      ✓ (value[1])
    ctx->f14 = X                      ✓ (value[0], in delay slot)
    LOOKUP_FUNC(0x80013B70):
        --- EMPTY BODY ---
        f0 unchanged (hold distance from sqrtf)
        returns
    MEM_W(0x5C, stick) = f0.u32l     ✗ WRONG: stores DISTANCE as ANGLE

→ stick->angle ≈ 0.2–1.0 (always distance, never correct angle 0–360°)

bastick_getAngleRelativeToBanjo:
  func_800CAF00(camera_ptr, stick_angle):
    cam_yaw + stick_angle + 90°
    = cam_yaw + distance + 90°

→ desired_yaw ≈ camera_yaw + 90° + small_offset

All inputs collapse to ~same desired yaw ≈ camera's right.
```

### Cardinal expected vs actual

| input | stick_x | stick_y | expected_angle | actual_angle (≈) |
|---|---|---|---|---|
| CENTER | 0 | 0 | undefined | 0 (distance=0, not updated) |
| RIGHT | +1 | 0 | 0° | distance ≈ 1.0 |
| UP | 0 | +1 | 90° | distance ≈ 1.0 |
| LEFT | -1 | 0 | 180° | distance ≈ 1.0 |
| DOWN | 0 | -1 | 270° | distance ≈ 1.0 |

## 4. Call graph — shared vs divergent

```text
LIVE PLAYER PATH:
  recomp_update_inputs → osContGetReadData → func_80015D60 (remap)
  → bastick_update → func_80013B70 [EMPTY → BROKEN] → bastick_getAngleRelativeToBanjo
  → func_8009D088 (yaw controller) → yaw_setIdeal → yaw_update

ATTRACT / GAMEPLAY-DEMO PATH (CORRECTED/SUPERSEDED):
  Historical claim that attract "skips bastick via func_800F468C" is
  over-broad. func_800F468C is a gate that *calls* func_800991B0, and
  that player update includes bastick_update.
  Wooded Hollow gameplay demonstration:
    gsattract mode-5 → func_800A9CAC (D_80127760 != 0)
    → glrecord_entrypoint_0 writes recorded OSContPad
    → func_8001608C remapper → bastick_update → func_80013B70
    → BASTICK USED
  Some cinematic/scripted actors still bypass bastick (USER_BUG_B).

BOT PATH:
  AI struct → func_800F8444 / func_800F45E0 → yaw_setIdeal → same yaw_update
  → BASTICK NOT USED
```

**FIRST_SHARED_DIRECTION_FUNCTION:** `func_80013B70` is shared by live
player bastick, gameplay-demo bastick, `func_800F18FC` world-delta heading,
and `subaddiejoy`. Bots still do not enter `bastick_update`.

**FIRST_PLAYER_VS_BOT_DIVERGENCE:** The entrance to bastick_update (0x8009EFF8). Bots never reach it.

## 5. Dynamic limitation

The synthetic file-select → Spiral Mountain gameplay flow (P08-T10) does not work on the current HEAD (2a41007). After CONT_A confirm, the game transitions to mode=3 (intro) instead of mode=1 (gameplay), and no player state is initialized. Root cause of this regression: unknown — may be timing perturbation from additional diagnostics or a latent file-start failure.

Repeated attempts with:
- `TOOIE_DIAG_FRONTEND=1` only (matching P08-T10 conditions)
- `TOOIE_DIAG_DIRECTION=1` (enables frontend + movement + input)
Both produced the same result: mode=3 after confirm, no Spiral Mountain load.

**This prevented the cardinal-direction dynamic trace.** Root-cause classification relies on static analysis of the generated recompiled output and the Tooie decomp source.

## 6. Root-cause classification

```text
ROOT_CAUSE_LEVEL:             A
ROOT_CAUSE_CLASS:             A (stick angle collapses to wrong value)
ROOT_CAUSE:                   func_80013B70 recompiled body is empty — angle never computed;
                              stick distance stored as angle (~0-1° always); all directions
                              collapse to desired_yaw ≈ camera_yaw + 90° + tiny offset

FIRST_BAD_FUNCTION:           func_80013B70 (0x80013B70)
FIRST_BAD_GUEST_PC:           0x80013B70 (LOOKUP_FUNC entry)
EXPECTED_VALUE:               angle = atan2(y, x) [0–360°]
ACTUAL_VALUE:                 angle = distance [0.0–1.0]

REFUTED_CANDIDATES:
  convert_to_n64_range quadrant collapse        — REFUTED (harness proven bidirectional)
  D_8007DB54 unseeded scale                     — REFUTED (seeded correctly)
  func_80013A7C LOOKUP_FUNC(0) / core1 RA       — RESOLVED (separate earlier fix)
  PATCH4 / Hybrid F' / BCF28                    — REFUTED (not in angle path)
  render-submission matrix split                — REFUTED (separate fix, still good)
  Actor.rotation / CPU model matrix             — REFUTED (upright and correct)
  host timing / presentation rate               — REFUTED (validated baseline)
  SDL axis signedness / deadzone                — REFUTED (bidirectional proven)
  negative-X clipping                           — REFUTED (s8 `lb` path clean)
```

## 7. Minimal fix design — SUPERSEDED by implementation

The sketch below is the original diagnosis proposal. Independent MIPS
reconstruction proved that step 2 (full `func_80013B7C` entry) is **not**
equivalent. The implemented fix materializes `$f16` from `$a2` and runs the
shared tail at `0x80013B90` natively. See [[p08-stick-angle-thunk-fix]].

```text
FIX_OWNER:          TOOIE_NATIVE (src/game/boot_hw_stubs.cpp or a new override file)
OWNING_LAYER:       TooieRecomp source (NOT RecompiledFuncs, NOT N64Recomp, NOT N64ModernRuntime)
FIX_MECHANISM:      Strong-symbol interposition (like func_800137D4, func_80013A7C etc.)
FUNCTION_SEMANTICS: func_80013B70(y, x, distance):
                       1. ctx->f16.u32l = ctx->r6  // third arg (distance) as float
                       2. func_80013B7C(rdram, ctx)  // NOT used — not equivalent
                       3. return  // ctx->f0 holds the angle result

MINIMAL_FILES:
  src/game/boot_hw_stubs.cpp  — add override + register in banjo::register_boot_lookup_stubs()
  (registration was NOT required; overlay table binds the strong symbol by name)

CHECK_BEFORE_FIX:
  - Verify func_80013B7C body (RecompiledFuncs/funcs_74.c:1175) is correct ✓ (appears correct)
  - Verify RA wrapper registration is not required (func_80013B70 is in overlay, not boot)
  - Test: cardinal inputs produce correct stick angles 0/90/180/270°
  - Verify no regression: matrix_submission_test, bcf28_helpers_test, core1_ra_wrappers_test PASS
```

## 8. Evidence artifacts

```text
LOG_FILE (raw, 479775 lines):  /tmp/tooie-p08-direction-run1.txt
  — TOOIE_DIAG_DIRECTION=1, 120s timeout, exit 137
  — mode=3 never reached gameplay
  — 1103 direction traces all show stick=(0,0,0.0) angle=0.00 (no player state init)

LOG_FILE (frontend-only):     /tmp/tooie-p08-frontend-only.txt
  — TOOIE_DIAG_FRONTEND=1, 60s timeout, exit 137
  — same mode=3 regression

GENERATED_BODY (func_80013B70): RecompiledFuncs/funcs_152.c:2644
GENERATED_BODY (func_80013B7C): RecompiledFuncs/funcs_74.c:1175
STICK_ANGLE_CALLER:          RecompiledFuncs/funcs_13.c:4301–4316 (generated bastick_update)
```

## 9. Acceptance test for fix

1. `TOOIE_AUTO_START=1` → reach any state where bastick_update fires with non-zero stick
2. Inject cardinals RIGHT/LEFT/UP/DOWN → bastick angle field reads:
   - RIGHT: stick_angle ≈ 0°
   - UP: stick_angle ≈ 90°
   - LEFT: stick_angle ≈ 180°
   - DOWN: stick_angle ≈ 270°
3. Player desired yaw differs for each cardinal (no collapse)
4. No regression: unit tests PASS, visible attract demo still renders upright

## 10. Resolution pointer

```text
RESOLUTION:  [[p08-stick-angle-thunk-fix]]
STATUS:      native override implemented; cardinal/diagonal/third-arg tests PASS
RUNTIME:     USER_BUG_A_WASD_RUNTIME RESOLVED (manual current-HEAD WASD)
             USER_BUG_A_FULL_CONTROLLER PENDING
             (P08-T10 synthetic file-select → mode=1 still FAIL; separate)
USER_BUG_A:  WASD resolved; full controller pending
USER_BUG_B:  DIAGNOSED_ONE_ACTOR — Wooded Hollow sucoaster
             chdiggerground 801c5d1c/801c5afc reverse walk,
             heading 294.21 vs yaw/unk54 114.21 (exact 180°);
             Level B; see [[p08-forced-cinematic-facing-diagnosis]]
```