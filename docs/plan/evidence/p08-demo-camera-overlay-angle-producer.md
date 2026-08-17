---
title: "P08 — relocated overlay angle producer (ncbapivot)"
description: "Live overlay of func_80110790 at consumed 384 is ncbapivot (781) / ncbapivot_entrypoint_3. All moving 0x801D RAs share section offset 0x24C. dest[1] is func_800F18FC/func_80013B70 of (eye − target). Inputs already differ at 384."
phase: P08
status: evidence
tags: [tooie-recomp, p08, measured-original, measured-recomp, camera, ncbapivot, func_80110790, func_800F18FC, func_80013B70]
---

# P08 — relocated overlay angle producer

## Scope

No production fix. Do not reopen first-straight speed, stick/remap/13B70 as
a stick bug, yaw_setIdeal, camera selector, `func_800CA628`, `func_800E1A58`,
FC38 mode selection, or `func_80110588` identity.

```text
BASE_HEAD:                    f13e334609c5d29a4736f8f18eb1a2c44bd371bb
WINDOW:                       70..95 and 370..410 (detail 382..392)
MODE:                         mode=5 Wooded Hollow demo
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
```

## Live overlay (PROVEN)

Guest load list `D_80126738[0..D_80117C60)` + package `+0x2C` (index) /
`+0x34` (relocated `text_start`) / header name, taken at `func_80110790`
entry. Cross-checked against `D_801247A0[obj+0x6C]+4`.

```text
LIVE_OVERLAY_ID:              781
LIVE_OVERLAY_SECTION:         text (package name "ncbapivot")
OVERLAY_CANONICAL_NAME:       ncbapivot
LIVE_OVERLAY_RUNTIME_BASE:    text_start  (moves on reload)
  this-run @384:              0x801D3A90  size 0x290  pkg 0x801D3A30
LIVE_OVERLAY_TEXT_RANGE:      text_start .. text_start+0x290
RELOCATION_DELTA:             text_start - 0x80800000
CALLER_RA_RUNTIME @384:       0x801D3CDC   (this run)
                              prior evidence 0x801D3E7C is the same
                              logical site on a different heap placement
CALLER_CALL_PC_RUNTIME:       RA - 8 = text_start + 0x244
CALLER_SECTION_RELATIVE_OFFSET: 0x24C   (RA) / 0x244 (jal)
```

`obj+0x6C == 19` on every selected SET80 in 70..95 and 370..410.
`D_801247A0[19]+4 == 0x80089488` both runtimes.

```text
0x80089488 = _ncbapivot_entrypoint_3
  syscall 781, addi $t0, $zero, 0xC  → entrypoint 3
```

That stub is a resident core-window trampoline, not the relocated body.

## Same logical callsite across reloads (PROVEN)

Every selected SET80 in the windows (67/67 original):

```text
name=ncbapivot  idx=781  rel=0x24C  type6c=19  jalr=0x80089488
```

This-run SET80 RA vs text_start:

```text
gl   RA         text_start  RA-text
70   801D541C   801D51D0    0x24C
78   801D531C   801D50D0    0x24C
384  801D3CDC   801D3A90    0x24C
389  801D2C7C   801D2A30    0x24C
```

Prior-run RAs `0x801D541C / 0x801D531C / 0x801D3E7C / 0x801D38BC` are the
same offset `0x24C` from whatever `text_start` that load used.

```text
SAME_LOGICAL_CALLSITE_ACROSS_RELOADS:  YES
CANONICAL_SECTION_RELATIVE_CALL_OFFSET: 0x244
```

## Recomp map (PROVEN)

```text
OVERLAY_SECTION_NAME:         ncbapivot text
CALLER_FUNCTION:              ncbapivot_entrypoint_3
CALLER_ENTRY_SECTION_OFFSET:  0x98   (0x80800098)
CALL_TO_80110790_SECTION_OFFSET: 0x244  (0x80800244)
RECOMP_FUNCTION:              ncbapivot_entrypoint_3
RECOMP_CALLSITE_GUEST_ID:     0x80800244
RESIDENT_STUB:                _ncbapivot_entrypoint_3 @ 0x80089488
CALLSITE_MATCH:               YES
OVERLAY_CALL_TARGETS_MATCH:   YES
```

Generated overlay JALs do not write guest `$ra`; Recomp SET80 `r31` is
therefore 0. Identity is from overlay index + section offset + stub PC,
not from a relocated RA.

## Caller contract

