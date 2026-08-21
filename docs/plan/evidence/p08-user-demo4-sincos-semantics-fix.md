---
title: "P08 — USER_DEMO_4 sincos MIPS round.w.s semantics fix"
description: "Architecture-level resolution of func_80013818 table-index drift: MIPS round.w.s ties-to-even was emitted as C lroundf ties-away-from-zero."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, mips, fpu, round-w-s, n64recomp, sincos, angrylion, rt64]
---

# P08 — USER_DEMO_4 sincos MIPS semantics fix

## Verdict

`func_80013818` has an architecture-level recompilation defect.  The guest
instruction at `0x80013848` is **`round.w.s $f4, $f4`**, not `trunc.w.s`.
MIPS `ROUND` uses round-to-nearest with ties-to-even, independent of the FCSR
rounding-mode field.  The static generator emitted `lroundf`, whose C contract
breaks ties away from zero.

For the exact bind-tick operands, `0x80013844` produces `0x4ADAA651`, exactly
`7164712.5`.  MIPS selects the even integer `0x006D5328`; `lroundf` selected
`0x006D5329`.  Angrylion therefore agrees with the architectural result.

## Guest sequence

| PC | Guest instruction | Purpose |
| --- | --- | --- |
| `80013818` | `mul.s f0, f12, f14` | angle in turns |
| `80013828` | `trunc.w.s f0, f0` | whole-turn extraction |
| `8001382C` | `cvt.s.w f0, f0` | restore whole-turn float |
| `80013830` | `mul.s f0, f0, f16` | whole-turn angle |
| `80013838` | `sub.s f12, f12, f0` | remainder |
| `8001383C` | `mul.s f12, f12, f14` | normalized remainder |
| `80013844` | `mul.s f4, f12, f16` | table coordinate |
| `80013848` | `round.w.s f4, f4` | nearest-even index |
| `80013858..64` | `andi` / `mtc1` / `srl` / `andi` | fraction and table offsets |

## Bit replay

```text
INPUTS:                   f12=4399bcf1 f14=3b360b61 f16=43b40000
MUL_INPUTS_MATCH:         YES
MUL_EXPECTED_FLOAT32_BITS: 4adaa651
MUL_RECOMP_FLOAT32_BITS:   4adaa651
TRUNC_INPUTS_MATCH:        N/A (guest opcode is round.w.s)
ROUND_EXPECTED_INT:        006d5328
PRE-FIX_RECOMP_INT:        006d5329
FIRST_SEMANTICALLY_WRONG_PC: 80013848
FCSR_RELEVANT:             round.w.s ignores FCSR rounding mode
```

`trunc.w.s` remains a toward-zero cast.  `cvt.w.s` remains FCSR-aware through
`lrintf`; neither instruction class caused this event.  The affected contract
is `round.w.s`, `round.w.d`, `round.l.s`, and `round.l.d`.

## Production change

`N64Recomp/include/recomp.h` now owns reusable nearest-even helpers.  The C
generator emits `ROUND_*` helpers for future output, and the LiveRecomp
generator uses the same helpers.  Because `RecompiledFuncs/*` is immutable,
the Tooie `RecompiledFuncs` target enables a post-`<math.h>` compatibility
mapping for existing generated `lround*` calls.  No generated source, camera
code, renderer, or speed code was edited.

`tests/mips_rounding_test.cpp` covers adjacent half-way values and replays the
USER_DEMO_4 arithmetic without camera-specific behavior.  It passes, and the
complete `RecompiledFuncs` target compiles with the compatibility mapping.

## Validation status

```text
GUEST_OPCODE:                          round.w.s
CORRECT_TIE_RULE:                      nearest-even
ROUND_W_S_ROOT_CAUSE:                  PROVEN
ROUND_W_S_ORIGINAL_DISCRIMINATOR_FIXED: YES
ROUND_W_S_BIND_AND_IDLE_CAMERA_EFFECT: FIXED
ORIGINAL_BIND_TICK_DISCRIMINATOR:      RESOLVED
IDLE_CAMERA_THROUGH_GLIDX_38:          BIT_IDENTICAL_POST_FIX
CAMERA_COMPLETE_FIX:                   NO
CAMERA_FEEDBACK_ACCUMULATION:          STILL_PRESENT
NEXT_CAMERA_DISCRIMINATOR:             first walking-player ncbapivot tick
FIX_IMPLEMENTED:                       YES (architectural ROUND.W.S only)
GENERATED_C_EDITED:                    NO
RECOMPILEDFUNCS_CHANGED:               NO
PLAYER_SPEED_CODE_CHANGED:             NO
ROUND_W_S_HALF_EVEN_TESTS:             PASS (+/- 0.5, 1.5, 2.5, 3.5)
ROUND_W_S_SPECIAL_VALUE_CONTRACT:      PARTIAL
  (masked Invalid destination-register result is 2^31-1 / 2^63-1;
   FCSR exception flags and enabled traps remain unsupported)
REGRESSION_TEST:                       PASS (mips_rounding_test; -UNDEBUG)
CUMULATIVE_PATCH_CLEAN_APPLY:          PASS (base 2b6f05688de2abc7d86da5b4a89b84c2c6acbabe; --whitespace=error)
N64RECOMP_RELEVANT_TESTS:              PASS (PreemptionSafepointTest 75/0)
TOOIERECOMP_BUILD:                     PASS
COMMIT:                                5854872c79a7cab1c9395ddcd173cece449ffe11
                                       (`fix: preserve MIPS round.w.s semantics`;
                                        CAMERA_COMPLETE_FIX=NO)
```

