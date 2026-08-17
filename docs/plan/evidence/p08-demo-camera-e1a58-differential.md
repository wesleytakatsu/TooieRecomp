---
title: "P08 — func_800E1A58 camera pipeline differential"
description: "Bounded original/Recomp function-level differential of the selected camera pipeline. func_800E1A58 is identity on both sides; the first meaningful camctrl is already present as FE14/CA628 src2[1] before CA8B4/E1A58. Overlay computer identified as ncbapivot_entrypoint_3."
phase: P08
status: evidence
tags: [tooie-recomp, p08, measured-original, measured-recomp, camera, func_800E1A58, func_8010FC38, func_8010FE14, func_800CA628, func_80110588]
---

# P08 — func_800E1A58 camera pipeline differential

## Scope

Narrow original-vs-recomp function-level differential only. No production fix.

```text
BASE_HEAD:                    9535803ce9f44926a6de66819fde5b16788402d5
ALIGNMENT:                    same consumed_glidx (function-level)
WINDOW:                       70..95 and 370..410 (detail 382..392)
MODE:                         mode=5, Wooded Hollow demo, map=0
FIX_IMPLEMENTED:              NO
```

## Environment

```text
MUPEN_ENV_REUSED:             YES  (/tmp/p08-m64p)
ROM_SHA1:                     af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:                    YES
MUPEN_CORE_COMMIT:            6dca4c15370ac3e2171ce7b31426695f8f39b460
RICE_COMMIT:                  038882dd2e7cb660fd7512c0745032f17edd0bfa
CPU_MODE:                     Pure Interpreter (R4300Emulator=0, NO_ASM=1)
RECOMP_SOURCE:                build/TooieRecompiled at BASE_HEAD
                              (temporary LOOKUP hooks, reverted before checkpoint)
```

## func_800E1A58 contract (independent MIPS)

```text
E1A58_ENTRY_PC:               0x800E1A58
E1A58_EXIT_PC:                0x800E1BF4  (jr ra; delay addiu $sp, 0x90)
ARG0 (a0):                    pointer to f32[3] out1  (eye; from cam+0x00)
ARG1 (a1):                    pointer to f32[3] out2  (from cam+0x18..+0x20)
FPR_ARGS:                     none
RETURN:                       void
SIDE_EFFECTS:                 may mutate out1/out2 via callees
CALLEES:                      func_800EE7F8 (vec3 copy),
                              func_800EEAD4, func_800F10B4, func_800F0E00
                              (only if a constraint slot +0x76 != 0),
                              func_800E1294 (only if slot type is 2, 3, or 4)
READS:                        D_8012CF90 .. D_8012D468, 10 records × 0x7C
                              each: +0x74 type, +0x75 mode, +0x76 flag,
                              +0x0C/+0x10/+0x14, +0x28/+0x2C
                              type==1 => skip slot
DOES_NOT_READ:                player position, dt/timing scalar
```

**CORRECTED/SUPERSEDED** from the prior `out2[2] -> cam+0x1C` attribution.
`func_800CA628` copies:

```text
cam+0x00..+0x08 = out1[0..2]
cam+0x18        = out2[0]
cam+0x1C        = out2[1]     # player-control camera angle
cam+0x20        = out2[2]
```

The last-iteration `swc1 $f6, 0x14($v1)` with `v1 = dst+0xC` writes
`dst+0x20`, not `+0x1C`. The `+0x1C` store is iteration 1 (`src2[1]`).

## Call alignment

Selected pipeline (camera `a0` / `*(FC38+0) == D_80127708[0]`) per consumed glidx,
mode 5, both windows:

```text
ORIGINAL_CAMERA_CALLS_PER_UPDATE:  FC38_sel=1  E1A58_sel=1  E1A58_all=1  CA628_sel=2
RECOMP_CAMERA_CALLS_PER_UPDATE:    FC38_sel=1  E1A58_sel=1  E1A58_all=1  CA628_sel=2
CAMERA_UPDATE_ORDINAL:             1
CAMERA_CALL_ALIGNMENT:             PASS
```

In-frame order (ORIGINAL guest RA; RECOMP LOOKUP RA is host-zero):

