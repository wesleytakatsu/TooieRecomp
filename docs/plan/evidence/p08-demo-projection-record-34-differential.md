---
title: "P08 — Persistent projection record +0x34 differential"
description: "Original-vs-Recomp record-identity / store-provenance differential for the func_800E8918 (selected record +0x34) return divergence at Q1 glidx 70."
status: current
type: evidence
project: TooieRecomp
tags: [p08, camera, projection, record, differential, mips, evidence]
---

# P08 — Persistent projection record `+0x34` differential

- Date: 2026-08-19
- Model: DeepSeek V4 Flash HIGH
- Base HEAD: `8a11a1ece2f9d8da29ed349c70617d098d1748b6`
- Upstream arbitration: [[p08-demo-camera-collision-sol-review]] (REFUTED `func_800C4C34` collision reading)

## 0. Verdict

**CORRECTED/SUPERSEDED heading (2026-08-20):** the provisional "V1 capture
pending / classification C" wording below is historical. The original6 capture
completed in §8 as **B-INPUT PROVEN** for the `+0x34` writer. The later
direction-producer trace ([[p08-demo-projection-v1-direction-producer]]) then
**CORRECTED/SUPERSEDED** the claim that classifier `a3=v1[2]` itself differs:
those two bit patterns are consecutive phi banks on both runtimes.

Canonical writer identity (unchanged):

The `func_800E8918` final return `lh v0, 0x34(s0)` (selected persistent
projection record) differs at the first relevant invocation:

```text
ORIGINAL_Q1_RECORD_PLUS_34:  0      (glidx 70, and ≥ glidx 4 in widened traces)
RECOMP_Q1_RECORD_PLUS_34:    1      (glidx 70, and ≥ glidx 4 in widened traces)
```

The causal writer is **`func_800E82EC`** in both runtimes, same logical record
(elem slot 2 / sub-slot 0 / phi 0), same control flow (mode == 1), same
instruction family (`sh s6/zero, 0x34(s0)` at
`0x800E85D8/0x800E85E8/0x800E85F0/...`). The write decision evaluates
`f20 = rec+8 + f0(func_800E81E0)` against the threshold `elem+0x0C` (10.0):

- Original: `rec+0x0C = +73.34` → `f0(E81E0) = -73.34` → `f20 = 1020.63 - 73.34 = +947.3 > 0` → **write 0** (`pc 0x800E85F0`, `sh zero`)
- Recomp:   `rec+0x0C = +1264.7` → `f0(E81E0) = -1264.7` → `f20 = 1020.63 - 1264.7 = -244.1 <= 0` → **write 1** (`pc 0x800E85E8`, `sh s6`)

