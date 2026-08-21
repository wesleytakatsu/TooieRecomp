---
title: "P08 — USER_DEMO_4 follow-camera first discriminator (camctrl glidx 41)"
description: "ANGRYLION_CONTROL vs RT64_CONTROL proof that func_800CA628 writes selected+0x1C on USER_DEMO_4, stores src2[1], and already receives a non-bit-identical eye/src2 from func_800CA8B4 at glidx 36."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, follow-camera, camctrl, func_800CA628, func_800CA8B4, ncbapivot, angrylion, rt64]
---

# P08 — USER_DEMO_4 follow-camera first discriminator

## 1. Verdict

```text
CAMCTRL_WRITER_PC:              0x800CA65C
CAMCTRL_WRITER_FUNCTION:        func_800CA628
CAMCTRL_WRITER_SAME_BOTH:       YES
CA628_CONFIRMED:                YES
CA628_SEMANTIC_JOIN:            PASS
CAMCTRL_SOURCE_WORD:            src2[1]  (src2[2] is 0.0 this clip; historical "src2[2]" is CORRECTED/SUPERSEDED for 0-based indexing)

LAST_MATCHING_CAMCTRL_GLIDX:    38 at printed 0.001°  (303.985); bits already 1-2 ULP off from glidx 36
FIRST_DIFFERENT_CAMCTRL_GLIDX:  41 at prior demo-sync 0.085° == this CA628 dump glidx 40

FIRST_CA628_DIFFERENT_INPUT:    src2[1] and eye src[0]/src[2] at first window sample
FIRST_CA628_DIFFERENT_INPUT_GLIDX: 36  (window sample; NOT first in clip)
FIRST_CA628_DIFFERENT_INPUT_PRODUCER: func_800CA8B4
TRUE_FIRST_CA8B4_INPUT_BIT_DIFFERENCE_GLIDX: 1
  (see [[p08-user-demo4-ca8b4-first-seed]])
func_800E1A58:                  identity (enter==leave) both oracles in this window; not the discriminator

PLAYER_POSITION_BEFORE_CAM_SPLIT_MATCH: YES (spawn -84,0,-1855 through dump glidx 38)
CAMERA_INTERNAL_STATE_BEFORE_SPLIT_MATCH: NO (eye Δ 0.00065 already at glidx 36)
CAMERA_TARGET_BEFORE_SPLIT_MATCH: YES if target = player; NO if target = follow-cam eye

NCBAPIVOT_FIRST_DIFFERENT_READ: CA8B4-produced eye / src2 (not ncbapivot +0x5C; *+0x5C = 0 both)
NCBAPIVOT_FIRST_DIFFERENT_READ_OFFSET: selected camera record +0x00..+0x08 and +0x1C
NCBAPIVOT_FIRST_DIFFERENT_READ_PRODUCER: func_800CA8B4 reading/updating that record

CAMERA_UPDATE_COUNT_MATCH:      YES (ncbapivot 1, fc38 1, fe14 1, ca628 2 per glidx 36..45)
CAMERA_UPDATE_ORDER_MATCH:      YES (fc38 → ncbapivot → fe14 → CA628 → CA8B4 → E1A58 → CA628)

FIRST_PROVEN_CAUSAL_CAMERA_DIFFERENCE: CA628 inputs already differ at glidx 36; CA628 stores them faithfully
ROOT_CLASS:                     C-FIRST-DISCRIMINATOR
A_CANDIDATE:                    NO
FIX_IMPLEMENTED:                NO
EGG_PARITY_DEFECT:              PROVEN_BUT_DEFERRED
```

The printed 0.085° split at demo-sync glidx 41 is **FEEDBACK_ACCUMULATION** of an earlier, already-present follow-camera eye/camctrl ULP error, amplified when walking starts at dump glidx 38-39. CA628 is not at fault.

Do not reopen Rice Z-buffer / `func_800E1A58` projection as the USER_DEMO_4 glidx-41 cause: E1A58 does not modify the vectors here.

---

## 2. Environment

```text
BASE_HEAD:     083b1b387ebbdc791924c0a859ada2cb5d76ed4b
RT64:          TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1 timeout 180s ./build/TooieRecompiled
               EXIT 0; USER_DEMO_4 reached; window glidx 36..50 logged
ANGRYLION:     mupen --emumode 0 + Angrylion RDP Plus + CXD4 LLE (pinned oracle)
               hook: tools/p08_attract_egg_hook.h (local mupen copy)
ROM_SHA1:      af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
CPU_MODE:      PURE_INTERPRETER
Angrylion:     ORACLE ONLY; TooieRecomp remains RT64
```

Lookup wrappers in `boot_hw_stubs.cpp` are env-gated (`TOOIE_DIAG_ATTRACT`) and only log for `mode=5 sub=4 glidx 36..50`. Generated C was not edited.

---

## 3. Writer proof (this clip)

`func_800CA628(dst, src, src2)` copies `src[0..2]` to `dst[0..2]` (eye) and writes `src2[i]` through `swc1 $f6, 0x14($v1)` at `0x800CA65C`. Iteration 2 stores to `dst+0x1C`.

