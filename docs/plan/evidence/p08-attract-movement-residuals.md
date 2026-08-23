---
title: "P08 — attract movement residuals after the 0x800979D8 checkpoint"
description: "User-observed residuals in attract scenes preserved separately from the validated USER_DEMO_4 helper fix."
status: active
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, attract, movement, residual, user-observation, demo1, demo5]
---

# Attract movement residuals

This note records manual observations after the `0x800979D8` checkpoint. They
are product/parity evidence, not automated proofs and not a new movement fix.

```text
USER_DEMO_1_MANUAL_STATUS:  RESIDUAL_REPRODUCED_BY_USER
USER_DEMO_2_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
USER_DEMO_3_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
USER_DEMO_4_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / bounded paired evidence
USER_DEMO_5_MANUAL_STATUS:  RESIDUAL_REPRODUCED_BY_USER
USER_DEMO_6_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
GLOBAL_MOVEMENT_PARITY:     NOT_PROVEN
GLOBAL_ATTRACT_PARITY:      NOT_PROVEN
GLOBAL_ANIMATION_PARITY:    NOT_PROVEN
```

Observed detail:

- Demo 1 is correct for roughly 80% of the scene, then follows the wrong
  trajectory / gets lost.
- Demo 5 starts correctly, then gets lost very early in the icy/snow scene.
- Demo 4 now looks correct in manual play and its bounded paired capture
  validates the `0x800979D8 → P60 → ncbapivot` event; this does not close the
  remaining attract set.

## Hypothesis

```text
TALON_TROT_IDENTITY:                         PROVEN (overlay 29 bsbtrot; bs 20 enter / bs 22 locomote)
TALON_TROT_OR_LOCOMOTION_TRANSITION_HYPOTHESIS:
CONFIRMED correlation; causal root NOT_PROVEN at initial capture stage
                                      (superseded by independent camera review below)
```

Paired USER_DEMO_5 evidence: [[p08-user-demo5-locomotion-first-divergence]].
Map `296` `MAP_128_HP_ICY_SIDE`. Identity **PROVEN**. The 698-CSV
enter-vs-locomote split at glidx 34 is **CORRECTED/SUPERSEDED**: both
runtimes call `bs_setState(22)` on that tick (timer `3f044445`,
`anctrl_isStopped=0`). The shared target-speed writer is
`baphysics_set_target_horizontal_velocity` at `0x8009B9B0`/store
`0x8009B9BC`, writing `player+0xC8` physics `+0x3C`. RT64's glidx-35 write
(`0x43d12f70` = 418.370605) is sampled by RT64 at glidx 36 and AN at glidx
35: a sampling-phase artifact, not a proven one-tick delay.
No production fix.

The first **real** post-transition movement discriminator is now
**PROVEN** as `B-CAMERA-DERIVED-DIRECTION`: at glidx 34 the first
`bastick_getAngleRelativeToBanjo` / `func_800CAF00` after `bs_setState(22)`
has matching stick bits and a different camera-record `+0x1C`, producing a
−22.5556° desired-yaw split (`430898a8` vs `431f26e4`). Camctrl already
differs at glidx 25 but is not consumed until ytype=1. Scalar target speed
still matches. Direction capture:
`/tmp/p08-demo5-direction-firstdiff-20260822T201012Z/` (SHA256 PASS twice).

Writer provenance (2026-08-22): `func_800CA628` **does** execute. It is
**CASE B / DOWNSTREAM_WRITER**. Last matching selected store is glidx 23
(`4392c3c2`). First differing store is glidx 24 ord 9 (`4392861e` vs
`4392b342`); `old_1c` still matches. Source `src2[1]` is already supplied as
the `func_8010FE14` path's `a2` stack input (`ra=8010fe24`, scratch `80045678`) **before**
`func_800CA8B4` on that tick. `CA8B4_ON_CURRENT_PROVENANCE_PATH: NO` for the
first write. Capture:
`/tmp/p08-demo5-camera1c-writer-20260822T211941Z/` (SHA256 PASS twice).

## Next single action

Capture the complete paired `func_80110A68` inputs/output at glidx 23..24,
then join its output to `func_80110790` and the existing FE14/CA628 event.
No production fix in this note.

