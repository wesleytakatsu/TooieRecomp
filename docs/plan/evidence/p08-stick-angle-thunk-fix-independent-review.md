---
title: "P08 — independent review of the func_80013B70 secondary-entry fix"
description: "Independent high-risk semantic review of the Tooie-native stick-angle secondary-entry override."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, independent-review, stick-angle, func_80013B70, secondary-entry]
---

# P08 — INDEPENDENT REVIEW OF THE `func_80013B70` SECONDARY-ENTRY FIX

## Review identity and boundary

```text
BASE_HEAD:                         e60eea4828e3bff803ddde6609888a0329e81e0b
EXPECTED_HEAD:                     e60eea4828e3bff803ddde6609888a0329e81e0b
MODEL:                             GPT-5.6 Luna
REASONING:                         X-HIGH
MODE:                              INDEPENDENT HIGH-RISK SEMANTIC REVIEW
WORKTREE_BEFORE_REVIEW:            CLEAN
GENERATED_CODE_EDITED:             NO
N64RECOMP_EDITED:                  NO
N64MODERNRUNTIME_EDITED:           NO
RT64_EDITED:                       NO
RUNTIME_PLAYER_VALIDATION:         NOT_PERFORMED
```

This review covers only the `func_80013B70` secondary entry, its shared tail,
callers, helper contract, binding, test quality, and the architecture wording
around the generated empty body. The P08-T10 synthetic transition and
attract/cinematic facing remain separate and were not diagnosed.

## 1. Independent MIPS reconstruction

Primary source: `banjo-tooie-Recomp/asm/core1/1E2B200.s`.

```text
func_80013B70:
  80013B70  mtc1  $a2, $f16
  80013B74  j     .L80013B90
  80013B78  nop

func_80013B7C preamble:
  80013B7C  mul.s $f16, $f12, $f12
  80013B80  nop
  80013B84  mul.s $f0,  $f14, $f14
  80013B88  add.s $f0,  $f0, $f16
  80013B8C  sqrt.s $f16, $f0
```

The shared tail begins at `0x80013B90`:

```text
80013B90  mtc1  $zero, $f10
80013B94  mov.s $f18, $f14
80013B98  c.eq.s $f10, $f12
80013B9C  bc1fl .L80013BC8       ; delay: c.eq.s $f10, $f14
80013BA4  c.ole.s $f10, $f14
80013BA8  bc1t  .L80013BBC       ; delay: lui $at, 0x4334
80013BB0  mtc1  $at, $f0
80013BB4  jr    $ra               ; delay: nop
80013BBC  jr    $ra               ; delay: mov.s $f0, $f10
80013BC4  c.eq.s $f10, $f14
80013BC8  bc1fl .L80013BFC       ; delay: lui $at, 0x3E34
80013BD0  c.olt.s $f10, $f12
80013BD4  bc1t  .L80013BE8       ; delay: lui $at, 0x4387
80013BDC  mtc1  $at, $f0
80013BE0  jr    $ra               ; delay: nop
80013BE8  lui   $at, 0x42B4
80013BEC  mtc1  $at, $f0
80013BF0  jr    $ra               ; delay: nop
80013BFC  mtc1  $at, $f14
80013C00  or    $a3, $ra, $zero
80013C04  jal   func_80013ABC    ; delay: div.s $f12, $f12, $f16
80013C0C  c.lt.s $f18, $f10
80013C10  or    $ra, $a3, $zero
80013C14  bc1f  .L80013C28       ; delay: lui $at, 0x4334
80013C1C  mtc1  $at, $f2
80013C24  sub.s $f0, $f2, $f0
80013C28  c.lt.s $f0, $f10
80013C2C  bc1f  .L80013C40       ; delay: lui $at, 0x43B4
80013C34  mtc1  $at, $f2
80013C38  jr    $ra               ; delay: add.s $f0, $f2, $f0
80013C40  jr    $ra               ; delay: nop
```

