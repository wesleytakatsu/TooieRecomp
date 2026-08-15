---
title: "P08 — stick-angle secondary-entry fix"
description: "Tooie-native shared-tail override for the empty generated func_80013B70 (0x80013B70). Cardinals 0/90/180/270 proven on the native body; full func_80013B7C call is not equivalent."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, stick-angle, func_80013B70, bastick, atan2, secondary-entry]
---

# P08 — STICK-ANGLE SECONDARY-ENTRY FIX

```text
BASE_HEAD:                         9c67d0143c039b90c2860395317a0671e1ecbf76
MODEL:                             Grok 4.6 HIGH
MODE:                              NARROW IMPLEMENTATION + VALIDATION
MIPS_13B70_CONFIRMED:              YES
SHARED_TAIL_ENTRY_PC:              0x80013B90
FULL_13B7C_CALL_EQUIVALENT:        NO
FIX_OWNER:                         TOOIE_NATIVE
IMPLEMENTATION_METHOD:             exact shared-tail native (approach A)
BINDING_METHOD:                    STRONG_SYMBOL
EXPLICIT_FUNCTION_MAP_REGISTRATION: NO
STICK_ANGLE_TEST:                  PASS
THIRD_ARGUMENT_SEMANTICS_TEST:     PASS
RUNTIME_PLAYER_VALIDATION:         BLOCKED_BY_SEPARATE_FRONTEND_REGRESSION
GENERATOR_CHANGED:                 NO
N64RECOMP_CHANGED:                 NO
N64MODERNRUNTIME_CHANGED:          NO
RT64_CHANGED:                      NO
RECOMPILEDFUNCS_MANUAL_EDIT:       NO
```

Diagnosis: [[p08-player-direction-facing-diagnosis]].

## 1. Independent MIPS reconstruction (PROVEN)

Primary source: `banjo-tooie-Recomp/asm/core1/1E2B200.s`
(matches `banjotooie_decompressed.elf` symbols `func_80013B70` / `func_80013B7C`).

```text
FUNC_13B70_MIPS:
  80013B70  mtc1   $a2, $f16
  80013B74  j      .L80013B90          ; 0x80013B90 = func_80013B7C + 0x14
  80013B78  nop

FUNC_13B7C_PREFIX:
  80013B7C  mul.s  $f16, $f12, $f12    ; y*y
  80013B80  nop
  80013B84  mul.s  $f0,  $f14, $f14    ; x*x
  80013B88  add.s  $f0,  $f0,  $f16
  80013B8C  sqrt.s $f16, $f0           ; overwrite f16

SHARED_TAIL_ENTRY_PC:  0x80013B90

F16_AT_SHARED_TAIL_FOR_13B70:
  bits of $a2 / ctx->r6 (caller-supplied distance)

F16_AT_SHARED_TAIL_FOR_13B7C:
  sqrt(y*y + x*x) computed by the preamble

FULL_13B7C_CALL_EQUIVALENT: NO

WHY:
  The 13B7C preamble always recomputes and overwrites $f16, discarding $a2.
  The 13B70 entry exists specifically to skip that sqrt. A full 13B7C call
  is therefore a different function: different f16, different intermediate
  f0, and a last-bit-different divisor when the caller's hypot add-order
  is x*x+y*y (bastick) rather than y*y then x*x (13B7C).
```

Shared tail (from `0x80013B90`):

```text
mtc1  $zero, $f10
mov.s $f18, $f14                 ; save x
if (y == 0):
    return (x >= 0) ? 0.0 : 180.0
if (x == 0):
    return (y > 0) ? 90.0 : 270.0
f14 = 0x3E340000                 ; 0.17578125
f12 = y / f16
jal   func_80013ABC
if (saved_x < 0) f0 = 180 - f0
if (f0 < 0)      f0 = 360 + f0
return f0
```

Constants (IEEE bits):

