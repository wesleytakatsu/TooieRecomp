---
title: "P08 — func_80013B70 review correction (delay-slot r1 side effect)"
description: "Minimal semantic correction to the native func_80013B70 override, reproducing the original non-likely bc1t lui $at, 0x4334 delay-slot clobber on all y==0 paths."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, stick-angle, func_80013B70, delay-slot, review-correction]
---

# P08 — `func_80013B70` REVIEW CORRECTION: DELAY-SLOT `$r1` SIDE EFFECT

## 1. Classification & Review Context

```text
BASE_HEAD:                         002a24d9b1a6e76e0a4f603e355cb4fa01ff3526
MODEL:                             Gemini 3.7 Flash HIGH
MODE:                              MINIMAL CORRECTION + TEST + DOCUMENTATION
LUNA_VERDICT_C_FINDING:            RESOLVED
ORIGINAL_MIPS_RECONFIRMED:         YES
BRANCH_OPCODE:                     bc1t (normal non-likely)
BRANCH_LIKELY:                     NO
DELAY_SLOT:                        lui $at, (0x43340000 >> 16)
EXECUTES_WHEN_BRANCH_TAKEN:        YES
EXECUTES_WHEN_BRANCH_NOT_TAKEN:    YES
R1_REQUIRED_VALUE:                 0x43340000
EMPTY_BODY_IMMEDIATE_CAUSE:        STUB_METADATA
MID_FUNCTION_ENTRY_GENERATOR_LIMITATION: INFERENCE
WHY_13B70_WAS_IN_STUBS:            UNKNOWN
RUNTIME_PLAYER_VALIDATION:         NOT_PERFORMED
USER_BUG_A_STATUS:                 IMPLEMENTED_PENDING_RUNTIME
USER_BUG_B_STATUS:                 OPEN_SEPARATE
```

## 2. Independent MIPS Delay-Slot Verification

Primary source: `banjo-tooie-Recomp/asm/core1/1E2B200.s:385-400`.

```mips
  .L80013B90:
    /* 1E2B6C0 80013B90 */  mtc1       $zero, $f10       # f10 = 0.0f
    /* 1E2B6C4 80013B94 */  mov.s      $f18, $f14        # f18 = x
    /* 1E2B6C8 80013B98 */  c.eq.s     $f10, $f12        # 0.0f == y?
    /* 1E2B6CC 80013B9C */  bc1fl      .L80013BC8        # branch likely if y != 0
    /* 1E2B6D0 80013BA0 */   c.eq.s    $f10, $f14        # delay slot (annulled if y == 0)
    /* 1E2B6D4 80013BA4 */  c.ole.s    $f10, $f14        # y == 0 path: 0.0f <= x?
    /* 1E2B6D8 80013BA8 */  bc1t       .L80013BBC        # branch if x >= 0 (NORMAL non-likely)
    /* 1E2B6DC 80013BAC */   lui       $at, (0x43340000 >> 16) # delay slot: ALWAYS executes!
    /* 1E2B6E0 80013BB0 */  mtc1       $at, $f0          # x < 0 path: f0 = 180.0f
    /* 1E2B6E4 80013BB4 */  jr         $ra
    /* 1E2B6E8 80013BB8 */   nop
  .L80013BBC:
    /* 1E2B6EC 80013BBC */  jr         $ra
    /* 1E2B6F0 80013BC0 */   mov.s     $f0, $f10         # x >= 0 path: f0 = 0.0f
```

Because `80013BA8` is `bc1t` (standard branch) and not `bc1tl`, the delay-slot instruction `lui $at, 0x4334` executes unconditionally on both branch-taken ($x \ge 0$) and branch-not-taken ($x < 0$) paths.

## 3. Native Behavior Before and After

### Before (Historical HEAD `002a24d`):
```cpp
    // y == 0
    if (ctx->f10.fl <= ctx->f14.fl) {
        ctx->f0.fl = ctx->f10.fl; // 0.0 — r1 left unchanged!
        tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
        return;
    }
    ctx->r1 = S32(0x4334 << 16); // 180.0
    ctx->f0.u32l = ctx->r1;
    tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
```

