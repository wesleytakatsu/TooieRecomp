---
title: "P08 — Projection classifier rejoin"
description: "Correct e82ec_ord/phi/Q1-stream rejoin of the E82EC classifier decision tuple. Q1 +0x34 0 vs 1 is real; explicit a0–a3 match; first difference is func_8001A0C0 v0 inside func_800E81E0."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, classifier, rejoin, e81e0, plus34, evidence]
---

# P08 — Projection classifier rejoin

- Date: 2026-08-20
- Model: Grok 4.6 HIGH
- Base HEAD: `2fe9e37773b0a8b93604b21aaf2925fcf0f2f2d6`
- Upstream: [[p08-demo-projection-v1-direction-producer]] (`a3=v1[2]` join artifact)
- Downstream: [[p08-demo-1a0c0-state-differential]] (B-STATE: hidden table
  `D_8007BCC4` input differs; Original `func_8001A0C0` v0 = 0 constant vs
  Recomp nonzero incrementing)

## 0. Verdict

**B-STATE.** After joining Q1 classifier events by the Q1 stream
`(a0,a1)=(176,161)`, `s3=0`, time order — not raw `glidx` and not
phi-bank pairing of consecutive `v1[2]` values — the historical

```text
Original record+0x34 = 0
Recomp   record+0x34 = 1
```

**survives**. It is **not** another join artifact.

Explicit classifier ABI args match 25/25. `func_800E81E0` is **not** a
pure function of `(a0,a1,a2,a3)`. It calls `func_8001A0C0(a0,a1)` and
indexes `D_801234C0` from that `v0`. On the aligned Q1 stream:

```text
Original func_8001A0C0 v0 = 0          (all 25)
Recomp   func_8001A0C0 v0 = 0x8660…    (nonzero, varying)
```

Globals `D_80125E40=0x43FF8000` (511.0) and `D_80125E44=0x358637BD` match.
That `v0` split produces different `f0`, different `f20=rec+8+f0`, and
different `+0x34` stores (`0x800E85F0 sh zero` vs write 1). `func_800E8918`
selector 2 / sub 0 returns the same 0 vs 1.

NO production fix. Do not instruction-trace `func_800E81E0` as Level A:
its hidden `1A0C0` input already differs.

The previous B-INPUT claim that **`a3=v1[2]`** is the differing writer
input remains **CORRECTED/SUPERSEDED** ([[p08-demo-projection-v1-direction-producer]]).

## 1. Bank producer relation

Verified on `/tmp/p08-v1dir-original.csv` Q1 (26/26):

```text
CLASSIFIER_CONSUMED_BANK_PRODUCER_RELATION:
  E82EC first loop reads D_8012D67C as phi, writes
  D_8012D600 + phi*60 + s3*12, then toggles D_8012D67C 0↔1.
  Second-loop classifier invocation N with consumed_phi=P
  reads bank P, which was written by invocation N-1's first loop
  (that invocation's first-loop phi was P).
```

Do not join a classifier row to the same-invocation first-loop store.

Raw `glidx` at E82EC time is unsafe. This recapture's `e82ec_ord` is off
by 7 between Original PC-count and Recomp `--wrap` count; join Q1 by
**stream index** of `(a0,a1)=(176,161)` + `s3=0`. That pairing still
gives bit-identical `a3` 25/25.

## 2. Alignment

```text
Q1_CLASSIFIER_ALIGNMENT:   PASS
ALIGNED_SAMPLE_COUNT:      25
JOIN_KEY:                  Q1 stream ordinal; (a0,a1)=(176,161); s3=0
ORIG_E82EC_ORD:            1072..1096
RECOMP_E82EC_ORD:          1065..1089  (offset 7; not used as join)
```

## 3. Match counts (25 aligned Q1 samples)

```text
Q1_A0_MATCH_COUNT:                    25/25
Q1_A1_MATCH_COUNT:                    25/25
Q1_A2_MATCH_COUNT:                    25/25   (0x3F820408)
Q1_A3_MATCH_COUNT:                    25/25
Q1_E81E0_F0_MATCH_COUNT:               0/25
Q1_REC8_MATCH_COUNT:                   3/25   (rest 1–few ULP; not causal)
Q1_F20_MATCH_COUNT:                    0/25
Q1_BRANCH_MATCH_COUNT:                 0/25
Q1_PLUS34_STORE_VALUE_MATCH_COUNT:     0/25
```

```text
Q1_PLUS34_REAL_DIVERGENCE_AFTER_CORRECT_JOIN: YES
FIRST_REAL_ALIGNED_DIFFERENCE:  func_8001A0C0(a0,a1) v0
                                (then f0, f20, +0x34)
```

