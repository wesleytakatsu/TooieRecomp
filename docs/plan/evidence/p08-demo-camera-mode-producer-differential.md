---
title: "P08 — FC38 mode-update vector producer differential"
description: "Mode 2 / func_80110588 proven. First FE14 src2[1] is a stack copy of mode-object +0x80. Last writer is setter func_80110790 (via EE7F8). Its a1 already differs at consumed 384 (−0.0675°). Caller is a relocated nc-ba overlay (RA 0x801D3E7C)."
phase: P08
status: evidence
tags: [tooie-recomp, p08, measured-original, measured-recomp, camera, func_8010FC38, func_80110588, func_80110790, func_8010FE14]
---

# P08 — FC38 mode-update vector producer differential

## Scope

No production fix. `func_800E1A58` remains REFUTED and is not reopened.

```text
BASE_HEAD:                    01391c547413fa3360907af7a956b1f7e238a902
WINDOW:                       70..95 and 370..410
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

## FC38 mode contract (MIPS)

```text
CTRL_POINTER:                 a0 of func_8010FC38
MODE_FIELD_ADDRESS:           ctrl + 0x16
MODE_FIELD_WIDTH:             s16 (lh)
GATE:                         if lbu(ctrl+0x18)==0 AND lbu(ctrl+0x19)==0
                              then effective mode forced to 0
MODE_DISPATCH:
  eff==2 -> jal 0x80110588      a0 = *(ctrl+4)
  eff==3 -> jal 0x80089208      _ncfixposrot_entrypoint_6  a0=*(ctrl+8)
  eff==8 -> jal 0x80089530      _ncpod_entrypoint_14       a0=lh(ctrl+0x14)
  else   -> skip to CA8B4
```

## Live mode (PROVEN)

Every selected-camera FC38 in 70..95 and 370..405:

```text
ORIGINAL_MODE_SEQUENCE:       raw=2 f18=1 f19=0 eff=2   (62/62)
RECOMP_MODE_SEQUENCE:         raw=2 f18=1 f19=0 eff=2   (62/62)
MODE_MATCH:                   YES
MODE_AT_GLIDX_384:            ORIGINAL 2 / RECOMP 2
ACTIVE_MODE_HANDLER:          func_80110588
ORIGINAL_MODE_HANDLER_ENTRY_PC: 0x80110588
RECOMP_MODE_HANDLER_ENTRY_PC:   0x80110588
MODE_HANDLER_PC_MATCH:        YES
```

`func_80110588` is **PROVEN** (runtime handler PC), not inference.

## First FE14 call

```text
FIRST_FE14_CALL_SITE:         0x8011066C  (inside func_80110588)
FIRST_FE14_RA:                0x80110670
FIRST_FE14_A0:                pointer-to-cam  (ctrl / *(mode_obj+0x64))
FIRST_FE14_A1:                0x80045684  (stack; same guest addr both)
FIRST_FE14_A2:                0x80045678  (stack; same guest addr both)
FIRST_FE14_A1_SOURCE:         copy of mode_obj+0x74 via func_800EE7F8
FIRST_FE14_A2_SOURCE:         copy of mode_obj+0x80 via func_800EE7F8
```

The static candidate `a1 = mode_obj+0x74`, `a2 = mode_obj+0x80` is the
**persistent source**, not FE14's actual arguments. FE14 receives stack copies.

```text
MODE_OBJECT:                  *(ctrl+4)  (mode-2 follow object)
ORIGINAL obj @384:            0x801D4050
RECOMP obj @384:              0x801C6FD0
MODE_OBJECT_SEMANTIC_MATCH:   YES
```

## Early control

```text
MODE_HELPER_EARLY_CONTROL:    PASS
  SET80 a1[1] |Δ| ≤ 0.0007° at 74..91, no growth
  last SET74 eye.x |Δ| ≤ 0.0006, no growth
  one SET80 and matching SET74 cadence both sides
```

## First vector divergence (SET80 / SET74, before FE14)

`func_80110790(obj, src)` is `EE7F8(obj+0x80, src)` — a copier.
`func_80110770(obj, src)` is `EE7F8(obj+0x74, src)` — a copier.

At 80110790 **entry**, `a1` already holds the new angle:

```text
consumed 384 SET80 a1:
  MEASURED_ORIGINAL  (341.392639, 53.650032, 0)  bits 43aab242 425699a2 00000000
  MEASURED_RECOMP    (341.402466, 53.582504, 0)  bits 43aab384 4256547c 00000000
  d[1] = -0.067528 deg
```

Last SET74 (eye) at the same glidx, caller RA 0x80113028 = `func_80112E98`:

```text
MEASURED_ORIGINAL eye.x -4446.002930
MEASURED_RECOMP   eye.x -4439.804688
delta             +6.198
```

FE14 then sees a tiny shared post-tweak (both sides ~+0.00042° on src2[1]),
consistent with `func_80116024` / optional ncba1p on the stack copies.
That tweak is **not** the causal split.

```text
FIRST_SRC_NUMERIC_DIFFERENCE:       ULP SET74 eye from glidx 74
FIRST_SRC2_NUMERIC_DIFFERENCE:      ULP SET80 a1[1] from glidx 74
FIRST_SRC_MEANINGFUL_DIFFERENCE:    consumed 384 last SET74  (+6.198 eye.x)
FIRST_SRC2_MEANINGFUL_DIFFERENCE:   consumed 384 SET80 a1[1] (−0.0675°)
```

Compact SET80 a1[1] / last SET74 eye.x:

```text
gl   O_ang      R_ang      d_ang    O_eye.x     R_eye.x     d_eye
382  61.351273  61.351025  -0.0002  -4419.60    -4419.59    +0.01
383  56.861931  56.861904  -0.0000  -4435.28    -4435.27    +0.01
384  53.650032  53.582504  -0.0675  -4446.00    -4439.80    +6.20
385  49.266365  49.271980  +0.0056  -4456.83    -4451.85    +4.98
387  42.006538  43.683743  +1.6772  -4470.02    -4451.85   +18.17
392  20.739220  26.719349  +5.9801  -4517.87    -4475.09   +42.79
```

## Writer set (mode-object +0x74 / +0x80)

ORIGINAL store intercept (all widths except unimplemented SCD) on
`obj+0x74..+0x8B` in 380..392:

```text
SRC_WRITER_PCS_ORIGINAL:      {0x800EE7FC, 0x800EE804, 0x800EE810}
                              (func_800EE7F8, RA 0x80110780 = func_80110770)