```text
func_8010FC38               RA 0x800A8ED0
  -> func_8010FE14/CA628    RA 0x8010FE24   # NEW camctrl already here
  -> func_800CA8B4          RA 0x8010FCC4   # copies that value
  -> func_800E1A58          RA 0x8010FCD0   # identity
  -> func_800CA628          RA 0x8010FCE0   # writes the same value again
```

`CAMERA_UPDATE_ORDINAL=1` is that single selected FC38 invocation.

## Early control (glidx 74..91)

```text
E1A58_EARLY_CONTROL:          PASS
  call counts match 1/1/1/2
  E1A58 is identity on both sides (input bits == output bits)
  |Δ out2[1]| ≤ 0.0007 deg, no growth
  matches prior <0.005 deg control
RAW_BITS_EARLY:               already differ at ULP scale (not treated as FAIL)
```

## E1A58 is identity

All 10 `D_8012CF90` slots have `type(+0x74)=1` at glidx 74, 384, and 385
in both runtimes. E1A58 therefore skips every constraint record.

```text
ORIG identity:    59/59 sampled invocations
RECOMP identity:  59/59 sampled invocations
COLLISION_INPUT_MATCH:     NOT_APPLICABLE  (no slot is live)
COLLISION_RESULT_MATCH:    NOT_APPLICABLE
```

## Entry-input decision

```text
E1A58_ENTRY_INPUTS_MATCH:     NO
FIRST_DIFFERENT_ENTRY_INPUT:  out2[1] (== cam+0x1C) and out1 (eye)
                              already at E1A58 entry
FIRST_DIFFERENT_ENTRY_INPUT_GLIDX:  consumed 384
                              (prior VI-aligned "glidx 385")
FIRST_DIFFERENT_ENTRY_INPUT_SOURCE: FE14/CA628 write that already
                              happened earlier in the same FC38
```

At consumed 384, `FC38_ENTRY` cam+0x1C still tracks (56.862354 vs 56.862328,
ULP). The first FE14/CA628 then stores:

```text
MEASURED_ORIGINAL out2[1] 53.650455  bits 42569a11  eye.x -4446.002930
MEASURED_RECOMP   out2[1] 53.582928  bits 425654eb  eye.x -4439.804688
delta             -0.067527 deg
```

Those bits are then copied by CA8B4, left unchanged by E1A58, and written
again by FC38's own CA628.

## Pipeline stages

```text
CA8B4_INPUT_MATCH:            NO   (first meaningful at consumed 384)
CA8B4_OUTPUT_MATCH:           NO   (pure copy of input)
E1A58_OUTPUT_MATCH:           NO   (identity of already-different input)
CA628_INPUT_MATCH:            NO   (first FE14 CA628 already differs)
FIRST_PIPELINE_DIVERGENCE_STAGE: FE14/CA628 during FC38, before CA8B4
```

Compact selected-chain table (consumed_glidx; `out2[1]` = camctrl). Prior
VI-aligned "385" is this table's 384:

```text
gl   stage            O_out2[1]    R_out2[1]    d_out2[1]
382  FE14/CA628       61.351696    61.351448    -0.0002
383  FE14/CA628       56.862354    56.862328    -0.0000
384  FC38_ENTRY +1C   56.862354    56.862328    -0.0000   # still ULP
384  FE14/CA628       53.650455    53.582928    -0.0675   # FIRST meaningful
384  CA8B4/E1A58/CA628  (same as FE14/CA628; E1A58 identity)
385  FE14/CA628       49.266788    49.272404    +0.0056
386  FE14/CA628       46.411045    47.094257    +0.6832
387  FE14/CA628       42.006962    43.684166    +1.6772
388  FE14/CA628       39.000668    41.404709    +2.4040
389  FE14/CA628       35.739124    39.069439    +3.3303
390  FE14/CA628       30.001722    35.373997    +5.3723
391  FE14/CA628       25.706940    31.456350    +5.7494
392  FE14/CA628       20.739645    26.719774    +5.9801
```

`out1` (eye) diverges in the same FE14/CA628 write (384: −4446.00 vs −4439.80).

## Persistent state