The cumulative patch was reconstructed from the clean registered base plus all
active protected N64Recomp changes, including same-section JAL and safepoints.
It applies cleanly (`tools/apply_runtime_patches.sh` reports already applied;
`git apply --reverse --check --whitespace=error` succeeds).

## Live Angrylion/RT64 oracle replay (2026-08-21, Grok 4.6 High)

Hardware-faithful reference remains Mupen Pure Interpreter + Angrylion RDP Plus
nightly-build + CXD4 LLE. TooieRecomp stayed on RT64. DISPLAY=:0 X11, RADV
POLARIS10 / Mesa 25.2.8. `xdpyinfo` PASS; short RT64 window-init smoke PASS
(`SDL Video Driver: x11`, `AMD Radeon RX 580 Series (RADV POLARIS10)`).

Bind-tick (`TOOIE_UD4_FIRST_TICK=1`, FIRST_TICK_STOP after mode=2 glidx=0):

```text
POST_FIX_YAW_INPUT_BITS:           4399bcf1 / 4399bcf1
POST_FIX_ROUND_SOURCE_BITS:        4adaa651 (Angrylion 80013844; architectural replay)
POST_FIX_TABLE_INDEX_RT64:         006d5328
POST_FIX_TABLE_INDEX_REFERENCE:    006d5328
POST_FIX_TABLE_INDEX_MATCH:        YES
POST_FIX_SIN_BITS_RT64:            bf4b29cc
POST_FIX_SIN_BITS_REFERENCE:       bf4b29cc
POST_FIX_SIN_BITS_MATCH:           YES
POST_FIX_SP48_XYZ:                 c43efa3b,43c49665,c4a6b798
POST_FIX_SP48_MATCH:               YES
POST_FIX_NODE74_XYZ:               c43efa3b,43c49665,c4a6b798
POST_FIX_NODE74_MATCH:             YES
```

Full USER_DEMO_4 (stage=0 sub=4, join by glidx+pad, 877/877 pad match).
Post-ROUND.W.S full-state join: [[p08-user-demo4-post-round-full-state-discriminator]].

```text
IDLE_FOLLOW_CAM_BIT_IDENTITY_GLIDX_1_38: YES (CA8B4/CA628)
FIRST_CAMERA_BIT_DIFFERENCE_AFTER_FIX: dump glidx 39 ncbapivot output
  AN eye_in=c43e5a36,43c4a193,c4a62d54 src2=43abb181,43987693
  RT eye_in=c43e67de,43c4a193,c4a629cd src2=43abb233,43987635
  player pos/yaw/ideal MATCH at that tick
DEMO_SYNC_FIRST_PRINTED_SPLIT:     glidx 41 camctrl 306.0416 vs 306.1269 (0.0853°)
CAMCTRL_DELTA_GLIDX_41:            0.0853 deg
CAMERA_FEEDBACK_ACCUMULATION:      STILL_PRESENT
ROUND_W_S_FULL_CAMERA_FIX:         NO
EGG_INDEPENDENT_FROM_REMAINING_CAMERA_STATE: UNKNOWN
CAMCTRL_DELTA_GLIDX_50:            12.0481 deg
CAMCTRL_DELTA_GLIDX_100:           112.6789 deg
PLAYER_XZ_DELTA_GLIDX_50:          27.224
PLAYER_XZ_DELTA_GLIDX_100:         829.493
CAMERA_FEEDBACK_ACCUMULATION:      STILL_PRESENT
PLAYER_TRAJECTORY_PARITY:          FAIL (not bounded non-gameplay noise)
POST_FIX_PLAYER_SPEED_MAGNITUDE:   PASS (same 0 / 217.50 / 299.42 / 386.51 ramp; 1-sample physics phase)
POST_FIX_PLAYER_DIRECTION:         FAIL after walk-start (yaw tracks the remaining camera split)
POST_CAMERA_FIX_REFERENCE_EGG:     FIRE (cheggfire 535)
POST_CAMERA_FIX_RT64_EGG:          BLUE (cheggnormal 539)
EGG_PARITY_DEFECT:                 PROVEN
EGG_INDEPENDENT_FROM_ROUND_W_S:    YES
EGG_INDEPENDENT_FROM_REMAINING_CAMERA_STATE: UNKNOWN
ATTRACT_SOAK:                      PASS (~460s demo-sync / 540s process; timeout stop; sub 0..11 including map 395; no SIGSEGV)
CRASH:                             NO
NEW_VISIBLE_REGRESSION:            NOT_INSPECTED (unattended soak)
```

The bind-tick `round.w.s` seed is **PROVEN** repaired and idle follow-cam glidx 1..38 is now bit-identical. This architectural checkpoint does **not** claim `CAMERA_FIXED`. The historical claim that this seed is sufficient to remove the glidx-41 0.085° `camctrl` split is **CORRECTED/SUPERSEDED**: the first remaining bit difference is dump glidx 39 `ncbapivot_entrypoint_3` output with matching player pos/yaw/ideal ([[p08-user-demo4-post-round-full-state-discriminator]]). No egg fix.

```text
CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT
CAMERA_COMPLETE_FIX:          NO
```

Next bounded action: instruction-join the first walking-player `ncbapivot`
at dump glidx 39. Do not reopen `round.w.s` ties-to-even. Do not claim a
complete camera fix.