| bits | value |
| --- | --- |
| `0x42B40000` | 90.0 |
| `0x43340000` | 180.0 |
| `0x43870000` | 270.0 |
| `0x43B40000` | 360.0 |
| `0x3E340000` | 0.17578125 |

The DeepSeek diagnosis proposed `f16 = r6; func_80013B7C(...)`. That is
**not** the implementation used here.

## 2. Caller contract (PROVEN)

```text
REAL_CALLER_COUNT:                     10
R6_IS_PRECOMPUTED_DISTANCE:            YES
R6_CAN_DIFFER_FROM_RECOMPUTED_LENGTH:  YES
```

Every real `jal func_80013B70` found in the decomp:

| caller | f12 | f14 | r6 / $a2 |
| --- | --- | --- | --- |
| `bastick_update` | `value[1]` (Y) | `value[0]` (X) | `sqrt(x*x+y*y)` then clamp *after* the call |
| `func_80016A74` | stick y | stick x | `sqrtf` of those two |
| `func_800F18FC` | dx | dz | `sqrt(dx*dx+dz*dz)` |
| `func_800F1A88` | v[0] | v[8] | `sqrt` of those two |
| `func_800F1AFC` | v[0] | v[8] | `sqrt` of those two |
| `subaddiejoy_entrypoint_3` | pad y | pad x | `sqrt` of those two |
| `subaddiejoy_entrypoint_9` | pad y | pad x | `sqrt` of those two |
| `func_808007D4_scradar` | xz[0] | xz[8] | `sqrt` of those two |
| `func_80800A4C_chevilspirit` | xz[0] | xz[8] | `sqrt` of those two |
| `func_80806588_bsfirstp` | s2[0] | s2[8] | `func_800EEFFC` = hypot of the same two |

High-priority `bastick_update` (`src/core2/bastick.c`):

```c
self->stick->distance = sqrtf(SQ(value[0]) + SQ(value[1]));
if (self->stick->distance != 0.0f)
    self->stick->angle = func_80013B70(value[1], value[0], distance);
if (self->stick->distance > 1.0f)
    self->stick->distance = 1.0f;
```

```text
f12:  stick Y (value[1])
f14:  stick X (value[0])
r6:   float bits of the unclamped precomputed distance
```

`R6_CAN_DIFFER` is **YES** because of precision/rounding and add-order
(`x*x+y*y` vs `y*y+x*x`), and because the secondary entry's contract is
"use the supplied third argument", not "recompute hypot". Deadzone/zone
markers run *after* the angle call and do not change r6. No caller was
found that passes a geometrically different length (for example 1.0, or
a 3D hypot of unused components).

## 3. Angle convention (PROVEN)

Reverified from the axis-aligned MIPS immediates, not from atan2 folklore.

```text
ANGLE_CONVENTION:  CCW degrees in [0, 360)
  0°   = +X  (RIGHT, y=0, x>0)
  90°  = +Y  (UP,    y>0, x=0)
  180° = -X  (LEFT,  y=0, x<0)
  270° = -Y  (DOWN,  y<0, x=0)
```

Expected approximate diagonals through the shared tail + `func_80013ABC`:

```text
(+1,+1) ≈ 45°
(-1,+1) ≈ 135°
(-1,-1) ≈ 225°
(+1,-1) ≈ 315°
```

Non-unit vectors use `y / supplied_distance` as the `func_80013ABC` argument.
`(0,0)` would return 0° (`c.ole.s 0, x` is true) but `bastick_update` skips
the call when distance is 0. `-0.0` compares equal to `+0.0` on both axes.

## 4. Chosen implementation

```text
FIX_OWNER:              TOOIE_NATIVE
IMPLEMENTATION_METHOD:  A — native implementation of the exact shared-tail math
OVERRIDDEN_FUNCTIONS:   func_80013B70
FILE:                   src/game/boot_hw_stubs.cpp
```

Preference order:

- **A used.** Native translation of `0x80013B90` onward, including the
  `func_80013ABC` call and the 180/360 adjust.