### After (Corrected):
```cpp
    // y == 0
    ctx->r1 = S32(0x4334 << 16); // original bc1t delay-slot LUI (0x43340000)
    if (ctx->f10.fl <= ctx->f14.fl) {
        ctx->f0.fl = ctx->f10.fl; // 0.0
        tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
        return;
    }
    ctx->f0.u32l = ctx->r1; // 180.0
    tooie_diag::note_math("80013B70", input_y, ctx->f0.fl);
```

### Register State Comparison:

| Case | Inputs | Angle (`f0`) | Previous `r1` | Corrected `r1` | Original MIPS `$at` |
| --- | --- | ---: | ---: | ---: | ---: |
| RIGHT | $y=0, x=+1$ | `0.0°` | unchanged | `0x43340000` | `0x43340000` |
| CENTER | $y=0, x=0$ | `0.0°` | unchanged | `0x43340000` | `0x43340000` |
| LEFT | $y=0, x=-1$ | `180.0°` | `0x43340000` | `0x43340000` | `0x43340000` |
| UP | $y=+1, x=0$ | `90.0°` | `0x42B40000` | `0x42B40000` | `0x42B40000` |
| DOWN | $y=-1, x=0$ | `270.0°` | `0x43870000` | `0x43870000` | `0x43870000` |

## 4. Test Suite Extension

The focused unit test `tests/stick_angle_thunk_test.cpp` was updated to:
1. Seed `ctx->r1 = 0xDEADBEEF` before invoking `func_80013B70`.
2. Explicitly assert the exact delay-slot clobber value for each cardinal direction:
   - `RIGHT`: `f0 == 0.0`, `r1 == 0x43340000`
   - `UP`: `f0 == 90.0`, `r1 == 0x42B40000`
   - `LEFT`: `f0 == 180.0`, `r1 == 0x43340000`
   - `DOWN`: `f0 == 270.0`, `r1 == 0x43870000`
   - `CENTER`: `f0 == 0.0`, `r1 == 0x43340000`
3. Retain all existing contracts: diagonal quadrants (45°, 135°, 225°, 315°), third-argument supplied distance (0.3 vs 0.6), small-magnitude distance preservation, and lookup resolution.

## 5. Verification Results

```text
stick_angle_thunk_test:            PASS (100% assertions pass)
matrix_submission_test:            PASS (MAX_MATRIX_ERROR: 0.000000e+00)
input_direction_test:              PASS (bidirectional pad conversion)
bcf28_helpers_test:                PASS (0x800BCCC0 / 0x800BCCD8 bound)
core1_ra_wrappers_test:            PASS (4 RA wrappers bound)
TooieRecompiled build:             PASS
```

## 6. Architecture & Bug Status

- `EMPTY_BODY_IMMEDIATE_CAUSE`: `STUB_METADATA` (`tooie.us.rev0.recomp.toml` `stubs[]`).
- `MID_FUNCTION_ENTRY_GENERATOR_LIMITATION`: `INFERENCE`.
- `WHY_13B70_WAS_IN_STUBS`: `UNKNOWN`.
- `USER_BUG_A`: `IMPLEMENTED_PENDING_RUNTIME` (controllable player movement pending manual/runtime validation).
- `USER_BUG_B`: `OPEN_SEPARATE` (attract/cinematic character facing and rotation).

## 7. Independent re-review resolution

The focused re-review is recorded in
[[p08-stick-angle-thunk-fix-rereview]]. It independently reconfirms the
normal, non-likely `bc1t` and accepts the corrected implementation:

```text
LUNA_ORIGINAL_VERDICT_C_BLOCKER:      RESOLVED
STICK_ANGLE_FUNCTION_LEVEL_STATUS:    RESOLVED
VERDICT:                              A
FIX_ACCEPTED:                         YES
RUNTIME_PLAYER_VALIDATION:            NOT_PERFORMED
```

The historical verdict C remains preserved in
[[p08-stick-angle-thunk-fix-independent-review]]. Acceptance is limited to
the function-level correction; runtime USER_BUG_A validation and separate
USER_BUG_B diagnosis remain open.