`FULL_FUNC_80013B7C_CALL_EQUIVALENT: NO`. Calling the full `13B7C` entry
recomputes `sqrt(y*y + x*x)` and overwrites the caller-supplied `$a2` distance;
the `13B70` entry intentionally skips that preamble.

## 2. Branch and delay-slot review

The native implementation reproduces the branch-likely control flow for the
ordinary finite caller domain:

| Native path | Original control semantics | Result |
| --- | --- | --- |
| `y == 0` | `bc1fl` is not taken and its `c.eq.s x` delay slot is annulled; `c.ole.s 0,x` selects `0` or `180` | PASS |
| `y != 0, x == 0` | `bc1fl` is taken and executes its delay-slot `c.eq.s`; the second likely branch annuls `lui 0x3E34` when `x == 0`; `c.olt.s 0,y` selects `90` or `270` | PASS |
| `x != 0` | likely branch reaches the helper path; delay-slot `lui 0x3E34` initializes `$f14` | PASS |
| `saved_x < 0` | non-likely `bc1f` always executes delay-slot `lui 0x4334` before the optional `180 - f0` | PASS |
| `f0 < 0` | non-likely `bc1f` always executes delay-slot `lui 0x43B4` before the optional `360 + f0` | PASS |

One delay-slot side effect is missing. On the `y == 0, x >= 0` path,
`bc1t .L80013BBC` executes its non-annulled delay slot
`lui $at, 0x4334`; the native path assigns `f0 = 0.0f` but does not assign
`ctx->r1`. This is a real guest-register mismatch even though callers found in
this review do not consume `$at` afterward.

## 3. Register and FPR side effects

```text
R1_SIDE_EFFECT_CORRECT:             NO
R6_SIDE_EFFECT_CORRECT:             YES (read as supplied distance; not modified)
R7_SIDE_EFFECT_CORRECT:             YES (helper path saves original $ra in $a3)
R31_SEMANTICS_CORRECT:              YES at caller-visible return
FPR_SIDE_EFFECTS_COMPATIBLE:        YES
```

The original helper sequence is:

```text
or   $a3, $ra, $zero
jal  func_80013ABC
     div.s $f12, $f12, $f16
...
or   $ra, $a3, $zero
```

The native implementation has the corresponding `r7 = r31`, supplied-distance
division, helper call, and `r31 = r7` restore. The generated helper provides
the expected `f0` result and its own clobbers. The final caller-visible `$ra`
and the relevant FPR state therefore match. The missing `$at` write on the
zero-angle path is nevertheless a semantic defect in the native replacement.

## 4. Floating-point comparisons and constants

For finite values, `+0.0`, and `-0.0`, the C++ `==`, `<=`, and `<` tests are
equivalent to the reachable ordered COP1 operations `c.eq.s`, `c.ole.s`, and
`c.olt.s`. NaN/Inf behavior is not a realistic caller domain for this input
path and is not used as a blocker.

The constants were checked by their exact bit patterns and uses:

| IEEE bits | value | use |
| --- | ---: | --- |
| `0x3E340000` | `0.17578125` | helper `f14` scale |
| `0x42B40000` | `90.0` | positive-Y cardinal |
| `0x43340000` | `180.0` | negative-X cardinal and quadrant wrap |
| `0x43870000` | `270.0` | negative-Y cardinal |
| `0x43B40000` | `360.0` | negative-angle wrap |

No host literal is used in place of these constants on the native paths.

## 5. Helper call semantics

`func_80013ABC` receives `f12 = y / supplied_distance`,
`f14 = 0x3E340000`, and the original `f16` distance. The native call also
preserves `f18 = x`, `f10 = 0`, and the `$a3/$ra` save-and-restore pattern
needed by the shared tail. `HELPER_CALL_SEMANTICS: PASS` for the observable
guest state and result used by this function.

## 6. Caller audit

