---
title: "P08 — focused independent re-review of the corrected func_80013B70"
description: "Function-level acceptance review of the minimal $r1 delay-slot correction for the Tooie-native stick-angle secondary entry."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, independent-review, rereview, stick-angle, func_80013B70, delay-slot]
---

# P08 — FOCUSED INDEPENDENT RE-REVIEW OF THE CORRECTED `func_80013B70`

## Review boundary

This is a focused re-review of the single blocker from the prior independent
review. It does not reopen P08-T10, attract/cinematic facing, transform
tagging, or any shared runtime/generator code.

```text
BASE_HEAD:                             d37616c961ab12f0bfed6ae1b48b1489946d5c4b
PRIOR_REVIEW_COMMIT:                   002a24d9b1a6e76e0a4f603e355cb4fa01ff3526
CORRECTION_COMMIT:                     d37616c961ab12f0bfed6ae1b48b1489946d5c4b
PRIOR_VERDICT:                         C
PRIOR_BLOCKER:                         missing $at/$r1 = 0x43340000 on y==0, x>=0
PRODUCTION_SCOPE_MINIMAL:              YES
UNRELATED_PRODUCTION_CHANGES:         NONE
```

The correction diff changes only `src/game/boot_hw_stubs.cpp` in production:
it moves the `0x43340000` assignment before the `y == 0` branch and reuses it
for the negative-X result. The test adds a hostile `$r1` seed and exact
register assertions. Documentation changes in the correction commit are not
production behavior.

## 1. Original MIPS reconfirmation

Primary source: `banjo-tooie-Recomp/asm/core1/1E2B200.s`.

```text
80013BA4  c.ole.s  $f10, $f14
80013BA8  bc1t     .L80013BBC
80013BAC   lui     $at, 0x4334
```

The branch is a normal, non-likely `bc1t`. Its delay slot executes whether the
branch is taken or not taken. Since `$f10` is zero at this point:

| input path | branch condition | return `f0` | required `r1` |
| --- | --- | ---: | ---: |
| RIGHT (`y=0`, `x>0`) | taken | `0.0` | `0x43340000` |
| CENTER (`y=0`, `x=0`) | taken | `0.0` | `0x43340000` |
| LEFT (`y=0`, `x<0`) | not taken | `180.0` | `0x43340000` |

The prior C verdict correctly identified that the old native code skipped this
delay-slot clobber on RIGHT and CENTER.

```text
ORIGINAL_MIPS_RECONFIRMED:             YES
BRANCH_OPCODE:                         bc1t
BRANCH_IS_LIKELY:                      NO
DELAY_SLOT_EXECUTES_IF_TAKEN:         YES
DELAY_SLOT_EXECUTES_IF_NOT_TAKEN:     YES
R1_EXPECTED_AFTER_RIGHT:               0x43340000
R1_EXPECTED_AFTER_LEFT:                0x43340000
R1_EXPECTED_AFTER_CENTER:              0x43340000
```

## 2. Corrected native behavior

The corrected native body now performs:

```cpp
ctx->r1 = S32(0x4334 << 16);
if (ctx->f10.fl <= ctx->f14.fl) {
    ctx->f0.fl = ctx->f10.fl;
    return;
}
ctx->f0.u32l = ctx->r1;
```

This exactly covers both outcomes of the original non-likely branch:

```text
RIGHT_PATH_MATCH:                     YES  (f0=0.0,   r1=0x43340000)
CENTER_PATH_MATCH:                    YES  (f0=0.0,   r1=0x43340000)
LEFT_PATH_MATCH:                      YES  (f0=180.0, r1=0x43340000)
R1_SIDE_EFFECT_CORRECT:               YES
```

## 3. Previously accepted semantics

The correction diff does not alter the previously reviewed shared-tail
behavior:

```text
SUPPLIED_DISTANCE_CONTRACT_UNCHANGED:  YES
GENERAL_ANGLE_PATH_UNCHANGED:         YES
HELPER_CALL_UNCHANGED:                YES
R7_R31_UNCHANGED:                     YES
BINDING_UNCHANGED:                    YES
```

`$r6` is still copied into `$f16`; the full `func_80013B7C` entry is still not
called; the helper path, angle convention, and saved `$r7`/`$r31` handling are
unchanged. The generated body remains empty/weak, the overlay table still
contains the 12-byte `func_80013B70` entry at offset `0x1B40`, and the native
symbol remains strong (`T` in `nm`).

## 4. Regression-test quality

`tests/stick_angle_thunk_test.cpp` now seeds `$r1` with `0xDEADBEEF` before
each call and asserts exact expected register values for RIGHT, CENTER, and
LEFT (as well as the existing UP and DOWN cases). The historical implementation
had no `$r1` assignment on RIGHT/CENTER, so the new test would fail there before
the correction; LEFT already assigned the same value and is retained as a
positive coverage case.

Existing coverage remains present and passing:

- lookup path resolves `0x80013B70` to the native body;
- cardinal angles are `0/90/180/270`;
- diagonals are `45/135/225/315`;
- the helper receives the supplied `$r6` distance contract;
- small-magnitude and non-unit supplied-distance cases remain covered;
- helper call count, `f14`, and `$r31` behavior remain checked.

```text
TEST_WOULD_FAIL_PRE_CORRECTION:        YES
DELAY_SLOT_REGRESSION_COVERED:        YES
PREVIOUS_TEST_COVERAGE_PRESERVED:     YES
```

## 5. Validation

```text
STICK_ANGLE_TEST:                     PASS
MATRIX_SUBMISSION_TEST:              PASS
INPUT_DIRECTION_TEST:                PASS
BCF28_TEST:                           PASS
CORE1_RA_TEST:                        PASS
TOOIE_BUILD:                          PASS
RUNTIME_PLAYER_VALIDATION:            NOT_PERFORMED
```

The function-level test output reports the corrected RIGHT, CENTER, and LEFT
`$r1` values. Runtime player validation remains a separate manual/runtime gate;
the broken synthetic P08-T10 flow is not used to judge this function correction.

## 6. Architecture and bug boundaries

These classifications remain unchanged:

```text
EMPTY_BODY_IMMEDIATE_CAUSE:           STUB_METADATA
MID_FUNCTION_ENTRY_GENERATOR_LIMITATION: INFERENCE
WHY_13B70_WAS_IN_STUBS:               UNKNOWN
USER_BUG_A_STATUS:                    IMPLEMENTED_PENDING_RUNTIME
USER_BUG_B_STATUS:                    OPEN_SEPARATE
```

Acceptance of this function does not claim that attract/cinematic characters
face correctly, walk sideways correctly, or avoid inappropriate 360-degree
rotations. Those remain USER_BUG_B and require an independent diagnosis.

## Final verdict

```text
LUNA_ORIGINAL_VERDICT_C_BLOCKER:      RESOLVED
STICK_ANGLE_FUNCTION_LEVEL_STATUS:    RESOLVED
VERDICT:                              A
FIX_ACCEPTED:                         YES
```

The corrected `func_80013B70` is semantically accepted at the function level.
The only prior blocker—the missing non-likely `bc1t` delay-slot `$at/$r1`
clobber on `y == 0 && x >= 0`—is fully resolved. Manual/runtime cardinal
validation remains the next gate before declaring USER_BUG_A resolved.

## Handoff

```text
RUNTIME_PLAYER_VALIDATION:            NOT_PERFORMED
USER_BUG_A_STATUS:                    IMPLEMENTED_PENDING_RUNTIME
USER_BUG_B_STATUS:                    OPEN_SEPARATE
NEXT_SINGLE_ACTION:                   manual/runtime cardinal validation of USER_BUG_A
```