```text
EARLIEST_PERSISTENT_CAMERA_STATE_DIFF:  ULP-level cam+0x1C from glidx 74
EARLIEST_STATE_DIFF_GLIDX:              74 (numeric ULP only)
EARLIEST_STATE_WRITER:                  previous FE14/CA628
EARLIEST_MEANINGFUL_STATE_DIFF_GLIDX:   384 FE14/CA628
STATE_WRITER:                           func_800CA628 via func_8010FE14
                                        (RA 0x8010FE24) during func_8010FC38
```

The FE14 site inside `func_80110588` is `jal func_8010FE14` at `0x8011066C`.
**CORRECTED/SUPERSEDED:** mode `*(ctrl+0x16)` / handler PC are now **PROVEN**
as 2 / `0x80110588` on both runtimes
([[p08-demo-camera-mode-producer-differential]]). FE14 `a1/a2` are stack
copies of mode-object `+0x74/+0x80`, not those fields themselves.

## Instruction trace

```text
INSTRUCTION_TRACE_REQUIRED:     NO   (E1A58 is identity; inputs already differ)
FIRST_EXECUTION_PC_DIFFERENCE:  NOT_APPLICABLE
FIRST_REGISTER_VALUE_DIFFERENCE_PC: NOT_APPLICABLE
FIRST_BAD_PC:                   0x800CA65C  (FE14's CA628 store; writer SAME)
FIRST_BAD_MIPS_INSTRUCTION:     swc1 $f6, 0x14($v1)  (iteration 1 => cam+0x1C)
FIRST_BAD_ORIGINAL_VALUE:       out2[1] 53.650455 (42569a11)
FIRST_BAD_RECOMP_VALUE:         out2[1] 53.582928 (425654eb)
FPU_RAW_BITS_ORIGINAL:          42569a11
FPU_RAW_BITS_RECOMP:            425654eb
```

The FPU bits differ in the *input* to that store, not in the store itself.

## Player amplification (continuity only)

Prior writer-trace numbers, preserved. Camera-source column is this run's
FE14/CA628 `out2[1]` (consumed N = prior aligned N+1):

```text
gl(prior)  d_camctrl  d_ideal   d_yaw    d_pos
385        -0.0676    -0.0675   (grows)  0.05
387        +0.6833    +0.6832            0.07
389        +2.4040    +2.4040            0.25
392        +5.7495    +5.7494            3.00
400        +5.7770    +5.7800            21.39
```

`PLAYER_DRIFT_CHAIN_CONFIRMED` remains YES.

## Thresholds

```text
FIRST_NUMERIC_DIFFERENCE:              ULP cam+0x1C from glidx 74 (not causal)
FIRST_PROVEN_CAUSAL_DIFFERENCE:        consumed 384 FE14/CA628 src2[1]
                                       -0.0675 deg (prior aligned 385)
FIRST_GAMEPLAY_MEANINGFUL_DIFFERENCE:  same 384/385 value; then grows
```

## Classification

```text
PRIMARY_CLASS:                R  (func_800E1A58 REFUTED as origin)
FIRST_BAD_STAGE:              FE14/CA628 during func_8010FC38, before CA8B4
FIRST_BAD_FUNCTION:           CORRECTED/SUPERSEDED — see
                              [[p08-demo-camera-mode-producer-differential]]
                              (func_80110790 a1 already differs)
FIRST_BAD_GUEST_PC:           0x800CA65C (same writer; different src2[1])
ROOT_CAUSE_LEVEL:             R
ROOT_CAUSE:                   E1A58 is a no-op on this path. The first
                              meaningful camctrl is already in FE14/CA628
                              src2[1] during the FC38 mode update.
FIX_OWNER_CANDIDATE:          CORRECTED/SUPERSEDED — ncbapivot_entrypoint_3
                              look-at inputs (see
                              [[p08-demo-camera-overlay-angle-producer]])
FIX_IMPLEMENTED:              NO
```

## Artifacts (not committed)

```text
/tmp/p08-e1a58-original.csv
/tmp/p08-e1a58-recomp.csv
/tmp/p08-e1a58-original-run.log
/tmp/p08-e1a58-recomp-run.log
```

Temporary LOOKUP wrappers and the mupen PC hook lived outside the committed
tree (recomp wrappers reverted; mupen tracer remains in `/tmp/p08-m64p` only).