Original Q1 always `sh zero` at `0x800E85F0` (stored 0; `rec34_old` already 0).
Recomp Q1 always stored 1 (`PLUS34_AFTER`; `rec34_old` already 1).

Threshold `elem+0x0C` = `0x41200000` (10.0) matches. Mode-1 branch:

- `|f20|<10` → write 1
- `f20<=0` → write 1
- else (`f20>10`) → write 0

Original `f20≈+945` → 0. Recomp `f20≈−219` → 1.

## 4. Compact Q1 sample (index 0 and the previously cited a3 pair)

| field | Original idx0 / idx20 | Recomp idx0 / idx20 | match |
| ----- | --------------------- | ------------------- | ----- |
| a0,a1 | 176,161 | 176,161 | YES |
| a2 | 0x3F820408 | 0x3F820408 | YES |
| a3 idx0 | 0x431BAF5E | 0x431BAF5E | YES |
| a3 idx20 | 0x4313D3A7 | 0x4313D3A7 | YES |
| a3 idx21 | 0x43136AD5 | 0x43136AD5 | YES |
| f0 idx0 | 0xC29A77FF (≈−77.23) | 0xC49B4430 (≈−1238) | NO |
| rec+8 idx0 | 0x447F9520 | ~same ULP | ~ |
| f20 idx0 | 0x446C4620 (≈+945) | 0xC35BCD00 (≈−219) | NO |
| +0x34 | 0 @ 0x800E85F0 | 1 | NO |
| 1A0C0 v0 | 0 | 0x00008660 | NO |

`0x4313D3A7` and `0x43136AD5` appear as consecutive Q1 stream values **on
both sides** (idx 20/21). They are not Original-vs-Recomp.

## 5. `func_800E81E0` contract

```text
ENTRY 0x800E81E0  EXIT 0x800E8298
E81E0_EXPLICIT_ARGS_MATCH: YES
E81E0_HIDDEN_INPUTS:
  - func_8001A0C0(a0, a1) → v0
  - D_801234C0[ (v0>>13)&7 ]  two words (shift, base)
  - D_80125E40  float 511.0  (0x43FF8000)  MATCH 25/25
  - D_80125E44  float ~1.04e-6 (0x358637BD) MATCH 25/25
E81E0_HIDDEN_INPUTS_MATCH: NO  (only 1A0C0 v0)
FIRST_E81E0_BAD_PC: n/a  (hidden input already differs)
```

With `v0=0` (Original): table index 0, `t2=6`, `t4=0`, bitfield 0,
`cvt.s.w(-511)/511 - a2` ≈ `-1-a2`, `f0 = a3 / that` ≈ −77.

Recomp `v0≠0` takes a different table cell / shift and a different `f12`,
so `a3/f12` is a different magnitude. Do not treat that as an FPU defect.

## 6. E8918 continuity

```text
PLUS34_WRITE_TO_E8918_RETURN_CONTINUITY: PROVEN
```

Paired `E8918_IN`/`RET` (a0 clobbered at return; join by call order):

- selector 2, sub 0: Original 97×0 (+3 × −1 init); Recomp 102×1
- selector 1, sub 0: Original 97×0; Recomp 104×0  (historical Q0)

## 7. Q0 control

```text
Q0_REJOIN_CONTROL: PASS
```

`func_800E8918` selector 1 returns 0 on both. Do not use classifier pair
`(127,161)` as Q0: that stream also stores +0x34 0 vs 1 (different record).

## 8. Classification

```text
ROOT_CAUSE_LEVEL: B-STATE
ROOT_CAUSE: correctly aligned Q1 +0x34 0 vs 1 is real. Writer inputs a0–a3
            match; f0(E81E0) differs because func_8001A0C0(176,161) returns
            0 (Original) vs nonzero packed v0 (Recomp). Globals/table[0]
            match. Next: 1A0C0 only.
FIX_IMPLEMENTED: NO
```

Captures (not committed): `/tmp/p08-rejoin-original.csv`,
`/tmp/p08-rejoin-recomp.csv` (and `-v1` without hidden fields).

Temporary `--wrap` / mupen PC hooks reverted before checkpoint.

```text
window_display_test:                    PASS
graphics_failure_test:                  PASS
stick_angle_thunk_test:                 PASS
matrix_submission_test:                 PASS
input_direction_test:                   PASS
bcf28_helpers_test:                     PASS
core1_ra_wrappers_test:                 PASS
attract_demo2_transition_jal_test.py:   PASS
TooieRecompiled rebuild:                PASS
```