P08 remains `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 remains
`PENDING / NOT_FORMALLY_OPEN`.

## Independent camera review update

The camera-derived classification is **CONFIRMED_WITH_CORRECTIONS**. The
semantic `yaw_ctrl` stream has its last camera-field match at glidx 24 and its
first field difference at glidx 25. That difference is initially parallel
(`ytype=2`), then becomes causal at the first `ytype=1`/`bs_cur=22`
`func_800CAF00` event at glidx 34.

Static ownership is now bounded: `func_800CAF00` reads `+0x1C` from the
player-selected `D_80127708[*(player+0x184)]` record. Dynamic writer join
proves `func_800CA628` / `0x800CA65C` executes, but it is CASE B: source
`src2[1]` already differs at glidx 24 ord 9 (`4392861e` vs `4392b342`) with
matching `old_1c`. `func_8010FE14` is only an adaptor into CA628; its `a2`
source is a caller stack vector. The selected static path copies element 1
from `s0+0x80` into `sp+0x2C` at `func_800EE7F8`/`0x800EE804`.

The new `s0+0x80` audit and subsequent dynamic capture isolates the exact boundary:
`func_80110790` is the downstream setter, while `func_80110A68` orchestrates
`800A516C -> 800F18FC`. The exact chain reads `sp+0x6C` (target position) and `s0+0x74` (camera eye position);
it has no smoothing factor or feedback read of `s0+0x80`/`+0x8C`.
Complete paired `80110A68` runtime inputs captured at `/tmp/p08-demo5-80110a68-firstdiff-20260823T011500Z`
prove that `sp+0x6C` matches bit-for-bit across all ticks (0 ULP), while `s0+0x74` (camera eye position)
is the first differing input beginning at glidx 24, produced upstream by `func_80111FD4` -> `func_80110770`. No production fix.

The immutable capture `/tmp/p08-demo5-80110a68-firstdiff-20260823T011500Z/` passes SHA256 verification twice.

### USER_DEMO_5 camera-eye translation review — 2026-08-23T01:15Z

The paired artifact closes `80110A68` as the first cause: at glidx 23 the
target `sp+0x6C`, eye `s0+0x74`, and look-at output match bit-for-bit; at
glidx 24 the target still matches, the eye differs, and the output follows
that eye difference. The first observed differing `+0x74` commit is the
`func_80111FD4` callsite `0x8011208C` (setter ordinal 9). A second setter at
`0x801132F0` is a same-tick no-op after that commit in both runtimes.

Static review corrects the reported path: `func_8011106C` copies
`s0+0x8C` into `sp+0x30`; it does not calculate a delta. `func_800EF04C`
adds that vector, while `func_80111F70` can modify the working eye through
`func_80111DB0 -> func_800EF174`. The immutable capture has no P0-P9
working-eye/delta snapshots or branch-flow trace, so the exact internal
translation discriminator remains `B-80111FD4-INTERNAL-TRANSLATION-UNRESOLVED`.
No production fix.

### USER_DEMO_5 inner 11F70/11DB0 discriminator — 2026-08-23T03:32Z

Paired inner capture `/tmp/p08-demo5-eye-inner-firstdiff-20260823T033246Z/`
passes SHA256 twice. `8011106C` remains a copy of `s0+0x8C` (zeros on this
event). `800EF04C` remains a three-float add of that copy. The first inner
difference at glidx 24 is `*a1` at `0x80045620` into `80111F70`/`80111DB0`:
Angrylion `|a1-eye|<0.01` so `0x80111DEC` skips `800EF174` and the eye stays
at the previous bits; RT64 applies `dir*0.2953` and commits
`45817cda,447db8e2,458200cd`. Classification
`B-80111F70-INPUT / B-80111DB0-INPUT`. Demo1 Trot #2 remains NOT_CAPTURED.
No production fix.

### USER_DEMO_5 a1@0x80045620 arbitration — 2026-08-23T03:32:46Z

The immutable inner capture passed SHA256 verification twice before the
interruption; the directory is now absent, so a fresh recheck is
NOT_RECHECKABLE. The branch is confirmed: 800EFB24 computes a1-a2,
800EEF94 computes its Euclidean length,
and 80111DEC compares it against 0.01f with bc1fl. AN skips 800EF174; RT
applies it and produces the known divergent eye.

The immediate final a1 writer is 800EE780 at 0x808001AC, writing the ncbapivot
local sp+0x48 from sp+0x28 and sp+0x54. 80111A98/80111ABC are not writers on
this active path. Complete 800EE780 operand and old/source/new writer
snapshots were not captured, so the boundary is
B-800EE780-INPUT-SOURCE-UNRESOLVED, not an A-candidate. No production fix.

### USER_DEMO_5 func_800EE780 operand provenance — 2026-08-23T05:35Z

Previous inner `/tmp` artifact remains LOST / NOT_RECHECKABLE; Luna arbitration
is preserved. New recapture
`/tmp/p08-demo5-ee780-source-20260823T053510Z/` SHA256 PASS twice after chmod.
Branch chain reproduced. `func_800EE780` is a vector add. Complete inputs at
glidx 24: `a1`/`sp+0x28` MATCH, `a2`/`sp+0x54` X+Z DIFF (Y MATCH). Writer of
that `a2` is `func_800EEBF0`; its `src_dir` Y already differs. Classification
`B-800EEBF0-INPUT`. `func_800EE780` is downstream. No production fix.