- **B rejected.** No already-exposed helper is the shared tail.
- **C rejected.** Full `func_80013B7C` is not equivalent (section 1).

`atan2f` / libm was not used. Cardinals are the original immediates.
The general path keeps the original table helper.

`func_80013B70` is also listed in `tooie.us.rev0.recomp.toml` `stubs[]`,
so N64Recomp emits an empty body even before the mid-function `j`. The
toml stub list was **not** changed (that would require regenerating
`RecompiledFuncs`, which this task forbids).

## 5. Binding (PROVEN)

```text
GENERATED_SYMBOL_BINDING:              WEAK  (Clang RECOMP_FUNC)
NATIVE_SYMBOL_BINDING:                 STRONG (T)
LOOKUP_80013B70_RESOLVES_TO_NATIVE:    YES
DIRECT_CALL_RESOLVES_TO_NATIVE:        YES
EXPLICIT_ADD_LOADED_FUNCTION_REQUIRED: NO
```

`RecompiledFuncs/recomp_overlays.inl`:

```c
{ .func = func_80013B70, .offset = 0x00001B40, .rom_size = 0x0000000C },
```

`nm -C build/TooieRecompiled` after this fix:

```text
000000000011d4d0 T func_800137D4
000000000011d5c0 T func_80013A7C
0000000000621ed0 W func_80013AA0
0000000000801220 W func_80013AAC
000000000011d630 T func_80013B70
00000000009d3000 W func_80013B7C
```

No `add_loaded_function(0x80013B70, ...)` was added. Precedent matches
`func_800137F4` / `func_80013A7C`.

## 6. Tests

`tests/stick_angle_thunk_test.cpp` calls the **actual** native
`func_80013B70`. It does not reimplement the shared tail. Cardinals never
reach `func_80013ABC` (MIPS immediates). Diagonals use a sign-preserving
45° stub of `func_80013ABC` so the 180/360 adjust can be observed without
seeding the core1 lookup tables.

```text
STICK_ANGLE_TEST:               PASS
CARDINAL_ANGLES:                RIGHT=0  UP=90  LEFT=180  DOWN=270
DIAGONAL_TEST:                  PASS   (45 / 135 / 225 / 315)
THIRD_ARGUMENT_SEMANTICS_TEST:  PASS
  (3,4) r6=10 → helper f12=0.3
  (3,4) r6=5  → helper f12=0.6
  outputs are not the stick distance
```

A full `func_80013B7C` call would have given the same helper `f12` for both
r6 values.

Regressions:

```text
MATRIX_SUBMISSION_TEST:  PASS
INPUT_DIRECTION_TEST:    PASS
BCF28_TEST:              PASS
CORE1_RA_TEST:           PASS
TOOIE_BUILD:             PASS
```

## 7. Local secondary-entry audit (narrow)

Same `asm/core1/1E2B200.s` cluster only.

```text
LOCAL_SECONDARY_ENTRY_COUNT:   8 tail-share / mid-label entries in 1E2B200.s
OTHER_BROKEN_LOCAL_ENTRIES:    none
```

Generated bodies in this cluster:

| symbol | generated | notes |
| --- | --- | --- |
| `func_800136D0` | instructions present | `b` to own mid-label |
| `func_80013798` | instructions present | `b func_80013818` |
| `func_800137AC` | instructions present | `b func_80013818` |
| `func_800138BC` | instructions present | `b` to own mid-label |
| `func_8001395C` | instructions present | `b` to own mid-label |
| `func_80013AA0` | instructions present | `b func_80013ABC` |
| `func_80013AAC` | instructions present | `b func_80013ABC` |
| `func_80013B70` | **empty** | `j` into `func_80013B7C+0x14` **and** listed in `stubs[]` |

Only `func_80013B70` is empty. No other sister of the 13B7C shared tail
was implemented.

```text
MID_FUNCTION_ENTRY_CODEGEN_GAP:  CORRECTED/SUPERSEDED — see independent review
```

