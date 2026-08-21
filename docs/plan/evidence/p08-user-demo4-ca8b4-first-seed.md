---
title: "P08 — USER_DEMO_4 CA8B4 first seed / recurrence discriminator"
description: "ANGRYLION_CONTROL vs RT64_CONTROL bit-level CA8B4 join on USER_DEMO_4 glidx 1..42. CA8B4 is an identity copier. Initialization matches. First ncbapivot tick produces the ULP seed; walk start amplifies it without a new CA8B4 external input."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, follow-camera, func_800CA8B4, ncbapivot, camctrl, recurrence, angrylion, rt64]
---

# P08 — USER_DEMO_4 CA8B4 first seed / recurrence discriminator

## 1. Verdict

```text
CA8B4_SEMANTIC_JOIN:                    PASS
CA8B4_EXECUTED_READ_SET:                record[+0x00,+0x04,+0x08] eye;
                                        record[+0x18,+0x1C,+0x20] src2
                                        (i=1 at +0x1C is camctrl)
CAMERA_RECORD_INITIALIZATION_MATCH:     YES (bit-identical)
FIRST_INITIAL_CAMERA_FIELD_DIFFERENCE:  NONE
INITIAL_CAMERA_FIELD_WRITER:            pre-ncbapivot default
                                        eye=45226124,453b8006,458c9fee
                                        src2=c1f00000,41f00000,00000000
                                        (-30, +30, 0); both oracles

FIRST_CA8B4_INPUT_BIT_DIFFERENCE_GLIDX: 1
FIRST_CA8B4_INPUT_BIT_DIFFERENCE:       eye[0] 7 ULP, eye[2] 4 ULP,
                                        src2[1]/camctrl 2 ULP
FIRST_CA8B4_OUTPUT_BIT_DIFFERENCE_GLIDX: 1 (identity copy of those inputs)
FIRST_CAMERA_RECORD_BIT_DIFFERENCE_GLIDX: 1 (after first ncbapivot)

CA8B4_BRANCH_PATH_MATCH:                YES (3-iteration copy loop only)
CA8B4_HAS_RECURRENT_CAMERA_STATE:       YES
RECURRENT_FIELDS:                       record+0x00,+0x04,+0x08,+0x18,+0x1C,+0x20
EXTERNAL_INPUT_FIELDS:                  NONE on the executed CA8B4 path

FIRST_BIT_LEVEL_DIFFERENCE:             glidx 1 CA8B4 inputs =
                                        first ncbapivot output
FIRST_VISIBLE_MEANINGFUL_DIFFERENCE:    demo-sync glidx 41 / dump glidx 40
                                        camctrl 0.085° (previous);
                                        last 0.001° display match remains glidx 38
FIRST_GAMEPLAY_CAUSAL_DIFFERENCE:       walk-start amplification of the
                                        glidx-1 seed (player pos still
                                        bit-identical at glidx 39..40)

IDLE_ULP_IS_RECURRENT_SEED:             PROVEN (ticks 2..37)
WALK_START_NEW_DISCRIMINATOR:           NO
WALK_START_FIRST_NEW_EXTERNAL_INPUT_DIFFERENCE: NONE (CA8B4 reads only
                                        the camera record; player pos
                                        MATCH through glidx 40)

FIRST_DIFFERING_INPUT_PRODUCER:         node+0x74 already differs at first
                                        ncbapivot ep3 enter
                                        ([[p08-user-demo4-ncbapivot-first-instruction]])
A_CANDIDATE:                            NO (CA8B4)
ROOT_CLASS:                             B-CAMERA-RECURRENT-SEED for CA8B4;
                                        ncbapivot tick itself is
                                        B-NCBAPIVOT-INPUT
FIX_IMPLEMENTED:                        NO
EGG_PARITY_DEFECT:                      PROVEN_BUT_DEFERRED
```

`glidx 36` is **not** the root. It is the previously captured sample of a seed that already exists at the first follow-camera CA8B4 (`glidx 1`).

Do not confuse **last matching at 0.001° display precision** (glidx 38 camctrl print) with **last bit-identical sample** (never: bits already differ at glidx 1).

---

## 2. Environment

```text
BASE_HEAD:     f4a8aa0972dd42c0d253286f8e998cc841dd415b
RT64:          TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1
               timeout 220s ./build/TooieRecompiled
               EXIT 137 (timeout after USER_DEMO_4 glidx 42 captured)
ANGRYLION:     mupen64plus --emumode 0
               Video: angrylion's RDP Plus v1.6.0
               RSP:   Static Interpreter (cxd4) v0.1.1
               CPU:   Pure Interpreter
               Angrylion: ORACLE ONLY; TooieRecomp remains RT64
ROM_SHA1:      af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
```

Run 1: full CA8B4 join glidx 1..42 (375 enter/leave both oracles).
Run 2: ncbapivot-stub record dump at first tick (init bits).