SRC2_WRITER_PCS_ORIGINAL:     {0x800EE7FC, 0x800EE804, 0x800EE810}
                              (func_800EE7F8, RA 0x801107A0 = func_80110790)
SRC_WRITER_PCS_RECOMP:        func_80110770 / func_800EE7F8  (LOOKUP wrap)
SRC2_WRITER_PCS_RECOMP:       func_80110790 / func_800EE7F8  (LOOKUP wrap)
WRITER_SETS_MATCH:            YES
```

Stack 0x80045678/84 is reused by many later functions after FE14; those
stores are **not** the FE14 source producers.

## Write that creates the glidx-384 angle

```text
ORIGINAL_PRODUCER_PC:         0x800EE804  (EE7F8 store of a1[1] -> obj+0x84)
RECOMP_PRODUCER_PC:           same generated EE7F8 / func_80110790
PRODUCER_PC_MATCH:            YES
PRODUCER_FUNCTION:            func_80110790   # copier, not computer
PRODUCER_CALLER:              ncbapivot_entrypoint_3
                              (CORRECTED/SUPERSEDED — was UNKNOWN)
                              idx=781  RA offset 0x24C
                              ORIGINAL RA @384 this later run 0x801D3CDC
                              (prior 0x801D3E7C is the same offset on a
                              different heap placement)
                              See [[p08-demo-camera-overlay-angle-producer]]
```

`0x800EE804` / `0x800CA65C` are **FIRST_DIVERGENT_STORE_PC** only
(same instruction, different input). Not FIRST_BAD_PC.

```text
PRODUCER_ENTRY_INPUTS_MATCH:  NO
FIRST_DIFFERENT_PRODUCER_INPUT: func_80110790 a1[1]  (guest 0x80045638)
ORIGINAL_VALUE:               53.650032  bits 425699a2
RECOMP_VALUE:                 53.582504  bits 4256547c
SOURCE_FUNCTION:              overlay caller RA 0x801D3E7C
FIRST_DIFFERENT_INPUT_GLIDX:  384
```

## Eye vs angle

```text
EYE_AND_ANGLE_SHARED_PRODUCER: NO   (last writers differ)
  last eye setter caller:   func_80112E98  (RA 0x80113028) -> func_80110770
  last angle setter caller: overlay RA 0x801D3E7C -> func_80110790
```

Both run inside the same `func_80110588` invocation and first grow at 384,
so they likely share an **upstream** state. That state is not isolated here.

## Persistent / player / dt

```text
FIRST_PERSISTENT_CAUSAL_FIELD:  SET80 a1[1] (and last SET74 eye.x) at 384
FIRST_PERSISTENT_CAUSAL_GLIDX:  384
FIELD_WRITER:                   overlay -> func_80110790 (angle);
                                func_80112E98 -> func_80110770 (eye)
PLAYER_POSITION_INPUT_MATCH:    NOT_READ   (by 80110790; overlay unread here)
TARGET_POSITION_INPUT_MATCH:    NOT_READ
CAMERA_DT_MATCH:                NOT_READ
CAMERA_DT_SOURCE:               UNKNOWN
UPDATE_COUNT_MATCH:             UNKNOWN
```

## Instruction trace

```text
INSTRUCTION_TRACE_REQUIRED:     NO
FIRST_CONTROL_FLOW_DIFFERENCE:  NOT_APPLICABLE
FIRST_REGISTER_VALUE_DIFFERENCE_PC: NOT_APPLICABLE
FIRST_DIVERGENT_STORE_PC:       0x800EE804  (obj+0x84) and later 0x800CA65C
FIRST_BAD_PC:                   none proven
```

## Classification

```text
PRIMARY_CLASS:                B
FIRST_BAD_STAGE:              func_80110790 entry a1 (overlay-computed stack)
FIRST_BAD_FUNCTION:           ncbapivot_entrypoint_3
                              (CORRECTED/SUPERSEDED from unnamed overlay)
ROOT_CAUSE_LEVEL:             B
ROOT_CAUSE:                   Mode 2 / func_80110588 proven. FE14/CA628 only
                              copies stack images of mode_obj+0x74/+0x80.
                              Those fields are filled by setters 80110770/80110790.
                              At 384 the setter inputs already differ
                              (−0.0675° and +6.2 eye.x). The computer is
                              ncbapivot_entrypoint_3 → func_800F18FC /
                              func_80013B70; look-at inputs already differ.
                              See [[p08-demo-camera-overlay-angle-producer]].
FIX_OWNER_CANDIDATE:          func_80112E98 (eye) and/or *(obj+0x30)+4
FIX_IMPLEMENTED:              NO
```

## Artifacts (not committed)

```text
/tmp/p08-mode-original.csv
/tmp/p08-mode-recomp.csv
```

Temporary LOOKUP hooks and the mupen PC/store tracer lived outside the
committed tree and were reverted before this checkpoint.