`CALLER_COUNT_CORRECT: YES` — ten real `jal 0x80013B70` sites were found:

```text
funcs_118.c  0x800F1AC4   f12=vector[0], f14=vector[8], r6=sqrt result
funcs_13.c   0x8009F0C0   f12=vector[0x58], f14=vector[0x54], r6=stored length
funcs_15.c   0x808003C4   f12=stack[0x34], f14=stack[0x30], r6=sqrt result
funcs_269.c  0x80806734   f12=s2[0], f14=s2[8], r6=hypot result
funcs_305.c  0x80800AC0   f12=stack[0x34], f14=stack[0x3C], r6=sqrt result
funcs_361.c  0x80016AD0   f12=stack[0x24], f14=stack[0x20], r6=sqrt result
funcs_365.c  0x800F1B38   f12=vector[0], f14=vector[8], r6=sqrt result
funcs_379.c  0x80800184   f12=stack[0x34], f14=stack[0x30], r6=sqrt result
funcs_87.c   0x800F1954   f12=stack[0x24], f14=stack[0x1C], r6=sqrt result
funcs_98.c   0x80800838   f12=stack[0x38], f14=stack[0x40], r6=sqrt result
```

`ALL_CALLERS_SUPPLY_VALID_DISTANCE: YES` for the original contract.
`SUPPLIED_DISTANCE_SEMANTICALLY_REQUIRED: YES`: `bastick_update` computes
`sqrt(x*x + y*y)` and calls before its clamp, while `13B7C` would recompute
in the opposite add order. `CAN_SUPPLIED_DISTANCE_DIFFER_FROM_13B7C_RECOMPUTE:
YES` by the explicit third-argument contract and possible last-bit rounding
differences.

## 7. Angle convention

```text
RIGHT = 0 degrees
UP    = 90 degrees
LEFT  = 180 degrees
DOWN  = 270 degrees
CCW degrees in [0, 360)
```

The general path produces approximately `45/135/225/315` for the four
diagonals with the helper's signed result and the original 180/360 adjustments.

## 8. Binding review

The generated body in `RecompiledFuncs/funcs_152.c` is empty and the symbol is
weak in the built artifacts. The native `extern "C" func_80013B70` is strong
(`T` in `nm`). `recomp_overlays.inl` still contains:

```c
{ .func = func_80013B70, .offset = 0x00001B40, .rom_size = 0x0000000C },
```

Thus direct calls resolve the strong native symbol and lookup by
`0x80013B70` resolves the overlay-table function pointer. Production does not
need `add_loaded_function(0x80013B70, ...)`; the test's local registration is
scaffolding only. `LOOKUP_BINDING: PASS`.

## 9. Test-quality review

The focused test calls the actual native body, covers cardinals, diagonals,
the supplied-distance contract, and lookup binding. The regression tests and
build all pass (see §10).

The test can falsely pass the current implementation because `call_13b70`
resets `recomp_context`, `$r1` starts at zero, and no assertion checks the
`y == 0, x >= 0` `$at` delay-slot result `0x43340000`. It also does not expose
the full helper clobber set, although it does verify the critical `f12/f14/f16`
inputs and the angle result. `TEST_QUALITY: ISSUE`.

No assertion was added: the production correction is intentionally out of
scope after the blocking C verdict, and the requested validation set must not
be left failing by this review.

## 10. Validation

```text
stick_angle_thunk_test: PASS
matrix_submission_test: PASS
input_direction_test: PASS
bcf28_helpers_test: PASS
core1_ra_wrappers_test: PASS
TooieRecompiled build: PASS
RUNTIME_PLAYER_VALIDATION: NOT_PERFORMED
```

The passing tests prove the tested angle, helper-input, binding, and regression
contracts; they do not prove complete register-state equivalence.

## 11. Architectural classification correction

The historical fix note said `MID_FUNCTION_ENTRY_CODEGEN_GAP: PROVEN` based on
the empty generated body and the jump into `func_80013B7C`. That conclusion
was confounded by explicit metadata:

```text
EMPTY_BODY_IMMEDIATE_CAUSE:           STUB_METADATA
MID_FUNCTION_ENTRY_GENERATOR_LIMITATION: INFERENCE
WHY_13B70_WAS_IN_STUBS:                UNKNOWN
```

It is proven that `func_80013B70` is intentionally listed in
`tooie.us.rev0.recomp.toml` `stubs[]`. It is not proven from this review that
N64Recomp cannot represent the mid-function entry when the symbol is not
explicitly stubbed, nor why the metadata entry was originally chosen. The
historical diagnosis remains preserved, but its generic generator claim is
`CORRECTED/SUPERSEDED` to the classification above.

## 12. Verdict

```text
VERDICT:                            C
FIX_ACCEPTED:                      NO
BLOCKER:                           Native 13B70 omits the original $at/r1
                                   delay-slot clobber on y==0, x>=0.
USER_BUG_A_STATUS:                 NOT_RESOLVED
USER_BUG_B_STATUS:                 OPEN_SEPARATE
```

The next action is to correct only the missing native `$r1` side effect and
then rerun this review's focused test/build set. Do not use this blocker to
reopen P08-T10, attract/cinematic facing, transform tagging, N64Recomp,
N64ModernRuntime, RT64, or generated `RecompiledFuncs`.

## Handoff

```text
=== P08 STICK ANGLE FIX INDEPENDENT REVIEW ===

BASE_HEAD:                          e60eea4828e3bff803ddde6609888a0329e81e0b
MIPS_13B70_MATCH:                   YES
SHARED_TAIL_MATCH:                  PARTIAL (r1 delay-slot mismatch)
FULL_13B7C_CALL_EQUIVALENT:         NO
BRANCH_DELAY_SEMANTICS:             FAIL
R7_SIDE_EFFECT_CORRECT:             YES
R31_SEMANTICS_CORRECT:              YES
FPR_SIDE_EFFECTS_COMPATIBLE:        YES
FLOAT_COMPARISON_DOMAIN:            PASS
CALLER_COUNT:                       10
ALL_CALLERS_VALID:                  YES
SUPPLIED_DISTANCE_REQUIRED:         YES
ANGLE_CONVENTION:                   CCW [0,360): R=0 U=90 L=180 D=270
HELPER_CALL_SEMANTICS:              PASS
TEST_QUALITY:                       ISSUE
LOOKUP_BINDING:                     PASS
EMPTY_BODY_IMMEDIATE_CAUSE:         STUB_METADATA
MID_FUNCTION_ENTRY_GENERATOR_LIMITATION: INFERENCE
WHY_13B70_WAS_IN_STUBS:             UNKNOWN
STICK_ANGLE_TEST:                   PASS
MATRIX_SUBMISSION_TEST:             PASS
INPUT_DIRECTION_TEST:               PASS
BCF28_TEST:                         PASS
CORE1_RA_TEST:                      PASS
TOOIE_BUILD:                        PASS
RUNTIME_PLAYER_VALIDATION:          NOT_PERFORMED
USER_BUG_A_STATUS:                 NOT_RESOLVED
USER_BUG_B_STATUS:                 OPEN_SEPARATE
VERDICT:                            C
FIX_ACCEPTED:                      NO
EVIDENCE_CREATED:                   docs/plan/evidence/p08-stick-angle-thunk-fix-independent-review.md
COMMIT:                             pending
PUSHED:                             NO
NEXT_SINGLE_ACTION:                correct only the missing r1 side effect

=== END HANDOFF ===
```

## Resolution Note (2026-08-15)

The single blocking finding from this review (missing `$at/$r1 = 0x43340000` on the `y == 0, x >= 0` path) has been corrected and verified in [[p08-stick-angle-thunk-fix-correction]]. The historical verdict C above is preserved intact; formal re-review is pending.