Generated C was not edited. Wrappers remain env-gated (`TOOIE_DIAG_ATTRACT`).

---

## 3. CA8B4 static / executed read set

`func_800CA8B4(a0=record, a1=out_eye, a2=out_src2)` is a 3-iteration load/store leaf:

```text
CA8B4_EXECUTED_READ_SET:
  PC 800CA8C8  lwc1 $f4, 0x0($a3)    base=a0  offset=0+i*4     role=eye[i]
  PC 800CA8D8  lwc1 $f6, 0x18($a3)   base=a0  offset=0x18+i*4  role=src2[i]
                                         i=1 → record+0x1C = camctrl
  i = 0,1,2. No other memory reads. Incoming a3 / FPRs are not inputs.
  Stores: a1[i] = eye[i]; a2[i] = src2[i].
```

CA628 then writes `src[0..2]` to `dst+0x00..+0x08` and `src2[i]` to `dst+0x18..+0x20` (`swc1 $f6, 0x14($v1)` with `v1` stepping). Together they are an identity round-trip of those six words onto the destination record.

Runtime identity: **0** CA8B4 enter/leave mismatches on either oracle (375/375).

```text
CA8B4_HAS_RECURRENT_CAMERA_STATE: YES
RECURRENT_FIELDS:   record+0x00,+0x04,+0x08,+0x18,+0x1C,+0x20
EXTERNAL_INPUT_FIELDS: NONE
```

Conceptual recurrence of the **follow-camera system** (CA8B4 is only the snapshot):

```text
record[n+1] = ncbapivot_entrypoint_3(record[n], player[n], …)
CA8B4(record[n+1]) → (eye, src2)   # identity
CA628(record, eye, src2)           # identity writeback
```

---

## 4. Semantic join

```text
CA8B4_SEMANTIC_JOIN: PASS
  key = glidx + selected follow-cam record pointer + ordinal among that rec
  dummy CA8B4 (eye=0,0,300 / a0≈0x80191xxx) excluded
  first-per-glidx follow-cam: 42/42 both oracles (glidx 1..42)
  glidx 0: no CA8B4 / no ncbapivot (first stable mode-5 tick is glidx 1)
  RT64 r31=0 at LOOKUP trampoline; AN follow-cam CA8B4 RA=0x8010FCC4
  Record pointer relocates (defrag): 80348cb0 → 80348b90 → 80344f20 → 80344e40
  Join uses the selected rec of that tick, not a fixed address.
```

Order on both oracles at glidx 1:

```text
fc38 → ncbapivot stub/ep3 → fe14 → CA628 → CA8B4 → E1A58 → CA628
```

`CA8B4_BRANCH_PATH_MATCH: YES` — the copy loop has no data-dependent branch. Path remains identical through glidx 42. `FIRST_BRANCH_DIFFERENCE: NONE`.

---

## 5. Initialization

Selected follow-cam record **immediately before first ncbapivot** (glidx 1, `first=1`):

| field | RT64 | ANGRYLION |
|---|---|---|
| eye bits | `45226124,453b8006,458c9fee` | `45226124,453b8006,458c9fee` |
| src2 bits | `c1f00000,41f00000,00000000` | `c1f00000,41f00000,00000000` |
| float | (2598.071289, 3000.001465, 4499.991211) | identical |
| src2 float | (−30, +30, 0) | identical |
| player pos | `c2a80000,00000000,c4e7e000` | identical |
| yaw/ideal | `42d80000` / `42d80000` | identical |

```text
CAMERA_RECORD_INITIALIZATION_MATCH: YES
FIRST_INITIAL_CAMERA_FIELD_DIFFERENCE: NONE
```

Warp-in still uses `ncpod`/`ncpoducam`. This default pose is the follow-cam record **before** the first `ncbapivot` tick, not a scripted-camera mismatch.

---

## 6. First bit difference

First follow-cam CA8B4 at **glidx 1** (player still spawn-identical):

| | RT64 | ANGRYLION | Δ |
|---|---|---|---|
| eye[0] | `c43efa34` −763.90942383 | `c43efa3b` −763.90985107 | 7 ULP |
| eye[1] | `43c49666` 393.174988 | `43c49666` 393.174988 | 0 |
| eye[2] | `c4a6b794` −1333.73681641 | `c4a6b798` −1333.73730469 | 4 ULP |
| src2[0] | `43a9628d` 338.769928 | `43a9628d` 338.769928 | 0 |
| src2[1] | `4399bb16` 307.46160889 | `4399bb14` 307.46154785 | 2 ULP |
| src2[2] | `00000000` | `00000000` | 0 |

Outputs equal inputs on each runtime. `A_CANDIDATE` for CA8B4: **NO**.

```text
FIRST_CA8B4_INPUT_BIT_DIFFERENCE_GLIDX:  1
FIRST_CA8B4_OUTPUT_BIT_DIFFERENCE_GLIDX: 1
FIRST_CAMERA_RECORD_BIT_DIFFERENCE_GLIDX: 1
FIRST_DIFFERING_INPUT_PRODUCER: ncbapivot_entrypoint_3 first tick
  (overlay 781 ep 3; stub 0x80089488; AN RA 0x8011061C)
```