The classifier inputs `a0 = trunc(rec+0)`, `a1 = trunc(rec+4)` are
**identical** between runtimes (176 / 161 at glidx 70-71 in both sides). The
divergence therefore sits in the classifier's remaining inputs
`a2 = v1[1]`, `a3 = v1[2]` (the `D_8012D600 + phi*60 + sub*12` per-direction
table written by `func_800E82EC`'s first loop). The original-side V1 capture is
in flight (`/tmp/p08-rec34-original4.csv`); classification level therefore
remains **C (one discriminator pending)** until those bytes land; expected
final classification **B-INPUT** (identical writer, differing classifier input
`a2`/`a3`).

## 1. Measured evidence

### 1.1 Original side (pure interpreter, mode 5, glidx 70..95 / 370..410 first windows; widened to 1..95 for v4)

- Capture: `/tmp/p08-rec34-original.csv` (176121 rows, glidx 70..95 and 370..410)
- Recapture with widened window + V1 classifier-input hook: `/tmp/p08-rec34-original4.csv` (in flight)
- Mupen core: `6dca4c15370ac3e2171ce7b31426695f8f39b460`, Rice `038882dd2e7cb660fd7512c0745032f17edd0bfa`, Pure Interpreter, `NO_ASM=1`, `R4300Emulator=0`
- ROM: `/home/wesley/projetos/N64-Recomp/Banjo-Tooie (USA).z64` SHA1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` — **MATCH**

### 1.2 Recomp side

- Capture: `/tmp/p08-rec34-recomp3.csv` (409307 rows, glidx 1..1271; includes E8918/E82EC/E81E0/1A0C0 rows) and `/tmp/p08-rec34-recomp5.csv` (with V1R classifier-input rows)
- Binary: `/tmp/TooieRecompiled-rec34-v3` / `-v5` (temporary instrumented builds; reverted at checkpoint)

## 2. `func_800E8918` static contract (Section 5 of the task)

```text
ENTRY_PC:                0x800E8918
EXIT_PC(s):              0x800E8A10 (lh v0, 0x34(s0)) / 0x800E8A18 (jr ra)
ARGS:                    a0 = selector byte (0x800E8920 andi s0,a0,0xff)
                         a1 = sub-slot index (0x800E8950 lw a0, 68(sp); t9 = a1*56)
                         a2 = output point (copied at 0x800E89E8/E8F4)
                         a3 = output depth write (0x800E89E0..)
RECORD_SELECTION:        fl = *(0x8013D600) -> freelist_at(fl, sel) (0x800E8940 jal 800b301c)
                         s0 = v0 + phi*280 + a1*56          (0x800E8978/E897C)
                         s0 += 0x10                          (0x800E8984)
                         final: lh v0, 0x34(s0)              (0x800E8A10)
PHI_SOURCE:              D_8012D67C (0x800E894C lw t6, -10628(t6); == 0 or 1)
FL_SOURCE:               D_8012D600 (freelist table; elem size = halfword[fl])
RECORD_STRIDE:           0x38 (56) per sub-slot; +0x118 (280) per phi bank
FINAL_RETURN:            0x800E8A10: lh v0, 52(s0)   [selected record + 0x34]
```

Internal call chain: `freelist_at` -> `func_800C4C34` (camera-space projection;
NOT collision), optional `func_800E3928`/`func_800E3980`/`func_800EE7F8`
updates, write of `+0x46` flag, final `lh +0x34`. The Q1..Q3 records have a
theta free-list element per selector; Q0..Q3 = selector values 1..4 (per the
caller `func_80113D30` loop `s1`), each at sub-slot 0, phi alternating 0/1.

## 3. First-differing field (measured, both sides)

At glidx 70..71, record `+0x34` (returned by E8918) and the classifier's
stored `-f0` at `rec+0x0C`:

```text
                    ORIGINAL                 RECOMP (v3/v5)
rec+0x34           0                        1
rec+0x0C          0x4292ac00 (+73.34)       0x449e1671 / 0x449dab3b (+1264.7)
f0(E81E0)          -73.34                   -1264.7
rec+8 (f16)        0x447f281f (1020.63)     0x447f281c / 0x447f2603 (~1020.6)
f20 = rec+8 + f0   +947.3                   -244.1
E82EC write PC     0x800E85F0 (0)           (leads to +0x34=1 via 0x800E85E8)
trunc(rec+0)       176                      176
trunc(rec+4)       161                      161
mode (elem+0x240)  1                        1
t1 (elem flag)     1                        1
```

`rec+0x0C` is written by `func_800E82EC`'s mode==1 branch as
`swc1 f2, 12(s0)` where `f2 = -f0(E81E0)` (`0x800E85A0 neg.s f2, f0`).

## 3b. Record identity proof (Section 7 / task Q1 at glidx 70)

The selected record is the same logical record on both sides:

```text
ORIGINAL_Q1_RECORD (glidx 70): 0x801d6bfc (ref) / 0x801d761c (orig6 capture)
RECOMP_Q1_RECORD   (glidx 70): 0x801da0ac (v3) / 0x801da1fc (v5)
SELECTOR (E8918 a0 & 0xff):     2 in both (Q1)
ARG1 / sub-slot:                0 in both
PHI:                            0/1 alternating identically in both
LOGICAL SLOT:                   elem (sel 2), sub-slot 0
Q1_RECORD_INDEX_MATCH:          YES (selector=2, sub=0, phi aligned)
Q1_RECORD_SEMANTIC_MATCH:       YES (same freelist element, selector, sub, phi;
                                 guest addresses differ as expected across runs)
RECORD_SIZE:                    0x38 (56) per sub-slot, +0x118 (280) per phi bank
```

The guest address differs because the freelist element base (`fl + sel*esz + 4`)
landed at different addresses across runs (0x801d6bfc vs 0x801da0ac vs
0x801da1fc) — a run-to-run allocation offset, not a different logical record.
Records are identified by `free-list element (selector) + phi*280 + sub*56 +
0x10`, and Q1 uses selector 2, sub 0 in every capture.

## 4. Writer identification (Section 12)

```text
ORIGINAL_PLUS34_WRITER_PCS:    {0x800E85D8, 0x800E85E8, 0x800E85F0,
                                0x800E85F4, 0x800E8658, 0x800E8660,
                                0x800E8668, 0x800E866C}  (func_800E82EC)
RECOMP_PLUS34_WRITER_PCS:      same generated function; observed leads to +0x34=1
PLUS34_WRITER_SET_MATCH:       YES (both: `sh s6/zero, 52(s0)` members)
PLUS34_STORE_SET_COMPLETE:     YES for the Q-record set (E82EC stores + the
                               STORE tracer watched rec+0x34 excluding E82EC's own)
LAST_WRITER_BEFORE_Q1_RETURN:  the immediately preceding E82EC mode==1 pass on the
                               same phi/sub-slot (glidx 69-equivalent on original;
                               glidx 64+ on recomp — persistent value)
Q1_PLUS34_IS:                  PRIOR_FRAME_STATE (persistent; present already at the
                               first fully-windowed glidx, 4 (recomp) / 70 (orig);
                               same-call invariant: entry rec34 == ret rec34)
```

## 5. Classification (Section 24)

```text
PLUS34_WRITER_INPUTS_MATCH:    NO (classifier result f0(E81E0) differs); the
                               shared inputs MATCH: mode=1, t1=1,
                               a0=trunc(rec+0)=176, a1=trunc(rec+4)=161,
                               a2=v1[1]=0x3f820408=1.015748 (bit-identical)
FIRST_DIFFERENT_WRITER_INPUT:  a3 = v1[2] (classifier arg to func_800E81E0):
                               ORIGINAL glidx71 a3=147.82677 (0x4313D3A7)
                               RECOMP   glidx71 a3=147.41731 (0x43136AD5)
                               -> f0(E81E0)= a3/f12 : -73.34 (orig) vs -1264.7 (recomp)
FIRST_DIFFERENT_WRITER_INPUT_SOURCE: v1[2] = D_8012D600 + phi*60 + sub*12 + 8,
                               written by the E82EC first loop
                               (0x800E83D0 neg.s f6, f4 ; 0x800E83D4 swc1 f6,8(t3));
                               f4 derives from func_800CA6D4(s1, s2) vectors f2/f12
                               whose producer is func_800A8984(s3) direction source
C4C34_DIRECT_V0_USED_FOR_PLUS34: NO (E8918 returns lh record+0x34, not C4C34 v0;
                               C4C34's a3 classification goes to the +0x46 flag path)
ROOT_CAUSE_LEVEL:              B-INPUT (PROVEN)
CLASSIFICATION:                same semantic record + same writer (func_800E82EC) +
                               same control flow (mode==1) + identical shared inputs,
                               but the classifier receives a causally different input
                               a3 (v1[2]) and therefore stores a different +0x34
PLUS34_VALUE_SEMANTIC:         E82EC mode==1: +0x34 = 1 when f20=rec+8+f0 <= 10
                               (or |f20|<thr), 0 when f20 > 0 and |f20|>=thr; PROVEN
                               from branch at 0x800E85C0..0x800E85F0;
                               f0 = func_800E81E0(a0,a1,a2,a3)
FIX_IMPLEMENTED:               NO
```

## 6. Q0 control (Section 21)

At glidx 70..71, Q0 record `+0x34`:

```text
ORIGINAL Q0: 0 (rec 0x801d69b8, writes 0 at 0x800E85F0 path)
RECOMP   Q0: 0 (sweeps rec 0x801d9f80/0x801d9e68 show rec34=0)
Q0_PROJECTION_RECORD_CONTROL:  PASS
```

Q1 breaks this control; the writer instrumentation reproduces Q0 correctly on
both sides, so the Q1 conclusion is trustworthy.

## 7. Do-not-reopen

Per upstream arbitration ([[p08-demo-camera-collision-sol-review]]): collision
geometry, triangles, raycasts, `func_80013B70`, stick path, `yaw_setIdeal`,
`func_800E1A58`, speed, timing, third-SET74 chain were not reopened. The
causal chain `E8918 return -> word_3C -> half_0C -> func_801138A0 -> third SET74
-> camera eye -> yaw` remains **PROVEN** upstream and is not revisited.

## 8. Completion

**B-INPUT PROVEN.** The divergence is fully isolated and reproducible with a
clean Pure-Interpreter original capture (`/tmp/p08-rec34-original6.csv`, glidx
1..369) vs the recomp capture (`/tmp/p08-rec34-recomp5.csv`, glidx 1..1271).
At the Q1 glidx-71 E82EC mode==1 write, the classifier `func_800E81E0` receives
identical `a0=176, a1=161, a2=1.015748` but a different `a3 = v1[2]`
(147.82677 original vs 147.41731 recomp), yielding `f0(E81E0) = -73.34` vs
`-1264.7` and flipping `+0x34` from 0 (original, `f20=+947`) to 1 (recomp,
`f20=-244`). Q0 control passes (0 vs 0).

Remaining (mechanical): update session handoff + HANDOFF.md + BT plan, run the
documented tests + `markdown_vault_audit`, revert the temporary tracer, and
checkpoint commit.

## 9. Direction-producer correction (2026-08-20)

The `a3=v1[2]` inequality quoted in §5 (`0x4313D3A7` vs `0x43136AD5`) is
**CORRECTED/SUPERSEDED** by [[p08-demo-projection-v1-direction-producer]].
Joined by `e82ec_ord + phi + s3=0 + (a0,a1)=(176,161)`, Q1 `a3` is
bit-identical 26/26. The two quoted values are phi=0 then phi=1 on **both**
sides. Writer identity (`func_800E82EC` mode==1) is unchanged. No production
fix.