Angrylion store intercept at `0x800CA65C`:

```text
v1=dst+8  store=dst+0x1C   == selected+0x1C
```

RT64 leave: `stored_1c` bits == `src2_1_bits` every sample.

Two invocations per consumed demo tick, both oracles:

1. `func_8010FE14` (AN RA `0x8010FE24`)
2. `func_8010FC38` (AN RA `0x8010FCE0`)

Same values both calls. RT64 `r31` is 0 at LOOKUP trampoline entry; join uses glidx + ordinal.

```text
CAMCTRL_WRITER_PC:        0x800CA65C
CAMCTRL_WRITER_FUNCTION:  func_800CA628
CAMCTRL_WRITER_SAME_BOTH: YES
CA628_SEMANTIC_JOIN:      PASS
```

---

## 4. CA628 inputs glidx 36..41 (first call)

| glidx | RT64 src2[1] | AN src2[1] | Δdeg | eye Δ | player XZ |
|---:|---:|---:|---:|---:|---|
| 36 | 304.079285 `43980a26` | 304.079224 `43980a24` | 0.000061 | 0.00065 | spawn MATCH |
| 37 | 303.984985 `4397fe14` | 303.984955 `4397fe13` | 0.000030 | 0.00065 | spawn MATCH |
| 38 | 304.104126 `43980d54` | 304.104065 `43980d52` | 0.000061 | 0.00065 | spawn MATCH |
| 39 | 304.923553 `43987637` | 304.926361 `43987693` | 0.002808 | 0.24 | walk start |
| 40 | 306.041656 `43990555` | 306.126892 `4399103e` | **0.085236** | 2.01 | |
| 41 | 307.851868 `4399ed0a` | 308.576752 `439a49d3` | 0.725 | 11.26 | |

Dump glidx 40 is the prior demo-sync glidx 41 write (CA628 runs on the same post as the in-tick camera update; demo-sync samples after consume).

```text
FIRST_CA628_DIFFERENT_INPUT:        src2[1] (and eye src[0]/src[2])
FIRST_CA628_DIFFERENT_INPUT_GLIDX:  36
CLASS_AT_36:                        INPUTS_DIFFERENT (not bit-identical; numerically close)
CLASS_AT_40:                        INPUTS_DIFFERENT (0.085°)
OUTPUT vs INPUT:                    CA628 output bit-identical to src2[1] both oracles
```

`func_800E1A58` enter == leave on both oracles in this window.

`func_800CA8B4(dst_record, out_eye, out_src2)` leave vectors equal the following CA628 inputs. Producer of the differing CA628 input is **CA8B4**, reading the selected camera record `a0`.

Idle follow slew (glidx 1-35) was **not** dumped in this CA628 session. The later CA8B4 seed trace **PROVES** the ULP already exists at glidx 1 (`ncbapivot_entrypoint_3` first tick). Canonical: [[p08-user-demo4-ca8b4-first-seed]]. `glidx 36` is **not** the root.

---

## 5. Feedback vs player

```text
PLAYER_POSITION_BEFORE_CAM_SPLIT_MATCH: YES through dump glidx 38
CAMERA_INTERNAL_STATE_BEFORE_SPLIT_MATCH: NO  (eye already 0.00065 off at 36)
CAMERA_TARGET_BEFORE_SPLIT_MATCH:       player pos YES; follow eye NO
```

A player-position difference after glidx 47 cannot explain dump-glidx-36 eye/camctrl ULP. Walking from 39 **amplifies** the existing camera-state error (FEEDBACK_ACCUMULATION).

---

## 6. ncbapivot / timing

```text
FOLLOW_CAMERA_MODE:            ncbapivot overlay 781
FOLLOW_CAMERA_UPDATE:          ncbapivot_entrypoint_3 (stub 0x80089488; AN RA 0x8011061C)
ncbapivot +0x5C dereference:   0 both oracles (branch taken; overlay 0x80089468 skipped)
CAMERA_UPDATE_COUNT_MATCH:     YES
CAMERA_UPDATE_ORDER_MATCH:     YES
ROOT_CLASS timing:             not B-FOLLOW-CAMERA-TIMING
```

---

## 7. Classification

```text
ROOT_CLASS:   C-FIRST-DISCRIMINATOR
EVIDENCE:     CA628 is a faithful copier of already-divergent CA8B4 outputs at the first
              sample of the requested window (glidx 36). The visible 0.085° demo-sync
              split at 41 is accumulation after walk start.
A_CANDIDATE:  NO
```

Egg FIRE-vs-BLUE remains **PROVEN_BUT_DEFERRED**. Fire is after this camera split.

---

## 8. Next single action

CA8B4 first-seed trace is **DONE**: [[p08-user-demo4-ca8b4-first-seed]]. Bit-level first difference is glidx 1 (`ncbapivot` output). No production fix. Do not open P09. Do not recommend Sol.

```text
NEXT_SINGLE_ACTION: capture complete ncbapivot_entrypoint_3 inputs/FPU on USER_DEMO_4 glidx 1; no fix
```