CA628 at the same tick already carries those bits (`src2_1_bits=4399bb16` vs `4399bb14`) because ncbapivot ran first. CA628 remains a faithful copier; do not reopen it.

FPU bit-level capture inside CA8B4 is **not applicable**: complete CA8B4 inputs are not identical.

---

## 7. Idle recurrence (glidx 2..37)

Mandatory per-tick pattern after the first ULP:

```text
PRIOR_STATE_MATCH:          NO   (follow-cam record already differs)
NEW_EXTERNAL_INPUTS_MATCH:  YES  (CA8B4 has no new externals;
                                  player pos/yaw/ideal bit-identical)
OUTPUT_MATCH:               NO
IDLE_ULP_IS_RECURRENT_SEED: PROVEN
```

`src2[1]` stays a 1–2 ULP offset through idle, including the previously published glidx 36 pair `43980a26` vs `43980a24`. Eye X/Z bits are **constant on each runtime** during idle; only eye Y and src2 slew.

First non-camera player-adjacent bit split: **ideal yaw at glidx 38** (`4380814a` vs `43808148`, 2 ULP) while player **position still matches**. That is downstream of the already-divergent camctrl, not a new walk-start discriminator.

---

## 8. Walk-start new-discriminator test (glidx 37..42)

CA8B4 executed reads at walk start are still only the six camera-record words.

| glidx | player pos | yaw/ideal | CA8B4 cam record | notes |
|---:|---|---|---|---|
| 37 | MATCH | MATCH | 1 ULP camctrl | last idle |
| 38 | MATCH | ideal 2 ULP | 2 ULP camctrl | last 0.001° display match |
| 39 | MATCH | ideal 2 ULP | camctrl Δ grows | walk pad starts; pos still identical |
| 40 | MATCH | ideal grows | camctrl **0.085°** | previous demo-sync glidx 41 |
| 41 | **SPLIT** | split | large | first player-pos bit split |
| 42 | split | split | large | |

```text
WALK_START_NEW_DISCRIMINATOR: NO
WALK_START_FIRST_NEW_EXTERNAL_INPUT_DIFFERENCE: NONE
```

Player position remains bit-identical through dump glidx 40. The large walk-start camera split is **pure amplification** of the glidx-1 ncbapivot seed through `ncbapivot(state[n], player[n])` with matching player.

---

## 9. Classification of the three “first difference” layers

```text
BIT_LEVEL_FIRST_DIFFERENCE:          node+0x74 at first ncbapivot ep3 enter
                                     (eye 4–7 ULP already in live eye;
                                     CA8B4 output is a later copy)
                                     see [[p08-user-demo4-ncbapivot-first-instruction]]
VISIBLE/MEANINGFUL_FIRST_DIFFERENCE: demo-sync glidx 41 / dump glidx 40
                                     (0.085° camctrl; eye Δ ~2)
                                     last 0.001° display match = glidx 38
GAMEPLAY_CAUSAL_FIRST_DIFFERENCE:    walk-start amplification of the
                                     glidx-1 seed (player still matches
                                     at 39..40; pos splits at 41)
```

---

## 10. Root class

```text
ROOT_CLASS:  B-CAMERA-RECURRENT-SEED
EVIDENCE:    Init record + player pos/yaw are bit-identical at first
             ncbapivot enter. First ncbapivot tick writes a different
             follow-cam eye/src2. CA8B4 copies that record identically.
             Idle ticks 2..37 have matching new player inputs and only
             recurrent camera state differs. Walk start does not introduce
             a new CA8B4 external input.
A_CANDIDATE (CA8B4):     NO
A_CANDIDATE (ncbapivot): NO — CORRECTED/SUPERSEDED by
             [[p08-user-demo4-ncbapivot-first-instruction]].
             Rec + player match at ep3 enter, but live eye
             node+0x74 / dest +0x80 already differ (7/4/2 ULP).
             CASE A / B-NCBAPIVOT-INPUT.
```

Do not implement a fix. Do not reopen `func_800CA628`, `func_800E1A58`, Rice Z-buffer, old gl385, `func_80013B70`, or C4C34. Egg remains `PROVEN_BUT_DEFERRED`.

The glidx-1 ncbapivot **output** is a preserve of `node+0x74`. First bit-level difference is that field **at ep3 enter**. Next: join `func_80110770` / `func_80110790` before this tick. Recommend **Grok 4.6 High**. Do **not** recommend Sol or Terra until same-input/different-output is proven.

```text
NEXT_SINGLE_ACTION: capture func_80110770/func_80110790 on USER_DEMO_4 before first ncbapivot ep3; no fix
NEXT_RECOMMENDED_MODEL: Grok 4.6 High
```