```text
CALLER_ENTRY_PC_OR_OFFSET:    0x80800098 / ncbapivot_entrypoint_3
CALLER_ARGUMENT_CONTRACT:     a0 = mode_obj  (FC38 mode-2 object)
SET80_VECTOR_STACK_OFFSET:    sp+0x60   (guest 0x80045638 this path)
TARGET_VECTOR_STACK_OFFSET:   sp+0x6C   (guest 0x80045644)
```

`ncbapivot_entrypoint_3` (abbreviated):

1. `func_80800008_ncbapivot(obj, sp+0x6C)` — C body is
   `func_80112524(obj, dest)` = `EE7F8(dest, *(obj+0x30)+4)`.
2. Local follow/collision helpers (`801107F0`, `800EFB24`, `80115358`, …).
3. `func_80110A68(obj, sp+0x60, sp+0x6C)`.
4. `func_80110790(obj, sp+0x60)` — copy dest → `obj+0x80`.

`func_80110A68` is a 5-instruction wrapper:

```text
func_80110A68(obj, dest, tgt):
    func_800A516C(dest, tgt, obj+0x74)
```

`func_800A516C`:

```text
func_800F18FC(tgt, eye=obj+0x74, dest)
dest[0] = wrap_degrees(-dest[0])   # func_800136E4
dest[2] = 0
# dest[1] left as func_800F18FC wrote it
```

## Angle formula (PROVEN)

`func_800F18FC(tgt, eye, dest)`:

```text
dx = eye.x - tgt.x
dy = eye.y - tgt.y
dz = eye.z - tgt.z
horiz = sqrtf(dx*dx + dz*dz)
dest[1] = func_80013B70(dx, dz)          # critical field
dest[0] = func_80013B7C(dy, horiz)
```

```text
ANGLE_COMPUTATION_FORMULA:
  dest[1] = func_80013B70(eye.x - tgt.x, eye.z - tgt.z)
  eye     = mode_obj + 0x74   (after last SET74 in this 80110588)
  tgt     = *(mode_obj + 0x30) + 4   via 80112524
```

`func_80013B70` is the already-accepted stick-angle helper. It is **not**
reopened as a stick/13B70 defect. Here it is a look-at atan2. Do not call
it `FIRST_BAD_PC` unless a later run shows matching `dx,dz` and a different
result.

## Temp-vector writers

Original stores overlapping `0x80045638..+0x0B` **before** SET80 at 384
include the computing writes:

```text
TEMP_VECTOR_WRITER_PCS_ORIGINAL:
  0x800F1960  swc1 dest[1]   func_800F18FC   (= 13B70 result)
  0x800F1974  swc1 dest[0]   func_800F18FC
  0x800A51A4  swc1 dest[0]   wrap_degrees(-dest[0])
  0x800A51A8  swc1 dest[2]   0
TEMP_VECTOR_WRITER_PCS_RECOMP:
  func_800F18FC / func_800A516C / func_80110A68  (LOOKUP wrap)
WRITER_SET_MATCH:             YES
```

Later stores to the same stack after FE14 are unrelated reuse.

```text
ORIGINAL_LAST_ANGLE_WRITE_PC: 0x800F1960
RECOMP_LAST_ANGLE_WRITE_PC:   generated func_800F18FC dest[1] store
ANGLE_WRITE_PC_MATCH:         YES
SOURCE_REGISTER:              $f0 of func_80013B70
ORIGINAL_VALUE @384:          53.650032  bits 425699a2
RECOMP_VALUE @384:            53.582504  bits 4256547c
```

`func_80110790` / `0x800EE804` remain **FIRST_DIVERGENT_STORE_PC** only
(same copier, different dest[1]).

## Compact SET80 dest[1]

```text
gl   O_ang      R_ang      d_ang
74   87.180740  87.181007  +0.000267
91  100.717316 100.717865  +0.000549
382  61.351273  61.351025  -0.000248
383  56.861931  56.861904  -0.000027
384  53.650032  53.582504  -0.067528
385  49.266365  49.271980  +0.005615
392  20.739220  26.719349  +5.980129
```

```text
OVERLAY_PRODUCER_CONTROL:     PASS
  same overlay / same 0x24C site in 74..91
  |Δ dest[1]| ≤ 0.00074°, no growth
```

## Input decision at 384

```text
ANGLE_COMPUTATION_INPUTS_MATCH_AT_384:  NO
EYE_INPUT_MATCH:                        NO   (grows at 384; eye.x +6.198)
TARGET_INPUT_MATCH:                     NO   (stable ~0.011/0.054 background)
SHARED_384_CAUSAL_DEPENDENCY:           NO
FIRST_DIFFERENT_INPUT_COMPONENT:        eye (mode_obj+0x74)
FIRST_UPSTREAM_WRITER:                  func_80112E98
INSTRUCTION_TRACE_REQUIRED:             NO
See [[p08-demo-camera-13b70-inputs]].
```