The historical diagnosis is preserved, but the generic generator conclusion is
corrected/superseded. The immediate empty-body cause is the explicit
`STUB_METADATA` entry. A standalone N64Recomp mid-function limitation remains
`INFERENCE`, and the reason for the historical stub entry is `UNKNOWN`.

## 8. Runtime

Clean first run (no direction diagnostics):

```text
DISPLAY=:0
XDG_SESSION_TYPE=x11
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1
timeout -k 2s 50s ./build/TooieRecompiled
exit 137 (timeout)
```

Boot, intro, and overlay dispatch ran. No `Failed to find function`.
No SIGSEGV. 50s stayed in intro (`mode=3 map=0`); Wooded Hollow mode-5
was not required for this function-level fix.

P08-T10 synthetic reproduction (exact frontend sequencer, no direction
override, no frontend-sequencing changes):

```text
TOOIE_AUTO_START=1 TOOIE_DIAG_FRONTEND=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1
timeout -k 2s 60s
```

```text
P08_T10_SYNTHETIC_FLOW_CURRENT:  FAIL
  chgameselect (205) detected at t=22.00s
  CONT_A at t=26.00s
  mode=1 count = 0
  chspiralmountain named overlay = 0
  late state: mode=3 map=0 (intro / chintrochar)
```

This matches the pre-existing file-select → Spiral Mountain regression.
It is **not** part of the `func_80013B70` fix. Do not diagnose it here.

```text
RUNTIME_PLAYER_VALIDATION:       BLOCKED_BY_SEPARATE_FRONTEND_REGRESSION
RIGHT_ONLY_MOVEMENT_RESOLVED:    NOT_RUNTIME_VALIDATED
ATTRACT_FACING_CHANGED:          UNKNOWN
CINEMATIC_FACING_CHANGED:        UNKNOWN
USER_BUG_A_STATUS:               IMPLEMENTED_PENDING_RUNTIME
USER_BUG_B_STATUS:               OPEN_SEPARATE
```

Attract / cinematic facing is a separate mechanism: attract bypasses
`bastick_update` (see the diagnosis). This fix is expected to correct
**live player** stick angle only.

## 9. User-observation split

```text
USER_BUG_A:
  live player — all directions collapse toward screen-right
  owner: func_80013B70
  status: native override implemented and unit-tested;
          not visually confirmed (T10 gameplay gate blocked)

USER_BUG_B:
  some attract / cinematic characters face backward / sideways / spin
  owner: not bastick / not func_80013B70 unless new evidence appears
  status: OPEN_SEPARATE
```

## 10. Scope held

Do not reopen: host input conversion, timing/4:3, render-submission
matrix, PATCH4 / BCF28 / core1 RA wrappers. Shared RT64, N64ModernRuntime,
N64Recomp, and generated `RecompiledFuncs` were not modified.

Logs (ephemeral, not committed):

```text
/tmp/tooie-p08-stick-angle-clean.txt
/tmp/tooie-p08-stick-angle-t10.txt
```

## 11. Independent review pointer and verdict

[[p08-stick-angle-thunk-fix-independent-review]]
[[p08-stick-angle-thunk-fix-correction]]
[[p08-stick-angle-thunk-fix-rereview]]

```text
INDEPENDENT_REVIEW_VERDICT:       C (historical; blocker resolved)
CURRENT_REREVIEW_VERDICT:         A
FIX_ACCEPTED:                     YES (function level)
BLOCKER:                          native 13B70 omits the original
                                  y==0, x>=0 delay-slot $at/r1 clobber
CORRECTION:                       RESOLVED in [[p08-stick-angle-thunk-fix-correction]]
REREVIEW:                         ACCEPTED in [[p08-stick-angle-thunk-fix-rereview]]
RUNTIME_PLAYER_VALIDATION:        NOT_PERFORMED
USER_BUG_A_STATUS:                IMPLEMENTED_PENDING_RUNTIME
USER_BUG_B_STATUS:                OPEN_SEPARATE
```