`func_800F18FC` reads **both** look-at endpoints:

```text
eye  = mode_obj+0x74
  last SET74 this frame, caller RA 0x80113028 = func_80112E98
  MEASURED_ORIGINAL  (-4446.002930, -206.578674, 1266.833740)
                     bits c58af006 c34e9424 449e5aae
  MEASURED_RECOMP    eye.x -4439.804688   (prior SET74 wrap; this run
                     did not re-hook SET74)
  FIRST_SRC_MEANINGFUL_DIFFERENCE already at 384 (+6.198 eye.x)

tgt  = *(mode_obj+0x30)+4   via func_80112524
  MEASURED_RECOMP A068 a2:
    (-4858.596680, -379.057587, 963.245972)
    bits c597d4c6 c3bd875f 4470cfbe
  MEASURED_ORIGINAL dest 0x80045644 was not in the store-watch set.
  A same-frame later 80112524 (RA 0x80110F5C, dest 0x800455CC)
  copies the same *(obj+0x30)+4:
    (-4823.640137, -370.067139, 940.386963)
  INFERENCE: original A068 tgt is that player/target record.
```

```text
FIRST_DIFFERENT_ANGLE_INPUT:      look-at pair
                                  eye (obj+0x74) PROVEN different
                                  tgt (*(obj+0x30)+4) MEASURED_RECOMP
                                  vs same-frame 12524 INFERENCE
FIRST_DIFFERENT_ANGLE_INPUT_GLIDX: 384
FIRST_DIFFERENT_ANGLE_INPUT_SOURCE: func_80112E98 (eye SET74);
                                    func_80112524 / player-or-target
                                    record at *(obj+0x30)+4
FIRST_PERSISTENT_ANGLE_STATE_DIFF:  dest[1] and its look-at inputs
                                    first grow at consumed 384
FIRST_PERSISTENT_DIFF_GLIDX:        384
FIELD_WRITER:                       0x800F1960 writes dest[1];
                                    inputs from SET74 + 80112524
```

```text
PLAYER_POSITION_INPUT_MATCH:  NO if *(obj+0x30)+4 is the player
                              record (recomp vs later original 12524)
TARGET_POSITION_INPUT_MATCH:  NO   (same pointer)
CAMERA_DT_MATCH:              NOT_READ
CAMERA_DT_SOURCE:             NOT_READ
UPDATE_COUNT_MATCH:           UNKNOWN
```

Do not instruction-trace `func_80013B70` / `func_800F18FC` until a run
shows bitwise-matching `dx,dz`.

## Eye corroboration

```text
ANGLE_AND_EYE_SHARE_EARLIER_STATE:  PROVEN
```

The angle formula **reads** `mode_obj+0x74`. The last SET74 and the
SET80 computer run in the same `func_80110588` / `ncbapivot` update
and first grow at 384. The eye path is not walked independently here.

## Terminology

```text
FIRST_DIVERGENT_STORE_PC:     0x800F1960  (dest[1] from already-different
                              dx,dz) then 0x800EE804 / 0x800CA65C
FIRST_BAD_PC:                 none proven
FIRST_BAD_MIPS_INSTRUCTION:   none
FIRST_REGISTER_VALUE_DIFFERENCE_PC: NOT_APPLICABLE
```

## Classification

```text
PRIMARY_CLASS:                B
FIRST_BAD_STAGE:              ncbapivot look-at inputs at 384
FIRST_BAD_FUNCTION:           ncbapivot_entrypoint_3
                              (computer: func_800F18FC / func_80013B70;
                               inputs already differ)
ROOT_CAUSE_LEVEL:             B
ROOT_CAUSE:                   Relocated SET80 caller is ncbapivot 781
                              / ncbapivot_entrypoint_3 (RA offset 0x24C).
                              dest[1] = 13B70(eye.x-tgt.x, eye.z-tgt.z).
                              At 384 eye and the *(obj+0x30)+4 target
                              already differ. 13B70 is a copier of those
                              deltas, not a proven bad instruction.
FIX_OWNER_CANDIDATE:          writer of obj+0x74 (func_80112E98) and/or
                              *(obj+0x30)+4 (player/target record)
FIX_IMPLEMENTED:              NO
```

## Artifacts (not committed)

```text
/tmp/p08-ovl-original.csv
/tmp/p08-ovl-recomp.csv
```

Temporary TooieRecomp LOOKUP wraps were reverted before checkpoint.
Mupen `/tmp/p08-m64p` Pure Interpreter retains env-gated tracers
(`TOOIE_MODE_TRACE=1`); not part of this tree.
